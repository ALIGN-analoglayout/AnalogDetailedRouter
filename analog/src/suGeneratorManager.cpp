//
//
//        INTEL CONFIDENTIAL - INTERNAL USE ONLY
//
//         Copyright by Intel Corporation, 2017
//                 All rights reserved.
//         Copyright does not imply publication.
//
//
//! \since  Analog/Mixed Signal Router (prototype); AMSR 0.00
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Thu Oct 26 12:52:43 2017

//! \file   suGeneratorManager.cpp
//! \brief  A collection of methods of the class suGeneratorManager.

// std includes
#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suGenerator.h>
#include <suLayer.h>
#include <suLayerManager.h>
#include <suRuleManager.h>
#include <suStatic.h>
#include <suToken.h>
#include <suTokenParser.h>
#include <suWire.h>
#include <suWireManager.h>

// module include
#include <suGeneratorManager.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  suGeneratorManager * suGeneratorManager::_instance = 0;


  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  suGeneratorManager::~suGeneratorManager ()
  {
    for (const auto & iter : _allgenerators) {
      delete iter;
    }
    
  } // end of suGeneratorManager::~suGeneratorManager


  // ------------------------------------------------------------
  // -
  // --- Public static methods
  // -
  // ------------------------------------------------------------

  // static
  void suGeneratorManager::delete_instance ()
  {
    if (suGeneratorManager::_instance)
      delete suGeneratorManager::_instance;
  
    suGeneratorManager::_instance = 0;
    
  } // end of suGeneratorManager::delete_instance


  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  //
  void suGeneratorManager::read_generator_file (const std::string & filename)
  {
    suTokenParser tokenparser;

    bool ok = tokenparser.read_file (filename);
    SUASSERT (ok, "");

    sutype::tokens_t tokens = tokenparser.rootToken()->get_tokens ("Generator");
    if (tokens.empty()) return;

    SUINFO(1) << "Found " << tokens.size() << " potential generators in file " << filename << std::endl;

    sutype::strings_t tokennames;
    sutype::layers_tc layers;

    SUASSERT (sutype::vgl_cut    == 0, "");
    SUASSERT (sutype::vgl_layer1 == 1, "");
    SUASSERT (sutype::vgl_layer2 == 2, "");

    tokennames.push_back ("cutlayer");
    tokennames.push_back ("Layer1");
    tokennames.push_back ("Layer2");

    layers.push_back (0);
    layers.push_back (0);
    layers.push_back (0);

    // stats
    std::set<std::string> skippedlayers;
    unsigned numskippedgenerators = 0;
    unsigned numcreatedgenerators = 0;
    
    for (sutype::uvi_t i=0; i < tokens.size(); ++i) {
      
      const suToken * token = tokens[i];

      const std::string & name = token->get_string_value ("name");

      bool skipThisGenerator = false;

      layers [sutype::vgl_cut]    = 0;
      layers [sutype::vgl_layer1] = 0;
      layers [sutype::vgl_layer2] = 0;
      
      for (int mode = 0; mode < 3; ++mode) {

        const std::string & tokenname = tokennames[mode];
        sutype::tokens_t tokens2 = token->get_tokens (tokenname);
        SUASSERT (tokens2.size() == 1, "");
        const std::string & layername = tokens2.front()->get_string_value ("value");
        const suLayer * layer = suLayerManager::instance()->get_colored_layer_by_name (layername);
        if (layer == 0) {
          skippedlayers.insert (layername);
          skipThisGenerator = true;
        }
        layers [mode] = layer;
      }

      // double check an optional parameter viaCentersOn      
      // disabled for a while
      if (0 && !skipThisGenerator) {
        
        sutype::tokens_t tokens2 = token->get_tokens ("viaCentersOn");
        SUASSERT (tokens2.size() == 1, "Unexpected format of a generator " << name << "; file " << filename);

        sutype::strings_t layernames = tokens2.front()->get_list_of_strings ("value");
        if (layernames.size() != 2) {
          SUISSUE("Skipped a bridge via for a while") << ": " << name << "; file " << filename << std::endl;
          skipThisGenerator = true;
        }
        else {
          SUASSERT (layernames.size() == 2, "Unexpected format of a generator " << name << "; file " << filename);
          SUASSERT (layernames.front().compare (layers[sutype::vgl_layer1]->name()) == 0, "Unexpected format of a generator " << name << "; file " << filename << "; " << suStatic::to_str (layernames));
          SUASSERT (layernames.back() .compare (layers[sutype::vgl_layer2]->name()) == 0, "Unexpected format of a generator " << name << "; file " << filename << "; " << suStatic::to_str (layernames));
        }
      }

      if (!skipThisGenerator) {

        sutype::tokens_t tokens2 = token->get_tokens ("OneEnclosure");
        if (!tokens2.empty()) {
          SUISSUE("Skipped a bridge via for a while") << ": " << name << "; file " << filename << std::endl;
          skipThisGenerator = true;
        }
      }

      if (!skipThisGenerator) {

        const suLayer * cutlayer = layers[sutype::vgl_cut];
        if (cutlayer->is_base() && !cutlayer->coloredLayers().empty()) {
          SUISSUE("Skipped a via with a non-colored cut layer") << ": via " << name << "; layer " << cutlayer->to_str() << "; file " << filename << std::endl;
          skipThisGenerator = true;
        }
      }
      
      if (skipThisGenerator) {
        ++numskippedgenerators;
        continue;
      }

      sutype::tokens_t cwts = token->get_tokens("CutWidth");
      sutype::tokens_t chts = token->get_tokens("CutHeight");

      SUASSERT (!cwts.empty(), "No CutWidth found");
      SUASSERT (!chts.empty(), "No CutHeight found");
      
      sutype::dcoord_t cutwidth  = suStatic::micron_to_udm (cwts.front()->get_float_value("value"));
      sutype::dcoord_t cutheight = suStatic::micron_to_udm (chts.front()->get_float_value("value"));

      SUASSERT (cutwidth % 10 == 0, "Unexpected format of a generator " << name << "; file " << filename);
      SUASSERT (cutheight % 10 == 0, "Unexpected format of a generator " << name << "; file " << filename);

      SUASSERT (cutwidth > 0, "");
      SUASSERT (cutheight > 0, "");
      
      sutype::dcoord_t cutxl = -cutwidth / 2;
      sutype::dcoord_t cutxh = cutxl + cutwidth;
      sutype::dcoord_t cutyl = -cutheight / 2;
      sutype::dcoord_t cutyh = cutyl + cutheight;

      SUASSERT (cutxl < cutxh, "");
      SUASSERT (cutyl < cutyh, "");

      suGenerator * generator = new suGenerator (name);

      for (int k = 0; k < 3; ++k) {
        sutype::via_generator_layer_t vgl = (sutype::via_generator_layer_t)k;
        generator->set_layer (vgl, layers[k]);
      }

      const suLayer * cutlayer = layers[sutype::vgl_cut];
      
      generator->add_wire (suWireManager::instance()->create_wire_from_dcoords ((const suNet *)0, layers[sutype::vgl_cut], cutxl, cutyl, cutxh, cutyh, sutype::wt_cut));
      
      // landing and enclosure wires
      for (int mode = 1; mode <= 2; ++mode) {
        
        sutype::via_generator_layer_t vgl = (sutype::via_generator_layer_t)mode;
        SUASSERT (vgl == sutype::vgl_layer1 || vgl == sutype::vgl_layer2, "");
       
        const suToken * token1 = token->get_tokens(tokennames[mode]).front();
        sutype::dcoord_t x_coverage = 0;
        sutype::dcoord_t y_coverage = 0;
        
        if (1) {
          sutype::tokens_t tokens2 = token1->get_tokens ("x_coverage");
          SUASSERT (tokens2.size() == 1, "Unexpected format of a generator " << name << "; file " << filename);
          x_coverage = suStatic::micron_to_udm (tokens2.front()->get_float_value ("value"));
        }
        if (1) {
          sutype::tokens_t tokens2 = token1->get_tokens ("y_coverage");
          SUASSERT (tokens2.size() == 1, "Unexpected format of a generator " << name << "; file " << filename);
          y_coverage = suStatic::micron_to_udm (tokens2.front()->get_float_value ("value"));
        }

        sutype::dcoord_t wirexl = cutxl - x_coverage;
        sutype::dcoord_t wirexh = cutxh + x_coverage;
        sutype::dcoord_t wireyl = cutyl - y_coverage;
        sutype::dcoord_t wireyh = cutyh + y_coverage;

        const suLayer * encllayer = layers[mode];
        
        // custom enclosures for vias
        if (suRuleManager::instance()->rule_is_defined (sutype::rt_minencl, cutlayer, encllayer)) {
          
          sutype::dcoord_t minencl = suRuleManager::instance()->get_rule_value (sutype::rt_minencl, cutlayer, encllayer);
          
          if (encllayer->pgd() == sutype::go_ver) {
            wireyl = std::min (wireyl, cutyl - minencl);
            wireyh = std::max (wireyh, cutyh + minencl);
          }
          else if (encllayer->pgd() == sutype::go_hor) {
            wirexl = std::min (wirexl, cutxl - minencl);
            wirexh = std::max (wirexh, cutxh + minencl);
          }
          else {
            SUASSERT (false, "");
          }
        }
        //
        
        suWire * wireenclosure = suWireManager::instance()->create_wire_from_dcoords ((const suNet *)0, encllayer, wirexl, wireyl, wirexh, wireyh, sutype::wt_enclosure);

        generator->add_wire (wireenclosure);

        sutype::dcoord_t wireenclosurewidth = wireenclosure->width();
        
        if (1) {          
          sutype::tokens_t tokens2 = token1->get_tokens ("widths");
          SUASSERT (tokens2.empty() || tokens2.size() == 1, "Unexpected format of a generator " << name << "; file " << filename);

          std::vector<float> values;
          if (!tokens2.empty()) {
            values = tokens2.front()->get_list_of_floats ("value");
          }
          
          for (const auto & iter : values) {
            float widthum = iter;
            sutype::dcoord_t width = suStatic::micron_to_udm (widthum);
            SUASSERT (width > 0, "Unexpected format of a generator " << name << "; file " << filename << "; widthum=" << widthum);
            //SUINFO(1) << "generator " << name << "; widthum=" << widthum << "; width=" << width << std::endl;
            if (width < wireenclosurewidth) {
              SUISSUE("Unexpected allowed value of generator wire width")
                << ": Generator " << generator->name()
                << " has enclosure " << wireenclosure->to_str()
                << " that cannot match allowed wire width " << width
                << ". Width skipped."
                << std::endl;
              continue;
            }
            generator->add_width (vgl, width);
          }
          
          if (!values.empty() && generator->get_widths(vgl).empty()) {
            SUISSUE("Generator has no allowed widths for one of enclosure layers")
              << ": Generator " << generator->name()
              << " has no allowed widths for layer " << layers[mode]->name() << "."
              << " Very unusual."
              << std::endl;
            skipThisGenerator = true;
          }
        }
      }

      if (skipThisGenerator) {
        SUISSUE("Skipped a bad generator")
          << ": Generator " << generator->name()
          << " was skipped. Found some issues. Read messages above."
          << std::endl;
        
        delete generator;
        ++numskippedgenerators;
        continue;
      }

      generator->make_canonical ();
      
      add_generator_ (generator);

      ++numcreatedgenerators;
    }
    
    if (!skippedlayers.empty()) {
      SUINFO(1) << "Skipped generators: " << numskippedgenerators << " generators of layers: " << suStatic::to_str (skippedlayers) << std::endl;
    }
    
    if (numcreatedgenerators) {
      SUINFO(1) << "Created " << numcreatedgenerators << " generators from file " << filename << std::endl;
    }
    
  } // end of read_generator_file

  //
  const sutype::generators_tc & suGeneratorManager::get_generators (const suLayer * cutlayer)
    const
  {
    sutype::id_t id = cutlayer->pers_id();
    SUASSERT (id >= 0, "");
    SUASSERT (id < (sutype::id_t)_generators.size(), "Can't get generators for cut layer " << cutlayer->name());
    
    return _generators[id];
    
  } // end of get_generators

  //
  const suGenerator * suGeneratorManager::get_generator (const suLayer * cutlayer,
                                                         sutype::dcoord_t cutw,
                                                         sutype::dcoord_t cuth)
    const
  {
    const sutype::generators_tc & generators = get_generators (cutlayer);

    for (const auto & iter : generators) {

      const suGenerator * generator = iter;
      
      if (generator->get_shape_width(sutype::vgl_cut) == cutw && generator->get_shape_height(sutype::vgl_cut) == cuth)
        return generator;
    }

    return 0;
    
  } // end of suGeneratorManager::get_generator
  
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
  void suGeneratorManager::add_generator_ (const suGenerator * generator)
  {    
    SUASSERT (generator, "");

    if (1) {
      
      for (const auto & iter : _allgenerators) {
        
        const suGenerator * generator1 = iter;
        if (generator1->name().compare (generator->name()) == 0) {
          SUISSUE("Generator name is not unique") << ": " << generator->name() << std::endl;
          SUASSERT (false, "Generator name is not unique: " << generator->name());
        }
      }
    }

    _allgenerators.push_back (generator);

    // mode 1: store generator by its actual cut layer
    // mode 2: store generator by its base cut layer
    for (int mode = 1; mode <= 2; ++mode) {
      
      const suLayer * cutlayer = generator->get_layer (sutype::vgl_cut);
      SUASSERT (cutlayer, "");
      
      // store to base during mode=2
      if (mode == 2) {
        if (cutlayer->is_base()) {
          continue;
        }
        cutlayer = cutlayer->base();
      }
      
      sutype::id_t id = cutlayer->pers_id();
      SUASSERT (id >= 0, "");
      
      if (id >= (sutype::id_t)_generators.size()) {
        _generators.resize (id + 1);
      }

      for (const auto & iter : _generators[id]) {
        const suGenerator * generator1 = iter;
        if (generator1->name().compare (generator->name()) == 0) {
          SUISSUE("Generator has been saved already") << ": " << generator->name() << " for this layer " << cutlayer->name() << "; mode=" << mode << std::endl;
          SUASSERT (false, "");
        }
      }

      SUINFO(0)
        << "Stored generator " << generator->name() << " for layer " << cutlayer->to_str()
        << "; mode=" << mode
        << "; original_cut: " << generator->get_layer (sutype::vgl_cut)->to_str()
        << "; original_layer1: " << generator->get_layer (sutype::vgl_layer1)->to_str()
        << "; original_layer2: " << generator->get_layer (sutype::vgl_layer2)->to_str()
        << std::endl;
      
      _generators [id].push_back (generator);
    }
    
  } // end of suGeneratorManager::add_generator_


} // end of namespace amsr

// end of suGeneratorManager.cpp
