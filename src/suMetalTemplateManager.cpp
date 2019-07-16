// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Thu Oct 12 09:31:26 2017

//! \file   suMetalTemplateManager.cpp
//! \brief  A collection of methods of the class suMetalTemplateManager.

// std includes
#include <algorithm>
#include <iostream>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suCellManager.h>
#include <suCellMaster.h>
#include <suColoredLayer.h>
#include <suGenerator.h>
#include <suGeneratorManager.h>
#include <suErrorManager.h>
#include <suLayer.h>
#include <suLayerManager.h>
#include <suMetalTemplate.h>
#include <suMetalTemplateInstance.h>
#include <suNet.h>
#include <suOptionManager.h>
#include <suStatic.h>
#include <suToken.h>
#include <suTokenParser.h>
#include <suWire.h>
#include <suWireManager.h>

// module include
#include <suMetalTemplateManager.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  suMetalTemplateManager * suMetalTemplateManager::_instance = 0;


  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  // constructor
  suMetalTemplateManager::suMetalTemplateManager ()
  {
    init_ ();
    
    const sutype::layers_tc & idToLayer = suLayerManager::instance()->idToLayer();

    _layerIdToMinimalWireWidth.resize (idToLayer.size(), 0);
    
  } // end of suMetalTemplateManager::suMetalTemplateManager

  //! destructor
  suMetalTemplateManager::~suMetalTemplateManager ()
  {
    for (sutype::uvi_t i=0; i < _metalTemplates.size(); ++i) {
      delete _metalTemplates[i];
    }

    for (const auto & iter : _allMetalTemplateInstances) {
      delete iter;
    }
    
  } // suMetalTemplateManager::~suMetalTemplateManager

  // ------------------------------------------------------------
  // -
  // --- Public static methods
  // -
  // ------------------------------------------------------------

  // static
  void suMetalTemplateManager::delete_instance ()
  {
    if (suMetalTemplateManager::_instance)
      delete suMetalTemplateManager::_instance;
  
    suMetalTemplateManager::_instance = 0;
    
  } // end of suMetalTemplateManager::delete_instance


  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  //
  void suMetalTemplateManager::read_metal_template_file (const std::string & filename)
  {
    suTokenParser tokenparser;

    bool ok = tokenparser.read_file (filename);
    SUASSERT (ok, "");

    const std::string emptystr ("");

    sutype::tokens_t tokens = tokenparser.rootToken()->get_tokens ("MetalTemplate");
    
    for (sutype::uvi_t i=0; i < tokens.size(); ++i) {
      
      const suToken * token = tokens[i];

      const std::string & layername = token->get_string_value ("layer");

      const std::string & name = (token->is_defined ("name")) ?
        token->get_string_value ("name") :
        emptystr;

      if (name.empty()) {
        SUISSUE("Metal template has no name") << std::endl;
      }
      
      std::vector<int> widths = token->get_list_of_integers ("widths");
      std::vector<int> spaces = token->get_list_of_integers ("spaces");

      std::vector<int> stops;
      if (token->is_defined ("stops")) {
        stops = token->get_list_of_integers ("stops");
      }
      
      sutype::strings_t colors;
      if (token->is_defined ("colors")) {
        colors = token->get_list_of_strings ("colors");
      }
      
      sutype::dcoord_t pitch = 0;
      if (token->is_defined ("pitch")) {
        pitch = token->get_integer_value ("pitch");
        SUASSERT (pitch > 0, "");
      }

      const suLayer * baselayer = suLayerManager::instance()->get_base_layer_by_name (layername);

      if (!baselayer) {
        
        SUISSUE("Could not get layer for a metal template")
          << ": layer=" << layername
          << "; mt=" << name
          << std::endl;

        continue;
        SUASSERT (false, "Could not get layer for a metal template: layer=" << layername << "; mt=" << name);
      }
      
      suMetalTemplate * metaltemplate = new suMetalTemplate ();
      SUASSERT (metaltemplate->id() == (int)_metalTemplates.size(), "");
      _metalTemplates.push_back (metaltemplate);

      // false = !inverted
      sutype::layers_tc & mtlayers = metaltemplate->get_layers_ (false);
      sutype::dcoords_t & mtwidths = metaltemplate->get_widths_ (false);
      sutype::dcoords_t & mtspaces = metaltemplate->get_spaces_ (false);
      sutype::strings_t & mtcolors = metaltemplate->get_colors_ (false);
      
      metaltemplate->_name = name;

      SUASSERT (baselayer, "");
      SUASSERT (baselayer->is_base(), "");
      //SUASSERT (baselayer->is (sutype::lt_metal), "");
      SUASSERT (baselayer->is (sutype::lt_wire), "");
      
      metaltemplate->_baseLayer = baselayer;

      std::set<std::string> uniquecolors;
      for (const auto & iter : colors) {
        const std::string & color = iter;
        uniquecolors.insert (color);
      }
      
      std::set<const suLayer *, suLayer::cmp_const_ptr> uniquecoloredlayers;
      
      // 1st check
      for (const auto & iter : uniquecolors) {
        const std::string & color = iter;
        const suLayer * coloredlayer = baselayer->get_colored_layer_by_type (color);
        if (coloredlayer == 0) {
          coloredlayer = baselayer->get_colored_layer_by_name (color);
        }
        SUASSERT (coloredlayer, "Could not get a colored layer " << color << " for layer " << baselayer->name() << "; mt=" << metaltemplate->name());
        uniquecoloredlayers.insert (coloredlayer);
      }
      
      // 2nd check
      const sutype::coloredlayers_tc & coloredLayers = baselayer->coloredLayers();
      for (const auto & iter : coloredLayers) {
        const suColoredLayer * coloredlayer = iter;
        if (uniquecoloredlayers.count (coloredlayer) > 0) continue;
        SUISSUE("Metal template does not use an available layer color")
          << ": mt=" << metaltemplate->name()
          << "; baselayer=" << baselayer->to_str()
          << "; missedcolor=" << coloredlayer->to_str()
          << std::endl;
      }
      
      // a base layer will be used
      if (colors.empty()) {
        colors.push_back (emptystr);
      }
      
      SUASSERT (!widths.empty(), "");
      SUASSERT (!spaces.empty(), "");
      SUASSERT (!colors.empty(), "");

      // error recovery: add one more width
      if (widths.size() == 1) {
        widths.push_back (widths.front());
      }
      
      // error recovery: add widths
      if (widths.size() < colors.size()) {

        SUISSUE("Auto-fixed a metal template; added missing widths to match the number of colors") << ": " << name << std::endl;
        
        std::vector<int> hardcopywidths = widths;

        while (widths.size() < colors.size()) {
          for (sutype::uvi_t k=0; k < hardcopywidths.size(); ++k) {
            widths.push_back (hardcopywidths[k]);
            if (widths.size() == colors.size()) break;
          }
        }
      }

      SUASSERT (spaces.size() <= widths.size(), "unexpected number of spaces");
      SUASSERT (colors.size() <= widths.size(), "unexpected number of colors");

      for (sutype::uvi_t k=0; k < widths.size(); ++k) {

        sutype::dcoord_t width = widths[k];
        SUASSERT (width > 0, "");
        mtwidths.push_back (width);
      }

      // the number of spaces can be less than the number of widths; I consider them as a repeating formula
      for (sutype::uvi_t k=0; k < mtwidths.size(); ) {
        for (sutype::uvi_t j=0; j < spaces.size() && k < mtwidths.size(); ++j, ++k) {
          sutype::dcoord_t space = spaces[j];
          SUASSERT (space > 0, "");
          mtspaces.push_back (space);
        }
      }

      // the number of colors can be less than the number of widths; I consider them as a repeating formula
      for (sutype::uvi_t k=0; k < mtwidths.size(); ) {
        for (sutype::uvi_t j=0; j < colors.size() && k < mtwidths.size(); ++j, ++k) {
          const std::string & color = colors[j];
          mtcolors.push_back (color);
          
          // default
          const suLayer * coloredlayer = baselayer;
          
          // override by a custom colored version
          if (!color.empty()) {
            coloredlayer = baselayer->get_colored_layer_by_type (color);
            if (coloredlayer == 0) {
              coloredlayer = baselayer->get_colored_layer_by_name (color);
            }
            SUASSERT (coloredlayer, "Could not get a colored " << metaltemplate->_baseLayer->name() << " color=" << color << " for mt=" << metaltemplate->name());
            SUASSERT (!coloredlayer->is_base(), "Could not get a colored " << metaltemplate->_baseLayer->name() << " color=" << color << " for mt=" << metaltemplate->name());
          }
          
          mtlayers.push_back (coloredlayer);
        }
      }
      
      SUASSERT (mtspaces.size() == mtwidths.size(), "");
      SUASSERT (mtcolors.size() == mtwidths.size(), "");
      SUASSERT (mtlayers.size() == mtwidths.size(), "");

      // error recovery: repeat the first track
      if (mtlayers.back() != mtlayers.front() || mtwidths.back() != mtwidths.front()) {

        SUISSUE("Auto-fixed a metal template; added the first width/space/color to the end to create a complete cyclic structure") << ": " << name << std::endl;
        
        mtwidths.push_back (mtwidths.front());
        mtspaces.push_back (mtspaces.front());
        mtcolors.push_back (mtcolors.front());
        mtlayers.push_back (mtlayers.front());
      }

      // sanity check
      SUASSERT (mtwidths.front() == mtwidths.back(), "Unexpected metal template: " << metaltemplate->name() << " " << suStatic::to_str (mtwidths) << " " << mtwidths.front() << " " << mtwidths.back());
      SUASSERT (mtlayers.front() == mtlayers.back(), "Unexpected metal template: " << metaltemplate->name() << " " << suStatic::to_str (mtwidths) << "; #layers=" << mtlayers.size());

      const sutype::uvi_t numtracks = mtwidths.size();

      // check again
      SUASSERT (mtwidths.size() == numtracks, "");
      SUASSERT (mtspaces.size() == numtracks, "");
      SUASSERT (mtcolors.size() == numtracks, "");
      SUASSERT (mtlayers.size() == numtracks, "");
      
      // calculate pitch
      SUASSERT (pitch >= 0, "");
      
      if (pitch == 0) {
        
        for (sutype::uvi_t k=0; k < mtwidths.size(); ++k) {
          
          sutype::dcoord_t width = mtwidths[k];
          
          if (k == 0 || k+1 == mtwidths.size()) {
            pitch += width/2;
          }
          else {
            pitch += width;
          }
        }
        
        // all but the last space
        for (sutype::uvi_t k=0; k+1 < mtspaces.size(); ++k) {
          sutype::dcoord_t space = mtspaces[k];
          pitch += space;
        }
      }

      metaltemplate->_dperiod [sutype::gd_pgd] = pitch;
      
      // true = inverted
      sutype::layers_tc & invertedmtlayers = metaltemplate->get_layers_ (true);
      sutype::dcoords_t & invertedmtwidths = metaltemplate->get_widths_ (true);
      sutype::dcoords_t & invertedmtspaces = metaltemplate->get_spaces_ (true);
      sutype::strings_t & invertedmtcolors = metaltemplate->get_colors_ (true);

      SUASSERT (invertedmtlayers.empty(), "");
      SUASSERT (invertedmtwidths.empty(), "");
      SUASSERT (invertedmtspaces.empty(), "");
      SUASSERT (invertedmtcolors.empty(), "");

      for (sutype::svi_t i = (int)numtracks - 1; i >= 0; --i) {

        invertedmtlayers.push_back (mtlayers[i]);
        invertedmtwidths.push_back (mtwidths[i]);
        invertedmtspaces.push_back (mtspaces[i]);
        invertedmtcolors.push_back (mtcolors[i]);
      }

      SUASSERT (invertedmtwidths.size() == numtracks, "");
      SUASSERT (invertedmtspaces.size() == numtracks, "");
      SUASSERT (invertedmtcolors.size() == numtracks, "");
      SUASSERT (invertedmtlayers.size() == numtracks, "");
      
      // remove the last wire to create repeating structrures easier
      mtlayers.pop_back();
      mtwidths.pop_back();
      mtspaces.pop_back();
      mtcolors.pop_back();

      // remove the last wire to create repeating structrures easier
      invertedmtlayers.pop_back();
      invertedmtwidths.pop_back();
      invertedmtspaces.pop_back();
      invertedmtcolors.pop_back();

      // pre-calculate offsets
      sutype::dcoords_t & mtoffset0 = metaltemplate->get_offset_ (false);
      sutype::dcoords_t & mtoffset1 = metaltemplate->get_offset_ (true);

      SUASSERT (mtoffset0.empty(), "");
      SUASSERT (mtoffset1.empty(), "");

      mtoffset0.push_back (0);
      mtoffset1.push_back (0);

      for (sutype::uvi_t i=1; i < mtwidths.size(); ++i) {
        sutype::dcoord_t offset = mtoffset0[i-1] + mtwidths[i-1]/2 + mtspaces[i-1] + mtwidths[i]/2;
        mtoffset0.push_back (offset);
      }

      for (sutype::uvi_t i=1; i < invertedmtwidths.size(); ++i) {
        sutype::dcoord_t offset = mtoffset1[i-1] + invertedmtwidths[i-1]/2 + invertedmtspaces[i-1] + invertedmtwidths[i]/2;
        mtoffset1.push_back (offset);
      }

      SUASSERT (mtoffset0.front() == 0, "");
      SUASSERT (mtoffset1.front() == 0, "");
      SUASSERT (mtoffset0.size() == mtwidths.size(), "");
      SUASSERT (mtoffset1.size() == mtwidths.size(), "");

      if (1) {
        sutype::dcoord_t testoffset = mtoffset0.back() + mtwidths.back()/2 + mtspaces.back() + mtwidths.front()/2;
        SUASSERT (testoffset == metaltemplate->pitch(), "testoffset=" << testoffset << "; pitch=" << metaltemplate->pitch());
      }
      if (1) {
        sutype::dcoord_t testoffset = mtoffset1.back() + invertedmtwidths.back()/2 + invertedmtspaces.back() + invertedmtwidths.front()/2;
        SUASSERT (testoffset == metaltemplate->pitch(), "testoffset=" << testoffset << "; pitch=" << metaltemplate->pitch());
      }

      sutype::dcoord_t totwirelength = 0;
      
      for (const auto & iter0 : stops) {
        sutype::dcoord_t wirelength = iter0;
        SUASSERT (wirelength > 0, "");
        metaltemplate->_stops.push_back (wirelength);
        totwirelength += wirelength;
      }
      metaltemplate->_dperiod[sutype::gd_ogd] = totwirelength;
      
    }
    
  } // end of suMetalTemplateManager::read_metal_template_file

  //
  void suMetalTemplateManager::dump_mock_via_generators (const std::string & filename)
    const
  {
    suStatic::create_parent_dir (filename);

    std::ofstream out (filename);
    if (!out.is_open()) {
      SUISSUE("Could not open file for writing") << ": " << filename << std::endl;
      SUASSERT (false, "");
    }

    out << "# Auto-generated file" << std::endl;
    out << "# Disclaimer: This file is not intended to be reader friendly" << std::endl;
    out << std::endl;
    
    std::map<const suLayer *, std::set<sutype::dcoord_t>, suLayer::cmp_const_ptr_by_level> widthsPerLayer;

    for (const auto & iter1 : _metalTemplates) {

      const suMetalTemplate * mt = iter1;
      
      const suLayer * layer = mt->_baseLayer->base();

      const sutype::dcoords_t & widths = mt->get_widths(false);

      for (const auto & iter1 : widths) {

        sutype::dcoord_t width = iter1;

        widthsPerLayer[layer].insert (width);
      }
    }

    const sutype::layers_tc & layers = suLayerManager::instance()->layers();

    for (const auto & iter1 : layers) {

      const suLayer * vialayer = iter1;
      
      if (!vialayer->is_base()) continue;
      if (!vialayer->is (sutype::lt_via)) continue;

      const sutype::layers_tc & electricallyConnected = vialayer->electricallyConnected();

      for (sutype::uvi_t i=0; i < electricallyConnected.size(); ++i) {

        const suLayer * layer1 = electricallyConnected[i];
        if (!layer1->is (sutype::lt_metal)) continue;
        if (!layer1->is (sutype::lt_wire)) continue;

        const suLayer * layerbelow = 0;
        const suLayer * layerupper = 0;

        if (layer1->level() < vialayer->level()) layerbelow = layer1;
        if (layer1->level() > vialayer->level()) layerupper = layer1;

        if (layerbelow == 0 && layerupper == 0) continue;

        int index = 0;

        for (sutype::uvi_t k=i+1; k < electricallyConnected.size(); ++k) {

          const suLayer * layer2 = electricallyConnected[k];
          if (!layer2->is (sutype::lt_metal)) continue;
          if (!layer2->is (sutype::lt_wire)) continue;
          SUASSERT (layer2 != layer1, "");

          if (layer2->level() < vialayer->level()) layerbelow = layer2;
          if (layer2->level() > vialayer->level()) layerupper = layer2;

          if (layerbelow == 0 || layerupper == 0) continue;

          const auto & widthsbelow = widthsPerLayer [layerbelow];
          const auto & widthsupper = widthsPerLayer [layerupper];
          
          SUASSERT (layerbelow->pgd() != layerupper->pgd(), "");

          for (const auto & iter2 : widthsbelow) {

            sutype::dcoord_t widthbelow = iter2;

            for (const auto & iter3 : widthsupper) {
              
              sutype::dcoord_t widthupper = iter3;

              sutype::dcoord_t cutwidth  = (layerbelow->pgd() == sutype::go_ver) ? widthbelow : widthupper;
              sutype::dcoord_t cutheight = (cutwidth == widthbelow) ? widthupper : widthbelow;

              cutwidth  -= 40;
              cutheight -= 40;

              out << "Generator name=" << vialayer->name()
                  << "_" << layerbelow->name() << "_" << widthbelow
                  << "_" << layerupper->name() << "_" << widthupper
                  << "_mock_" << index << " {"
                  << std::endl;

              sutype::dcoord_t xcoverage1 = (layerbelow->pgd() == sutype::go_ver) ? 0 : 100;
              sutype::dcoord_t ycoverage1 = (layerbelow->pgd() == sutype::go_hor) ? 0 : 100;

              sutype::dcoord_t xcoverage2 = (layerupper->pgd() == sutype::go_ver) ? 0 : 100;
              sutype::dcoord_t ycoverage2 = (layerupper->pgd() == sutype::go_hor) ? 0 : 100;

              out << "  Layer1" << " value=" << layerbelow->name() << " {" << std::endl;
              out << "    x_coverage value=" << suStatic::udm_to_micron (xcoverage1) << std::endl;
              out << "    y_coverage value=" << suStatic::udm_to_micron (ycoverage1) << std::endl;
              out << "    widths value=" << suStatic::udm_to_micron (widthbelow) << std::endl;
              out << "  }" << std::endl;

              out << "  Layer2" << " value=" << layerupper->name() << " {" << std::endl;
              out << "    x_coverage value=" << suStatic::udm_to_micron (xcoverage2) << std::endl;
              out << "    y_coverage value=" << suStatic::udm_to_micron (ycoverage2) << std::endl;
              out << "    widths value=" << suStatic::udm_to_micron (widthupper) << std::endl;
              out << "  }" << std::endl;

              out << "  CutWidth value=" << suStatic::udm_to_micron (cutwidth) << std::endl;
              out << "  CutHeight value=" << suStatic::udm_to_micron (cutheight) << std::endl;
              out << "  cutlayer value=" << vialayer->name() << std::endl;
              
              out << "}" << std::endl;
              out << std::endl;

              ++index;
            }
          }
        }
      }

      bool foundalllayers = true;
      
      for (const auto & iter2 : electricallyConnected) {

        const suLayer * layer = iter2;
        if (widthsPerLayer.count (layer) == 0) {
          foundalllayers = false;
          break;
        }
      }
      if (!foundalllayers) continue;

      
      
    }
    
    out.close ();

    SUINFO(1) << "Written " << filename << std::endl;
    
  } // end of suMetalTemplateManager::dump_mock_via_generators

  //
  void suMetalTemplateManager::create_metal_template_instances ()
  {
    const bool np = false;
    
    SUINFO(1) << "Create metal template instances." << std::endl;
    
    sutype::tokens_t tokens = suOptionManager::instance()->token()->get_tokens ("MetalTemplateInstance");
    SUINFO(1) << "Found " << tokens.size() << " custom metal template instances." << std::endl;
    
    const suRectangle & topCellMasterBbox = suCellManager::instance()->topCellMaster()->bbox();
    
    for (const auto & iter : tokens) {
      
      suToken * token = iter;

      const std::string & mtname = token->get_string_value ("template");
      const suMetalTemplate * mt = get_metal_template_by_name (mtname);
      SUASSERT (mt, "Could not get metal template: " << mtname);

      SUINFO(np) << "Found custom metal template instance: " << mt->name() << std::endl;

      suRectangle region = topCellMasterBbox;

      bool inverted = false;

      if (token->is_defined ("inverted")) {

        inverted = token->get_boolean_value ("inverted");
      }

      if (token->is_defined ("region")) {
        
        std::vector<int> bbox = token->get_list_of_integers ("region", ':');
        SUASSERT (bbox.size() == 4, "");
        
        unsigned i = 0;
        sutype::dcoord_t xl = bbox[i]; ++i;
        sutype::dcoord_t yl = bbox[i]; ++i;
        sutype::dcoord_t xh = bbox[i]; ++i;
        sutype::dcoord_t yh = bbox[i]; ++i;

        SUASSERT (xl < xh, "");
        SUASSERT (yl < yh, "");

        region.xl (xl);
        region.yl (yl);
        region.xh (xh);
        region.yh (yh);

        SUINFO(np) << "Found custom region: " << region.to_str() << std::endl;
      }
      else {
        
        SUINFO(np) << "Used default region: " << region.to_str() << std::endl;
      }

      sutype::dcoord_t pgdoffset0 = 0; // to set
      sutype::dcoord_t ogdoffset0 = 0; // to set

      for (int mode = 1; mode <= 2; ++mode) {

        std::string str_gd = (mode == 1) ? std::string ("p") : std::string ("o");
        std::string str_gdoffset_abs = str_gd + std::string ("gdoffset_abs");
        std::string str_gdoffset_rel = str_gd + std::string ("gdoffset_rel");
        
        sutype::dcoord_t gdoffset0 = 0; // to set
        sutype::dcoord_t gdoffset1 = 0; // tmp to calculate from gdoffset_abs
        sutype::dcoord_t gdoffset2 = 0; // tmo to calculate from gdoffset_rel

        bool found1 = false; // found gdoffset_abs
        bool found2 = false; // found gdoffset_rel
        
        if (token->is_defined (str_gdoffset_abs)) {
          gdoffset1 = token->get_integer_value (str_gdoffset_abs);
          found1 = true;
        }

        // unevident logic; calculate such gdoffset1 so that a first track starts at xl/yl of the region and corresponds to x/y=gdoffset_rel for a mti with gdoffset=0
        if (token->is_defined (str_gdoffset_rel)) {
          found2 = true;
          gdoffset2 = - (token->get_integer_value (str_gdoffset_rel));
          gdoffset2 += ((mode == 1) ? region.sidel (mt->baseLayer()->pgd()) : region.edgel (mt->baseLayer()->pgd()));
        }
        
        if (found1 && found2) {
          SUASSERT (gdoffset1 == gdoffset2, "");
        }

        if (!found1 && !found2) {

          if (mode == 1) {
            SUASSERT (false, "You must explicitly specify " << str_gd << "gd offset for the metal template instance.");
          }

          if (mode == 2 && !mt->stops().empty()) {
            //SUASSERT (false, "You must explicitly specify " << str_gd << "gd offset for the metal template instance.");
          }
          
          continue;
        }
        
        // set final gdoffset0
        if (found1) {
          gdoffset0 = gdoffset1;
          SUINFO(np) << "Found custom " << str_gd << "gdoffset=" << gdoffset0 << std::endl;
        }
        else if (found2) {
          gdoffset0 = gdoffset2;
          SUINFO(np) << "Calculated custom " << str_gd << "gdoffset=" << gdoffset0 << std::endl;
        }
        else {
          SUINFO(np) << "Use default " << str_gd << "gdoffset=" << gdoffset0 << std::endl;
        }
        
        if        (mode == 1) { pgdoffset0 = gdoffset0;
        } else if (mode == 2) { ogdoffset0 = gdoffset0;
        } else {
          SUASSERT (false, "");
        }
      }
      
      suMetalTemplateInstance * mti = new suMetalTemplateInstance (mt, pgdoffset0, ogdoffset0, region, inverted);

      SUINFO(1) << "Created custom mti: " << mti->to_str() << std::endl;
      
      _allMetalTemplateInstances.push_back (mti);
    }

    // temporary solution for backward compatibility
    // create default metal template instances for the rest of metal templates
    
    for (sutype::uvi_t i=0; i < _metalTemplates.size(); ++i) {

      const suMetalTemplate * mt = _metalTemplates[i];

      bool foundCustomMti = false;
      for (const auto & iter : _allMetalTemplateInstances) {
        const suMetalTemplateInstance * mti = iter;
        if (mti->metalTemplate() == mt || mti->metalTemplate()->baseLayer() == mt->baseLayer()) {
          foundCustomMti = true;
          break;
        }
      }
      if (foundCustomMti) continue;

      // create a default mti for this metal template
      suMetalTemplateInstance * mti = new suMetalTemplateInstance (mt,
                                                                   0, // pgdoffset
                                                                   0, // ogdoffset
                                                                   topCellMasterBbox, // region
                                                                   false); // false = direct; true = inverted
      
      SUINFO(1) << "Created default mti: " << mti->to_str() << std::endl;
      
      _allMetalTemplateInstances.push_back (mti);
    }

    //std::sort (_allMetalTemplateInstances.begin(), _allMetalTemplateInstances.end(), suStatic::compare_metal_template_instances);
    merge_metal_template_instances ();

    for (const auto & iter1 : _allMetalTemplateInstances) {

      const suMetalTemplateInstance * mti = iter1;
      const sutype::dcoords_t & widths = mti->metalTemplate()->get_widths(false);
      const suLayer * baselayer = mti->metalTemplate()->baseLayer();
      SUASSERT (baselayer, "");
      SUASSERT (baselayer->is_base(), "");
      sutype::id_t layerid = baselayer->base_id();
      SUASSERT (layerid >= 0 && layerid < (sutype::id_t)_layerIdToMinimalWireWidth.size(), "");
      SUASSERT (_layerIdToMinimalWireWidth[layerid] >= 0, "");

      for (const auto & iter2 : widths) {
        
        sutype::dcoord_t width = iter2;
        SUASSERT (width > 0, "");
        if (_layerIdToMinimalWireWidth[layerid] == 0 || width < _layerIdToMinimalWireWidth[layerid])
          _layerIdToMinimalWireWidth[layerid] = width;
      }
    }
    
  } // end of suMetalTemplateManager::create_metal_template_instances

  //
  void suMetalTemplateManager::merge_metal_template_instances ()
  {
    sutype::uvi_t num0 = _allMetalTemplateInstances.size();

    for (sutype::svi_t i=0; i < (sutype::svi_t)_allMetalTemplateInstances.size(); ++i) {

      suMetalTemplateInstance * mti0 = _allMetalTemplateInstances[i];

      for (sutype::svi_t k=i+1; k < (sutype::svi_t)_allMetalTemplateInstances.size(); ++k) {

        suMetalTemplateInstance * mti1 = _allMetalTemplateInstances[k];

        if (mti0->metalTemplate() != mti1->metalTemplate()) continue;
        if (mti0->shift() != mti1->shift()) continue;
        if (mti0->isInverted() != mti1->isInverted()) continue;
        if (!mti0->region().has_at_least_common_point (mti1->region())) continue;

        if (mti0->region().covers_compeletely (mti1->region())) {

          delete mti1;
          _allMetalTemplateInstances[k] = _allMetalTemplateInstances.back();
          _allMetalTemplateInstances.pop_back();
          --k;
          continue;
        }

        if (mti1->region().covers_compeletely (mti0->region())) {

          _allMetalTemplateInstances[i] = mti1;
          delete mti0;
          _allMetalTemplateInstances[k] = _allMetalTemplateInstances.back();
          _allMetalTemplateInstances.pop_back();
          --i;
          break;
        }

        sutype::dcoord_t xl = mti0->region().xl();
        sutype::dcoord_t xh = mti0->region().xh();
        sutype::dcoord_t yl = mti0->region().yl();
        sutype::dcoord_t yh = mti0->region().yh();
        
        if (mti0->region().yl() == mti1->region().yl() && mti0->region().yh() == mti1->region().yh()) {

          xl = std::min (mti0->region().xl(), mti1->region().xl());
          xh = std::max (mti0->region().xh(), mti1->region().xh());
        }
        else if (mti0->region().xl() == mti1->region().xl() && mti0->region().xh() == mti1->region().xh()) {

          yl = std::min (mti0->region().yl(), mti1->region().yl());
          yh = std::max (mti0->region().yh(), mti1->region().yh());
        }
        else {

          continue;
        }

        mti0->update_region (xl, yl, xh, yh);
        delete mti1;
        _allMetalTemplateInstances[k] = _allMetalTemplateInstances.back();
        _allMetalTemplateInstances.pop_back();
        --k;
      }
    }

    std::sort (_allMetalTemplateInstances.begin(), _allMetalTemplateInstances.end(), suStatic::compare_metal_template_instances);

    sutype::uvi_t num1 = _allMetalTemplateInstances.size();

    if (num1 != num0) {
      SUINFO(1) << num0 << " MTIs were merged into " << num1 << " MTIs." << std::endl;
    }
    
  } // end of suMetalTemplateManager::merge_metal_template_instances

  //
  sutype::metaltemplates_tc suMetalTemplateManager::get_metal_templates_by_layer (const suLayer * layer)
    const
  {
    sutype::metaltemplates_tc mts;
    
    for (sutype::uvi_t i=0; i < _metalTemplates.size(); ++i) {
      
      const suMetalTemplate * mt = _metalTemplates[i];
      if (mt->baseLayer() == layer->base()) mts.push_back (mt);
    }
    
    return mts;
    
  } // end of suMetalTemplateManager::get_metal_templates_by_layer

  //
  const suMetalTemplate * suMetalTemplateManager::get_metal_template_by_name (const std::string & mtname)
    const
  {
    for (sutype::uvi_t i=0; i < _metalTemplates.size(); ++i) {
      
      const suMetalTemplate * mt = _metalTemplates[i];
      if (mt->name().compare (mtname) == 0) return mt;
    }

    return 0;
    
  } // end of suMetalTemplateManager::get_metal_template_by_name
  
  //
  void suMetalTemplateManager::print_metal_templates ()
    const
  {
    const bool inverted = false;

    for (sutype::uvi_t i=0; i < _metalTemplates.size(); ++i) {
      
      const suMetalTemplate * mt = _metalTemplates[i];
      
      SUINFO(1)
        << "MetalTemplate"
        << " layer=" << mt->_baseLayer->name()
        << " name=" << mt->_name
        << " period=" << mt->pitch()
        << std::endl;
      
      const sutype::layers_tc & layers = mt->get_layers (inverted);
      const sutype::dcoords_t & widths = mt->get_widths (inverted);
      const sutype::dcoords_t & spaces = mt->get_spaces (inverted);

      for (sutype::uvi_t k=0; k < layers.size(); ++k) {

        const suLayer *  layer = layers[k];
        sutype::dcoord_t width = widths[k];
        sutype::dcoord_t space = spaces[k];

        SUINFO(1)
          << "  "
          << layer->name()
          << "; width=" << width
          << "; space=" << space
          << std::endl;
      }

      const sutype::dcoords_t & stops = mt->stops();

      SUINFO(1)
        << "  "
        << "stops: ";

      if (!stops.empty()) {
        SUOUT(1) << suStatic::to_str(stops);
      }
      else {
        SUOUT(1) << "<no stops>";
      }

      SUOUT(1) << std::endl;

      sutype::dcoord_t lineEndPeriod = mt->lineEndPeriod();
      SUASSERT (lineEndPeriod >= 0, "");
      
      if (lineEndPeriod != 0) {
        SUINFO(1)
          << "  Line-end period: " << lineEndPeriod
          << std::endl;
      }
    }

    //dump_mock_via_generators ("via_generators.txt");
    //SUABORT;
    
  } // end of suMetalTemplateManager::print_metal_templates

  //
  bool suMetalTemplateManager::check_regions_of_metal_template_instances ()
    const
  {
    bool ok = true;

    // sanity check
    for (sutype::uvi_t i=0; i < _allMetalTemplateInstances.size(); ++i) {

      const suMetalTemplateInstance * mti0 = _allMetalTemplateInstances[i];

      for (sutype::uvi_t k=i+1; k < _allMetalTemplateInstances.size(); ++k) {

        const suMetalTemplateInstance * mti1 = _allMetalTemplateInstances[k];
        
        if (mti0->metalTemplate()->baseLayer() != mti1->metalTemplate()->baseLayer()) continue;
        
        if (mti0->metalTemplate() == mti1->metalTemplate() && 
            mti0->shift() == mti1->shift() &&
            mti0->isInverted() == mti1->isInverted()) {
          continue; // not a conflict even mtis overlap
        }

        if (mti0->region().has_a_common_rect (mti1->region())) {
          
          SUISSUE("Two metal template instances belong to the same base layer and regions overlap (written an error mti_overlap)")
            << ": mti0=" << mti0->to_str() << "; mti1=" << mti1->to_str()
            << std::endl;
          
          ok = false;

          sutype::dcoord_t errorxl = 0;
          sutype::dcoord_t erroryl = 0;
          sutype::dcoord_t errorxh = 0;
          sutype::dcoord_t erroryh = 0;

          mti0->region().calculate_overlap (mti1->region(), errorxl, erroryl, errorxh, erroryh);
          
          suErrorManager::instance()->add_error (std::string ("mti_overlap_") + mti0->metalTemplate()->baseLayer()->name(),
                                                 errorxl,
                                                 erroryl,
                                                 errorxh,
                                                 erroryh);
        }
      }
    }

    return ok;
  }
  // end of check_regions_of_metal_template_instances

  //
  void suMetalTemplateManager::check_metal_template_instances ()
    const
  {
    const bool inverted = false; // here we don't need any custom "inverted"

    std::set<std::string> messages;

    for (sutype::uvi_t t0 = 0; t0 < _allMetalTemplateInstances.size(); ++t0) {

      const suMetalTemplateInstance * mti0 = _allMetalTemplateInstances [t0];

      const suMetalTemplate * mt0 = mti0->metalTemplate();
      const sutype::layers_tc & layers0 = mt0->get_layers(inverted);
      const sutype::dcoords_t & widths0 = mt0->get_widths(inverted);

      SUASSERT (layers0.size() == widths0.size(), "");

      for (sutype::uvi_t t1 = t0+1; t1 < _allMetalTemplateInstances.size(); ++t1) {
      
        const suMetalTemplateInstance * mti1 = _allMetalTemplateInstances [t1];
      
        const suMetalTemplate * mt1 = mti1->metalTemplate();
        const sutype::layers_tc & layers1 = mt1->get_layers(inverted);
        const sutype::dcoords_t & widths1 = mt1->get_widths(inverted);

        SUASSERT (layers1.size() == widths1.size(), "");

        for (sutype::uvi_t i0 = 0; i0 < layers0.size(); ++i0) {

          const suLayer *  layer0 = layers0 [i0];
          sutype::dcoord_t width0 = widths0 [i0];

          for (sutype::uvi_t i1 = 0; i1 < layers1.size(); ++i1) {
          
            const suLayer *  layer1 = layers1 [i1];
            sutype::dcoord_t width1 = widths1 [i1];

            const suLayer * layerA = (layer0->level() < layer1->level()) ? layer0 : layer1;
            const suLayer * layerB = (layerA == layer0) ? layer1 : layer0;

            sutype::dcoord_t widthA = (layerA == layer0) ? width0 : width1;
            sutype::dcoord_t widthB = (widthA == width0) ? width1 : width0;

            const suMetalTemplate * mtA = (layerA == layer0) ? mt0 : mt1;
            const suMetalTemplate * mtB = (mtA     == mt0)   ? mt1 : mt0;

            const suLayer * cutlayer = suLayerManager::instance()->get_base_via_layer (layerA, layerB);
            if (!cutlayer) continue;

            const sutype::generators_tc & generators = suGeneratorManager::instance()->get_generators (cutlayer);
          
            bool foundGenerator = false;

            for (const auto & iter1 : generators) {  
              const suGenerator * generator = iter1;
              if (generator->matches (layerA, layerB, widthA, widthB)) {
                foundGenerator = true;
                break;
              }
            }
          
            if (foundGenerator) continue;

            std::ostringstream oss;
          
            oss
              << "Found no via generators for"
              << " cut layer " << cutlayer->to_str() << " between templates "
              << "(" << mtA->name() << "; " << layerA->name() << "; w=" << widthA << ")"
              << " and "
              << "(" << mtB->name() << "; " << layerB->name() << "; w=" << widthB << ")"
              << "; generators=" << generators.size()
              << ".";
            
            messages.insert (oss.str());
          }
        }
      }
    }

    for (const auto & iter : messages) {
      SUISSUE("MTI has an issue") << ": " << iter << std::endl;
    }
    
  } // end of suMetalTemplateManager::check_metal_template_instances

  //
  const suMetalTemplateInstance * suMetalTemplateManager::get_best_mti_for_wire (suWire * wire)
    const
  {
    SUASSERT (wire, "");
    SUASSERT (wire->layer()->is (sutype::lt_wire), "");
    SUASSERT (!wire->layer()->is (sutype::lt_via), "");

    const suMetalTemplateInstance * bestMti = 0;
    const suLayer * bestLayer = 0;
    sutype::dcoord_t bestSideL = 0;
    sutype::dcoord_t bestSideH = 0;
    sutype::dcoord_t bestDiff = 0;

    get_best_matching_metal_template_instance_ (wire->layer(),
                                                wire->edgel(),
                                                wire->edgeh(),
                                                wire->sidel(),
                                                wire->sideh(),
                                                bestMti,
                                                bestLayer,
                                                bestSideL,
                                                bestSideH,
                                                bestDiff);
    
    if (bestMti == 0 || bestLayer != wire->layer() || bestSideL > wire->sidel() || bestSideH < wire->sideh()) {
      return 0;
    }

    return bestMti;
    
  } // end of suMetalTemplateManager::get_best_mti_for_wire
  
  //
  suWire * suMetalTemplateManager::create_wire_to_match_metal_template_instance (suWire * wire,
                                                                                 const sutype::wire_type_t wiretype)
    const
  {
    SUASSERT (wire->layer()->is (sutype::lt_wire), "");
    SUASSERT (!wire->layer()->is (sutype::lt_via), "");

    const suMetalTemplateInstance * bestMti = 0;
    const suLayer * bestLayer = 0;
    sutype::dcoord_t bestSideL = 0;
    sutype::dcoord_t bestSideH = 0;
    sutype::dcoord_t bestDiff = 0;

    get_best_matching_metal_template_instance_ (wire->layer(),
                                                wire->edgel(),
                                                wire->edgeh(),
                                                wire->sidel(),
                                                wire->sideh(),
                                                bestMti,
                                                bestLayer,
                                                bestSideL,
                                                bestSideH,
                                                bestDiff);
    
    if (bestMti == 0 || bestLayer != wire->layer() || bestSideL > wire->sidel() || bestSideH < wire->sideh()) {
      //SUASSERT (false, "Wire doesn't match any metal template instance: " << wire->to_str());
      return 0;
    }

    SUASSERT (bestMti, "");

    sutype::dcoord_t edgel = wire->edgel();
    sutype::dcoord_t edgeh = wire->edgeh();

    edgel = bestMti->get_line_end_on_the_grid (edgel, true);
    edgeh = bestMti->get_line_end_on_the_grid (edgeh, false);
    
    if (edgel     == wire->edgel() &&
        edgeh     == wire->edgeh() &&
        bestSideL == wire->sidel() &&
        bestSideH == wire->sideh())  {
      
      return wire;
    }
    
    suWire * newwire = suWireManager::instance()->create_wire_from_edge_side (wire->net(),
                                                                              bestLayer,
                                                                              edgel,
                                                                              edgeh,
                                                                              bestSideL,
                                                                              bestSideH,
                                                                              wiretype);
      
    return newwire; 
    
  } // end of suMetalTemplateManager::create_wire_to_match_metal_template_instance

  //
  suWire * suMetalTemplateManager::create_wire_to_match_metal_template_instance (const suNet * net,
                                                                                 const suLayer * layer,
                                                                                 const sutype::dcoord_t edgel,
                                                                                 const sutype::dcoord_t edgeh,
                                                                                 const sutype::dcoord_t sidel,
                                                                                 const sutype::dcoord_t sideh,
                                                                                 const sutype::wire_type_t wiretype)
    const
  {
    const suMetalTemplateInstance * bestMti = 0;
    const suLayer * bestLayer = 0;
    sutype::dcoord_t bestSideL = 0;
    sutype::dcoord_t bestSideH = 0;
    sutype::dcoord_t bestDiff = 0;

    get_best_matching_metal_template_instance_ (layer,
                                                edgel,
                                                edgeh,
                                                sidel,
                                                sideh,
                                                bestMti,
                                                bestLayer,
                                                bestSideL,
                                                bestSideH,
                                                bestDiff);

    if (bestMti == 0 || bestSideL > sidel || bestSideH < sideh) {
      return 0;
    }

    SUASSERT (bestMti, "");

    sutype::dcoord_t bestEdgeL = bestMti->get_line_end_on_the_grid (edgel, true);
    sutype::dcoord_t bestEdgeH = bestMti->get_line_end_on_the_grid (edgeh, false);
    
    suWire * newwire = suWireManager::instance()->create_wire_from_edge_side (net,
                                                                              bestLayer,
                                                                              bestEdgeL,
                                                                              bestEdgeH,
                                                                              bestSideL,
                                                                              bestSideH,
                                                                              wiretype);

    return newwire;
    
  } // end of suMetalTemplateManager::create_wire_to_match_metal_template_instance
  
  // \return true if all ok
  bool suMetalTemplateManager::check_wire_with_metal_template_instances (suWire * wire)
    const
  {    
    const suMetalTemplateInstance * bestMti = 0;
    const suLayer * bestLayer = 0;
    sutype::dcoord_t bestSideL = 0;
    sutype::dcoord_t bestSideH = 0;
    sutype::dcoord_t bestDiff = 0;

    get_best_matching_metal_template_instance_ (wire->layer(),
                                                wire->edgel(),
                                                wire->edgeh(),
                                                wire->sidel(),
                                                wire->sideh(),
                                                bestMti,
                                                bestLayer,
                                                bestSideL,
                                                bestSideH,
                                                bestDiff);
    
    if (bestMti == 0 || bestDiff > 0 || bestLayer != wire->layer()) {
      
      SUISSUE("Wire does not match any metal template instance (written an error no_mti_found)")
        << ": " << wire->to_str()
        << std::endl;
      
      suErrorManager::instance()->add_error (std::string("no_mti_found") + "_" + wire->layer()->name() /*+ "_" + wire->net()->name()*/,
                                             wire->rect().xl(),
                                             wire->rect().yl(),
                                             wire->rect().xh(),
                                             wire->rect().yh());
    }
    else {
      return true;
    }
    
    if (bestMti && bestDiff > 0) {
      SUINFO(0)
        << "The nearest match for the wire " << wire->to_str() << " is "
        << "mti=" << bestMti->to_str()
        << "; sidel=" << bestSideL
        << "; sideh=" << bestSideH
        << "; layer=" << bestLayer->name()
        << std::endl;
    }
    
    if (bestMti && bestDiff == 0 && bestLayer != wire->layer()) {
      SUINFO(0)
        << "Tested wire " << wire->to_str() << " matches geometry of mti=" << bestMti->to_str() << " but layer must be " << bestLayer->name()
        << std::endl;
    }

    // print wire in input format
    if (bestMti) {
      
      sutype::dcoord_t xl = (bestLayer->pgd() == sutype::go_ver) ? bestSideL : wire->edgel();
      sutype::dcoord_t xh = (bestLayer->pgd() == sutype::go_ver) ? bestSideH : wire->edgeh();
      sutype::dcoord_t yl = (bestLayer->pgd() == sutype::go_hor) ? bestSideL : wire->edgel();
      sutype::dcoord_t yh = (bestLayer->pgd() == sutype::go_hor) ? bestSideH : wire->edgeh();

      SUOUT(0)
        << "Wire"
        << " net=" << wire->net()->name()
        << " layer=" << bestLayer->name()
        << " rect=" << xl << ":" << yl << ":" << xh << ":" << yh
        << std::endl;
    }

    return false;
    
  } // end of suMetalTemplateManager::check_wire_with_metal_template_instances

  //
  void suMetalTemplateManager::create_all_possible_mtis (suWire * wire,
                                                         sutype::metaltemplateinstances_t & externalmtis)
    const
  {
    for (const auto & iter0 : _metalTemplates) {
      
      const suMetalTemplate * mt = iter0;
      const suLayer * baselayer = mt->baseLayer();
      
      if (baselayer != wire->layer()->base()) continue;

      for (int i=0; i <= 1; ++i) {

        bool inverted = (bool)i;

        const sutype::dcoords_t & mtwidths = mt->get_widths (inverted);
        const sutype::layers_tc & mtlayers = mt->get_layers (inverted);

        SUASSERT (mtwidths.size() == mtlayers.size(), "");

        for (sutype::uvi_t i=0; i < mtwidths.size(); ++i) {

          sutype::dcoord_t mtwidth = mtwidths[i];
          const suLayer * mtlayer  = mtlayers[i];

          if (wire->width() != mtwidth) continue;
          if (wire->layer() != mtlayer) continue;

          sutype::dcoord_t trackoffset = mt->get_track_offset (i, inverted);
          SUASSERT (trackoffset >= 0, "");
          
          sutype::dcoord_t mtipgdoffset = wire->sidec() - trackoffset;
          mtipgdoffset = mt->convert_to_canonical_offset (sutype::gd_pgd, mtipgdoffset);

          // MetalTemplateInstance template=<template_name> pgdoffset_abs=<value> region=<xl>:<yl>:<xh>:<yh> inverted=<0|1>
          SUINFO(1)
            << "Wire"
            << " net=" << wire->net()->name()
            //<< " " << wire->to_str()
            << " would match mti: "
            << "{"
            << mt->name()
            << "; pgdoffset=" << mtipgdoffset
            << "; region=<none>"
            << "; inverted=" << inverted
            << "}"
            << "; you can copy/paste folowing line to the option file and edit the region:"
            << " \""
            << "MetalTemplateInstance"
            << " template="      << mt->name()
            << " pgdoffset_abs=" << mtipgdoffset
            << " region="        << "<xl>:<yl>:<xh>:<yh>"
            << " inverted="      << (int)inverted
            << "\""
            << std::endl;
        }
      }
    }
    
  } // end of suMetalTemplateManager::create_all_possible_mtis

  //
  sutype::dcoord_t suMetalTemplateManager::get_minimal_wire_width (const suLayer * layer)
    const
  {
    SUASSERT (layer, "");
    
    sutype::id_t layerid = layer->base_id();
    SUASSERT (layerid >= 0, "");
    SUASSERT (layerid < (sutype::id_t)_layerIdToMinimalWireWidth.size(), "");
    
    sutype::dcoord_t width = _layerIdToMinimalWireWidth[layerid];

    return width;
    
  } // end of suMetalTemplateManager::get_minimal_wire_width
  
  // ------------------------------------------------------------
  // -
  // --- Private static methods
  // -
  // ------------------------------------------------------------
  
  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------

  //
  void suMetalTemplateManager::get_best_matching_metal_template_instance_ (const suLayer * wirelayer,
                                                                           const sutype::dcoord_t wireedgel,
                                                                           const sutype::dcoord_t wireedgeh,
                                                                           const sutype::dcoord_t wiresidel,
                                                                           const sutype::dcoord_t wiresideh,
                                                                           const suMetalTemplateInstance * & bestMti,
                                                                           const suLayer * & bestLayer,
                                                                           sutype::dcoord_t & bestSideL,
                                                                           sutype::dcoord_t & bestSideH,
                                                                           sutype::dcoord_t & bestDiff)
    const
  {
    SUASSERT (wirelayer->is (sutype::lt_wire), "");
    SUASSERT (!wirelayer->is (sutype::lt_via), "");
    SUASSERT (bestMti == 0, "");
    SUASSERT (bestLayer == 0, "");
    SUASSERT (bestSideL == 0, "");
    SUASSERT (bestSideH == 0, "");
    SUASSERT (bestDiff == 0, "");

    // check feasibility
    bool foundmti = false;
    
    for (const auto & iter1 : _allMetalTemplateInstances) {
      
      const suMetalTemplateInstance * mti = iter1;      
      const suMetalTemplate * mt = mti->metalTemplate();
      
      if (wirelayer->base() != mt->baseLayer()) continue;
      foundmti = true;
      break;
    }

    SUASSERT (foundmti, "Layer " << wirelayer->base()->to_str() << " has zero metal template instances. The router doesn't know how to route this layer. Aborted.");
    
    for (const auto & iter1 : _allMetalTemplateInstances) {
      
      const suMetalTemplateInstance * mti = iter1;

      get_best_matching_metal_template_instance_ (mti,
                                                  wirelayer,
                                                  wireedgel,
                                                  wireedgeh,
                                                  wiresidel,
                                                  wiresideh,
                                                  bestMti,
                                                  bestLayer,
                                                  bestSideL,
                                                  bestSideH,
                                                  bestDiff);
    }
    
  } // end of suMetalTemplateManager::get_best_matching_metal_template_instance_

  //
  void suMetalTemplateManager::get_best_matching_metal_template_instance_ (const suMetalTemplateInstance * mti,
                                                                           const suLayer * wirelayer,
                                                                           const sutype::dcoord_t wireedgel,
                                                                           const sutype::dcoord_t wireedgeh,
                                                                           const sutype::dcoord_t wiresidel,
                                                                           const sutype::dcoord_t wiresideh,
                                                                           const suMetalTemplateInstance * & bestMti,
                                                                           const suLayer * & bestLayer,
                                                                           sutype::dcoord_t & bestSideL,
                                                                           sutype::dcoord_t & bestSideH,
                                                                           sutype::dcoord_t & bestDiff)
    const
  {
    SUASSERT (mti, "");

    const suMetalTemplate * mt = mti->metalTemplate();
    if (wirelayer->base() != mt->baseLayer()) return;

    if (!mti->wire_edgel_edgeh_is_inside_region (wireedgel, wireedgeh)) return;
    if (!mti->wire_sidel_sideh_is_inside_region (wiresidel, wiresideh)) return;
    
    const sutype::dcoord_t absregionsidel = wiresidel;
    const sutype::dcoord_t absregionsideh = wiresideh;

    const sutype::dcoord_t refregionsidel = mti->move_coord_inside_first_metal_template_instance (absregionsidel);
    const sutype::dcoord_t shift = (absregionsidel - refregionsidel);
    SUASSERT (shift % mti->metalTemplate()->pitch() == 0, "");
      
    const sutype::dcoord_t firsttracksidel = shift + mti->get_first_track_sidel();
      
    const sutype::layers_tc & layers = mti->metalTemplate()->get_layers (mti->isInverted());
    const sutype::dcoords_t & widths = mti->metalTemplate()->get_widths (mti->isInverted());
    const sutype::dcoords_t & spaces = mti->metalTemplate()->get_spaces (mti->isInverted());
      
    SUASSERT (!layers.empty(), "");
    SUASSERT (!widths.empty(), "");
    SUASSERT (!spaces.empty(), "");
    SUASSERT (widths.size() == spaces.size(), "");
    SUASSERT (widths.size() == layers.size(), "");

    sutype::dcoord_t prevtracksidel = firsttracksidel;
    
    do {

      bool repeat = true;

      // do not count the last wire; it must be equal to the first
      for (sutype::uvi_t i=0; i < widths.size(); ++i) {
        
        const suLayer *        layer = layers[i];
        const sutype::dcoord_t width = widths[i];
        const sutype::dcoord_t space = spaces[i];
        
        SUASSERT (layer, "");
        SUASSERT (width > 0, "");
        SUASSERT (space > 0, "");
        
        const sutype::dcoord_t tracksidel = prevtracksidel;
        const sutype::dcoord_t tracksideh = tracksidel + width;
          
        // time to increment
        prevtracksidel = tracksideh + space;

        // track coords exceeded wire coords
        // the next track will be even farther
        if (tracksidel > absregionsideh) {
          repeat = false;
          break;
        }
          
        // track is (still) out of the region
        if (!mti->wire_sidel_sideh_is_inside_region (tracksidel, tracksideh)) {
          continue;
        }
          
        sutype::dcoord_t diff = abs (tracksidel - wiresidel) + abs (tracksideh - wiresideh);
        if (bestMti == 0 || diff < bestDiff) {
          bestMti = mti;
          bestLayer = layer;
          bestSideL = tracksidel;
          bestSideH = tracksideh;
          bestDiff = diff;
        }
      }

      if (!repeat) break;

    } while (true);
    
  } // end of suMetalTemplateManager::get_best_matching_metal_template_instance_

} // end of namespace amsr

// end of suMetalTemplateManager.cpp
