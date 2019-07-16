// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Wed Oct 24 13:49:37 2018

//! \file   suSVI.cpp
//! \brief  A collection of methods of the class suSVI.

// std includes
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suGeneratorInstance.h>
#include <suCellManager.h>
#include <suCellMaster.h>
#include <suLayer.h>
#include <suLayerManager.h>
#include <suNet.h>
#include <suStatic.h>
#include <suWire.h>

// module include
#include <suSVI.h>

// other includes
#ifdef _ENABLE_SVI_

#include "svMadFox.h"

#endif // _ENABLE_SVI_

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  suSVI * suSVI::_instance = 0;

  // hardcoded colors
  sutype::strings_t suSVI::_mColors;
  sutype::strings_t suSVI::_vColors;
  
  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  //
  suSVI::~suSVI ()
  {
    
  } // end of suSVI::~suSVI

  // ------------------------------------------------------------
  // -
  // --- Public static methods
  // -
  // ------------------------------------------------------------

  // static
  void suSVI::delete_instance ()
  {
    if (suSVI::_instance)
      delete suSVI::_instance;
  
    suSVI::_instance = 0;
    
  } // end of suSVI::delete_instance


  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  //
  void suSVI::dump_xml_file (const std::string & filename)
    const
  {

#ifndef _ENABLE_SVI_

    return;

#else // _ENABLE_SVI_

    SUINFO(1) << "Dump XML file: " << filename << std::endl;
    
    _SVI_MANAGER_ * svmanager = new _SVI_MANAGER_ (filename);
    
    prepare_header_ (svmanager);

    svmanager->begin();

    add_wires_ (svmanager);

    svmanager->finish();

    delete svmanager;

    SUINFO(1) << "Written " << filename << std::endl;
    
#endif // _ENABLE_SVI_

  } // end of suSVI::dump_xml_fil

  // ------------------------------------------------------------
  // -
  // --- Private static methods
  // -
  // ------------------------------------------------------------

  // static
  void suSVI::init_static_vars_ ()
  {
    if (!suSVI::_mColors.empty()) return;

    suSVI::_mColors.push_back ("black");  // m0
    suSVI::_mColors.push_back ("red");    // m1
    suSVI::_mColors.push_back ("blue");   // m2
    suSVI::_mColors.push_back ("green");  // m3
    suSVI::_mColors.push_back ("orange"); // m4
    suSVI::_mColors.push_back ("purple"); // m5
    suSVI::_mColors.push_back ("orange"); // m6

    suSVI::_vColors.push_back ("lightBlue"); // v0
    suSVI::_vColors.push_back ("cyan");      // v1
    suSVI::_vColors.push_back ("lightGray"); // v2
    suSVI::_vColors.push_back ("magenta");   // v3
    suSVI::_vColors.push_back ("pink");      // v4
    suSVI::_vColors.push_back ("darkGray");  // v5
    
  } // end of suSVI::init_static_vars_

  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------

  //
  void suSVI::prepare_header_ (_SVI_MANAGER_ * svmanager)
    const
  {

#ifdef _ENABLE_SVI_

    bool                    visible      = true;
    //bool                    invisible    = false;
    //bool                    spectrum     = true;
    bool                    non_spectrum = false;
    //madfox::svConst::type_t no_type      = madfox::svConst::NO_TYPE;
    madfox::svConst::type_t integer      = madfox::svConst::INTEGER;
    //madfox::svConst::type_t real         = madfox::svConst::REAL;

    svmanager->addGroup ("Nets");
    svmanager->addGroup ("Layers");
    
    const sutype::layers_tc & layers = suLayerManager::instance()->layers();

    for (const auto & iter0 : layers) {

      const suLayer * layer = iter0;

      const std::string & schemename = layer->name();

      std::string colorstr = auto_detect_color_for_layer_ (layer);
      madfox::svConst::color_t color = madfox::svStatic::str2color (colorstr);
      
      svmanager->addScheme (schemename, madfox::svConst::SINGLE, color);

      svmanager->addParameter (layer->name(), non_spectrum, integer, "Layers", visible, schemename); 
    }

    const suCellMaster * cellmaster = suCellManager::instance()->topCellMaster();
    
    const sutype::nets_t & nets = cellmaster->nets();

    for (sutype::uvi_t i=0; i < nets.size(); ++i) {
      
      const suNet * net = nets[i];

      svmanager->addParameter (net->name(), non_spectrum, integer, "Nets", visible, "netcs");
    }
    
#endif // _ENABLE_SVI_
    
  } // end of suSVI::prepare_header_

  //
  void suSVI::add_wires_ (_SVI_MANAGER_ * svmanager)
    const
  {

#ifdef _ENABLE_SVI_

    const suCellMaster * cellmaster = suCellManager::instance()->topCellMaster();

    const sutype::nets_t & nets = cellmaster->nets();

    std::map<const suLayer *, sutype::wires_t, suLayer::cmp_const_ptr_by_level> wiresPerLayers;

    for (sutype::uvi_t i=0; i < nets.size(); ++i) {
      
      const suNet * net = nets[i];
      const sutype::wires_t & wires = net->wires ();
      const sutype::generatorinstances_t & generatorinstances = net->generatorinstances ();

      for (const auto & iter1 : generatorinstances) {

        const suGeneratorInstance * gi = iter1;
        
        const sutype::wires_t & wires = gi->wires();
        
        for (const auto & iter2 : wires) {

          suWire * wire = iter2;
          if (!wire->layer()->is (sutype::lt_via)) continue;

          wiresPerLayers[wire->layer()].push_back (wire);
        }
      }

      for (const auto & iter1 : wires) {
        
        suWire * wire = iter1;
        wiresPerLayers[wire->layer()].push_back (wire);
      }
    }

    for (const auto & iter0 : wiresPerLayers) {

      const auto & container1 = iter0.second;

      for (const auto & iter1 : container1) {

        const suWire * wire = iter1;

        add_wire_ (svmanager, wire);
      }
    }
    
//     for (sutype::uvi_t i=0; i < nets.size(); ++i) {
      
//       const suNet * net = nets[i];
//       const sutype::wires_t & wires = net->wires ();
//       const sutype::generatorinstances_t & generatorinstances = net->generatorinstances ();

//       for (const auto & iter1 : generatorinstances) {

//         const suGeneratorInstance * gi = iter1;

//         const sutype::wires_t & wires = gi->wires();

//         for (const auto & iter2 : wires) {

//           const suWire * wire = iter2;

//           add_wire_ (svmanager, wire);          
//         }
//       }

//       for (const auto & iter1 : wires) {

//         const suWire * wire = iter1;

//         add_wire_ (svmanager, wire);
//       }
//     }

#endif // _ENABLE_SVI_
    
  } // end of suSVI::add_wires_

  //
  void suSVI::add_wire_ (_SVI_MANAGER_ * svmanager,
                         const suWire * wire)
    const
  {

#ifdef _ENABLE_SVI_

    int xl = wire->rect().xl();
    int yl = wire->rect().yl();
    int xh = wire->rect().xh();
    int yh = wire->rect().yh();

    std::ostringstream msg;
    msg << "";

    std::vector<std::string> pars;
    std::vector<double>      vals;

    pars.push_back (wire->layer()->name());
    vals.push_back (0);

    pars.push_back (wire->net()->name());
    vals.push_back (0);
    
    svmanager->rectangle (xl, yl, (xh-xl), (yh-yl),
                          false, // no vertical symmetry
                          false, // no horizontal symmetry
                          msg.str(),
                          madfox::svConst::UNKNOWN_COLOR,
                          pars, vals);

#endif // _ENABLE_SVI_

  } // end of suSVI::add_wire_

  //
  std::string suSVI::auto_detect_color_for_layer_ (const suLayer * layer)
    const
  {
    SUASSERT (!suSVI::_mColors.empty(), "");
    SUASSERT (!suSVI::_vColors.empty(), "");
    
    const std::string & layername = layer->name();

    if (layer->is (sutype::lt_metal)) {

      for (sutype::uvi_t i = 0; i < suSVI::_mColors.size(); ++i) {

        std::string strindex = suStatic::dcoord_to_str (i);

        if (layername.find (strindex) == std::string::npos) continue;

        return suSVI::_mColors[i];
      }
    }

    if (layer->is (sutype::lt_via)) {
      
      for (sutype::uvi_t i = 0; i < suSVI::_vColors.size(); ++i) {
        
        std::string strindex = suStatic::dcoord_to_str (i);
        
        if (layername.find (strindex) == std::string::npos) continue;
        
        return suSVI::_vColors[i];
      }
    }

    SUISSUE("Could not detect color for layer") << ": " << layer->name() << std::endl;
    
    return "gray";
    
  } // end of suSVI::auto_detect_color_for_layer_

} // end of namespace amsr

// end of suSVI.cpp
