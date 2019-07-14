// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Fri Oct 13 10:39:10 2017

//! \file   suGlobalRoute.cpp
//! \brief  A collection of methods of the class suGlobalRoute.

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
#include <suGlobalRouter.h>
#include <suLayer.h>
#include <suLayerManager.h>
#include <suNet.h>
#include <suRectangle.h>

// module include
#include <suGlobalRoute.h>

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

  // clone only net, layer, bbox
  suGlobalRoute * suGlobalRoute::create_light_clone ()
    const
  {
    suGlobalRoute * gr = new suGlobalRoute ();

    gr->_net   = _net;
    gr->_layer = _layer;
    gr->_bbox  = _bbox;
    
    return gr;
    
  } // end of suGlobalRoute::create_light_clone
       
  //
  sutype::regions_t suGlobalRoute::get_regions ()
    const
  {
    sutype::regions_t regions;

    SUASSERT (bbox().is_line(), "");

    for (sutype::dcoord_t x = bbox().xl(); x <= bbox().xh(); ++x) {
      for (sutype::dcoord_t y = bbox().yl(); y <= bbox().yh(); ++y) {

        suRegion * region = suGlobalRouter::instance()->get_region (x, y);
        SUASSERT (region, "Bad global route: " << to_str() << "; could not get region (" << x << "," << y << ")");
        regions.push_back (region);
      }
    }

    return regions;
    
  } // end of suGlobalRoute::get_regions

  //
  int suGlobalRoute::length_in_regions ()
    const
  {
    return bbox().length (_layer->pgd());
    
  } // end of suGlobalRoute::length_in_regions
  
  //
  int suGlobalRoute::width_in_regions ()
    const
  {
    return bbox().width (_layer->pgd());
    
  } // end of suGlobalRoute::width_in_regions
  
  //
  std::string suGlobalRoute::to_str ()
    const
  {
    std::ostringstream oss;

    oss << "(";

    if (_net) {
      oss << _net->name();
    }
    else {
      oss << "<null_net>";
    }
    oss << ",";

    oss
      <<        bbox().xl()
      << "," << bbox().yl()
      << "," << bbox().xh()
      << "," << bbox().yh();

    oss << ",";
    if (_layer) {
      oss << _layer->name();
    }
    else {
      SUASSERT (false, "");
      oss << "<null_layer>";
    }
    
    if (_minWireWidth > 0) {
      oss << ",minw=" << _minWireWidth;
    }

    if (_gid != sutype::UNDEFINED_GLOBAL_ID) {
      oss <<",gid=" << _gid;
    }

    if (_samewas != sutype::UNDEFINED_GLOBAL_ID) {
      oss <<",samewas=" << _samewas;
    }
    
    oss << ")";

    return oss.str();
    
  } // end of suGlobalRoute::to_str

  //
  bool suGlobalRoute::change_layer (bool upper)
  {
    const suLayer * layer0 = layer();
    
    SUASSERT (layer0, "");
    SUASSERT (layer0->has_pgd(), "");
    SUASSERT (layer0->is (sutype::lt_wire), "");
    SUASSERT (layer0->is (sutype::lt_metal), "");

    const suLayer * layer1 = suLayerManager::instance()->get_base_adjacent_layer (layer0, sutype::lt_wire | sutype::lt_metal, upper, 0);
    if (!layer1) return false;
    
    SUASSERT (layer1->is (sutype::lt_wire), "");
    SUASSERT (layer1->is (sutype::lt_metal), "");
    SUASSERT (layer1->has_pgd() && layer1->pgd() != layer0->pgd(), "layer0=" << layer0->to_str() << "; layer1=" << layer1->to_str());
    
    const suLayer * layer2 = suLayerManager::instance()->get_base_adjacent_layer (layer1, sutype::lt_wire | sutype::lt_metal, upper, 0);
    if (!layer2) return false;
    
    SUASSERT (layer2->is (sutype::lt_wire), "");
    SUASSERT (layer2->is (sutype::lt_metal), "");
    SUASSERT (layer2->has_pgd() && layer2->pgd() == layer0->pgd(), "layer0=" << layer0->to_str() << "; layer2=" << layer2->to_str());
    
    if (layer2->level() < suLayerManager::instance()->lowestGRLayer()->level()) return false;
    
    _layer = layer2;
    
//     SUISSUE("Changed layer for a global route")
//       << ": oldlayer=" << layer0->to_str()
//       << "; newlayer=" << layer2->to_str()
//       << "; gr=" << to_str()
//       << std::endl;
    
    return true;
    
  } // end of suGlobalRoute::change_layer
  
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

// end of suGlobalRoute.cpp
