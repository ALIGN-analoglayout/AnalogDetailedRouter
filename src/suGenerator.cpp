// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Mon Oct 30 13:07:26 2017

//! \file   suGenerator.cpp
//! \brief  A collection of methods of the class suGenerator.

// std includes
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suLayer.h>
#include <suStatic.h>
#include <suWire.h>
#include <suWireManager.h>

// module include
#include <suGenerator.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  //
  sutype::id_t suGenerator::_uniqueId = 0;
  

  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  //
  suGenerator::~suGenerator ()
  {

    for (int i=0; i < (int)sutype::vgl_num_types; ++i) {
      sutype::wires_t & wires = _wires[i];
      for (const auto & iter : wires) {
        suWireManager::instance()->release_wire (iter);
      }
    }
    
  } // end of suGenerator::~suGenerator


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

  //
  void suGenerator::add_wire (suWire * wire)
  {
    SUASSERT (wire->net() == 0, "");

    const suLayer * layer = wire->layer();
    SUASSERT (layer, "");

    
    for (int i=0; i < (int)sutype::vgl_num_types; ++i) {
      
      if (_layers[i] == layer) {
        _wires[i].push_back (wire);
        return;
      }
    }

    SUASSERT (false, "Can't find layer for wire " << wire->to_str());
    
  } // end of suGenerator::add_wire

  //
  void suGenerator::set_layer (sutype::via_generator_layer_t vgl,
                               const suLayer * layer)
  {
    SUASSERT (vgl == sutype::vgl_cut || vgl == sutype::vgl_layer1 || vgl == sutype::vgl_layer2, "");
    SUASSERT (layer, "");
    
    _layers [vgl] = layer;

    if (vgl == sutype::vgl_cut)    { SUASSERT (layer->is (sutype::lt_via), ""); }
    if (vgl == sutype::vgl_layer1) { SUASSERT (layer->is (sutype::lt_wire), ""); }
    if (vgl == sutype::vgl_layer2) { SUASSERT (layer->is (sutype::lt_wire), ""); }
    
  } // end of set_layer

  //
  void suGenerator::make_canonical ()
  {
    for (int i=0; i < (int)sutype::vgl_num_types; ++i) {

      sutype::via_generator_layer_t vgl = (sutype::via_generator_layer_t)i;

      const suLayer * layer = _layers[i];
      SUASSERT (layer, "");

      if        (vgl == sutype::vgl_cut)    { SUASSERT (layer->is (sutype::lt_via), "");
      } else if (vgl == sutype::vgl_layer1) { SUASSERT (layer->is (sutype::lt_wire), "");
      } else if (vgl == sutype::vgl_layer2) { SUASSERT (layer->is (sutype::lt_wire), "");
      } else {
        SUASSERT (false, "");
      }

      if (vgl == sutype::vgl_cut) {
        SUASSERT (_widths[i].empty(), "");
      }

      SUASSERT (!_wires[i].empty(), "");

      // no widths is OK; it means that the generator matches any width      
      //SUASSERT (vgl == sutype::vgl_cut || !_widths[i].empty(), "");

      std::sort (_widths[i].begin(), _widths[i].end());
    }

    for (int i=0; i < (int)sutype::vgl_num_types; ++i) {
      for (int k=i+1; k < (int)sutype::vgl_num_types; ++k) {
        SUASSERT (_layers[i] != _layers[k], "");
      }
    }

    // change the order
    if (_layers[sutype::vgl_layer1]->level() > _layers[sutype::vgl_layer2]->level()) {

      const suLayer * tmplayer = _layers [sutype::vgl_layer1];
      _layers [sutype::vgl_layer1] = _layers [sutype::vgl_layer2];
      _layers [sutype::vgl_layer2] = tmplayer;

      sutype::wires_t tmpwires = _wires [sutype::vgl_layer1];
      _wires [sutype::vgl_layer1] = _wires [sutype::vgl_layer2];
      _wires [sutype::vgl_layer2] = tmpwires;

      sutype::dcoords_t tmpwidths = _widths [sutype::vgl_layer1];
      _widths [sutype::vgl_layer1] = _widths [sutype::vgl_layer2];
      _widths [sutype::vgl_layer2] = tmpwidths;
    }

    SUASSERT (_layers[sutype::vgl_layer1]->level() < _layers[sutype::vgl_cut]->level(), "unexpected levels; " << name() << "; encl=" << _layers[sutype::vgl_layer1]->to_str() << "; cut=" << _layers[sutype::vgl_cut]->to_str());
    SUASSERT (_layers[sutype::vgl_cut]->level() < _layers[sutype::vgl_layer2]->level(), "unexpected levels; " << name() << "; encl=" << _layers[sutype::vgl_layer2]->to_str() << "; cut=" << _layers[sutype::vgl_cut]->to_str());
    
  } // end of suGenerator::make_canonical

  //
  bool suGenerator::matches (const suLayer * layer1,
                             const suLayer * layer2,
                             sutype::dcoord_t width1,
                             sutype::dcoord_t width2)
    const
  {
    const bool np = false;

    SUINFO(np)
      << "Check generator " << name() << " for"
      << ": layer1=" << layer1->to_str()
      << "; layer2=" << layer2->to_str()
      << "; width1=" << width1
      << "; width2=" << width2
      << std::endl;

    SUINFO(np) << "  cutlayer=" << _layers [sutype::vgl_cut]->to_str() << std::endl;
    SUINFO(np) << "  layer1=" << _layers [sutype::vgl_layer1]->to_str() << std::endl;
    SUINFO(np) << "  layer2=" << _layers [sutype::vgl_layer2]->to_str() << std::endl;
    SUINFO(np) << "  wires[0]=" << suStatic::to_str (_wires[0]) << std::endl;
    SUINFO(np) << "  wires[1]=" << suStatic::to_str (_wires[1]) << std::endl;
    SUINFO(np) << "  wires[2]=" << suStatic::to_str (_wires[2]) << std::endl;
    SUINFO(np) << "  widths[1]=" << suStatic::to_str (_widths[1]) << std::endl;
    SUINFO(np) << "  widths[2]=" << suStatic::to_str (_widths[2]) << std::endl;

    for (int mode = 1; mode <= 2; ++mode) {

      sutype::via_generator_layer_t vgl1 = (mode == 1) ? sutype::vgl_layer1 : sutype::vgl_layer2;
      sutype::via_generator_layer_t vgl2 = (mode == 1) ? sutype::vgl_layer2 : sutype::vgl_layer1;

      if (layer1 != _layers [vgl1]) continue;
      if (layer2 != _layers [vgl2]) continue;

      // no widths is OK; it means that the generator matches any width
      if (!_widths[vgl1].empty() && std::find (_widths[vgl1].begin(), _widths[vgl1].end(), width1) == _widths[vgl1].end()) continue;
      if (!_widths[vgl2].empty() && std::find (_widths[vgl2].begin(), _widths[vgl2].end(), width2) == _widths[vgl2].end()) continue;

      // via enlosure is larger that wire width
      for (const auto & iter : _wires[vgl1]) {
        const suWire * giwire = iter;
        if (giwire->width() > width1) {
          SUASSERT (false, "Unexpected wire width " << giwire->width() << ". Allowed width " << width1 << " of layer " << layer1->to_str() << " must be removed during reading of car file. " << name());
        }
      }
      
      // via enlosure is larger that wire width
      for (const auto & iter : _wires[vgl2]) {
        const suWire * giwire = iter;
        if (giwire->width() > width2) {
          SUASSERT (false, "Unexpected wire width " << giwire->width() << ". Allowed width " << width2 << " of layer " << layer2->to_str() << " must be removed during reading of car file. " << name());
        }
      }

      // debug
      // try to find exactly the same width of wires
      if (0) {

        SUINFO(np) << "  Preliminary matches." << std::endl;
        
        bool widthExactlyMatchesOneOfWires1 = false;
        bool widthExactlyMatchesOneOfWires2 = false;

        for (const auto & iter : _wires[vgl1]) {
          const suWire * wire = iter;
          if (wire->width() == width1) {
            widthExactlyMatchesOneOfWires1 = true;
            break;
          }
        }

        for (const auto & iter : _wires[vgl2]) {
          const suWire * wire = iter;
          if (wire->width() == width2) {
            widthExactlyMatchesOneOfWires2 = true;
            break;
          }
        }

        if (!widthExactlyMatchesOneOfWires1) continue;
        if (!widthExactlyMatchesOneOfWires2) continue;
      }
      // end of debug

      SUINFO(np) << "MATCHES" << std::endl;
      return true;
    }
    
    SUINFO(np) << "DOES NOT MATCH" << std::endl;
    return false;
    
  } // end of suGenerator::matches

  //
  void suGenerator::clone_wires (sutype::wires_t & clonedwires,
                                 const suNet * net,
                                 sutype::dcoord_t dx,
                                 sutype::dcoord_t dy,
                                 sutype::wire_type_t cutwiretype)
                                 
    const
  {
    SUASSERT (cutwiretype == sutype::wt_preroute || cutwiretype == sutype::wt_route, "");
    
    for (int i=0; i < (int)sutype::vgl_num_types; ++i) {
      
      const sutype::wires_t & wires = _wires[i];
      sutype::via_generator_layer_t vgl = (sutype::via_generator_layer_t)i;

      sutype::wire_type_t wiretype = sutype::wt_undefined;

      if      (vgl == sutype::vgl_cut)    wiretype = cutwiretype;
      else if (vgl == sutype::vgl_layer1) wiretype = sutype::wt_enclosure;
      else if (vgl == sutype::vgl_layer2) wiretype = sutype::wt_enclosure;
      else {
        SUASSERT (false, "");
      }
      
      for (const auto & iter : wires) {
        
        const suWire * wire = iter;

        clonedwires.push_back (suWireManager::instance()->create_wire_from_wire_then_shift (net, wire, dx, dy, wiretype));
      }
    }
    
  } // end of suGenerator::clone_wires

  //
  suRectangle suGenerator::get_shape (sutype::dcoord_t dx,
                                      sutype::dcoord_t dy,
                                      sutype::via_generator_layer_t vgl)
    const
  {
    const suWire * wire = get_wire_ (vgl);
    
    suRectangle rect;
    
    rect.copy (wire->rect());
    
    rect.shift (dx, dy);
    
    return rect;
    
  } // end of suGenerator::get_shape

  //
  sutype::dcoord_t suGenerator::get_shape_width (sutype::via_generator_layer_t vgl)
    const
  {
    const suWire * wire = get_wire_ (vgl);

    return wire->rect().w();
    
  } // end of suGenerator::get_shape_width

  //
  sutype::dcoord_t suGenerator::get_shape_height (sutype::via_generator_layer_t vgl)
    const
  {
    const suWire * wire = get_wire_ (vgl);

    return wire->rect().h();
    
  } // end of suGenerator::get_shape_height
  
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
  const suWire * suGenerator::get_wire_ (sutype::via_generator_layer_t vgl)
    const
  {
    sutype::svi_t i = (sutype::svi_t)vgl;
    
    SUASSERT (i >= 0 && i < (int)_wires.size(), "");
    
    const sutype::wires_t & wires = _wires[i];
    SUASSERT (wires.size() == 1, "");
    
    const suWire * wire = wires.front();

    return wire;
    
  } // end of suGenerator::get_wire_


} // end of namespace amsr

// end of suGenerator.cpp
