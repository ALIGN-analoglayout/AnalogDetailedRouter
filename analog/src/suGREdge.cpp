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
//! \date   Wed May 15 13:41:20 PDT 2019

//! \file   suGREdge.cpp
//! \brief  A collection of methods of the class suGREdge.

// std includes
#include <algorithm>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suLayer.h>
#include <suLayerManager.h>
#include <suMetalTemplate.h>
#include <suMetalTemplateInstance.h>
#include <suMetalTemplateManager.h>
#include <suWireManager.h>

// module include
#include <suGREdge.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------


  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------


  // ------------------------------------------------------------
  // -
  // --- Public static methods
  // -
  // ------------------------------------------------------------


  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------
  
  // very simple mode for now
  void suGREdge::calculate_capacity ()
  {
    _capacity = 0;
    
    const sutype::metaltemplateinstances_t & mtis = suMetalTemplateManager::instance()->allMetalTemplateInstances();

    const suLayer * lowestGRLayer = suLayerManager::instance()->lowestGRLayer();

    const suNet * net = 0;
    
    for (sutype::uvi_t m=0; m < mtis.size(); ++m) {
      
        const suMetalTemplateInstance * mti = mtis[m];
        const suMetalTemplate * mt = mti->metalTemplate();
        const suLayer * baselayer = mt->baseLayer();

        if (!baselayer->is (sutype::lt_metal)) continue;

        if (baselayer->level() < lowestGRLayer->level()) continue;
        
        if (is_ver() && !baselayer->is_hor()) continue;
        if (is_hor() && !baselayer->is_ver()) continue;
        
        sutype::wires_t trunkwires;
        mti->create_wires_in_rectangle (_rect,
                                        net,
                                        sutype::wt_trunk,
                                        false, // discretizeWires
                                        trunkwires); // trunk wires

        for (const auto & iter : trunkwires) {

          suWire * wire = iter;
          suWireManager::instance()->release_wire (wire);
          
          _capacity += 10;
        }
    }
    
  } // end of suGREdge::calculate_capacity

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


} // end of namespace amsr

// end of suGREdge.cpp
