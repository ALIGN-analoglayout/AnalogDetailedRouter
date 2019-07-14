// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Mon Oct 16 13:21:31 2017

//! \file   suWireManager.cpp
//! \brief  A collection of methods of the class suWireManager.

// std includes
#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suLayer.h>
#include <suLayerManager.h>
#include <suNet.h>
#include <suSatRouter.h>
#include <suStatic.h>
#include <suWire.h>

// module include
#include <suWireManager.h>

namespace amsr
{  
  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  suWireManager * suWireManager::_instance = 0;


  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  //! destructor
  suWireManager::~suWireManager ()
  {
    SUINFO(1) << "Delete suWireManager and " << _wires.size() << " wires." << std::endl;
    
    sutype::uvi_t numunreleasedwires = 0;
    
    for (const auto & iter1 : _wiresPerLayers) {

      const auto & map1 = iter1;

      for (const auto & iter2 : map1) {

        const auto & map2 = iter2.second;

        for (const auto & iter3 : map2) {

          const auto & map3 = iter3.second;

          for (const auto & iter4 : map3) {

            const sutype::wires_t & wires = iter4.second;
            if (wires.empty()) continue;

            // Only suWireManager keeps illegal wires; I have to release such wires here
            sutype::wires_t wireshardcopy = wires;

            for (const auto & iter5 : wireshardcopy) {
              
              suWire * wire = iter5;
              if (wire_is_illegal (wire)) {
                SUASSERT (get_wire_usage (wire) == 1, ""); // if asserts most likely wire here is already dead
                release_wire (wire);
              }
            }
            //
            
            for (const auto & iter5 : wires) {

              suWire * wire = iter5;
              
              //SUINFO(1) << "Found unreleased wire: wire=" << wire->to_str() << std::endl;
              ++numunreleasedwires;
              wire->_net = 0;
            }
          }
        }
      }
    }

    if (numunreleasedwires > 0) {
      SUISSUE("Found unreleased wires") << ": " << numunreleasedwires << std::endl;
      //SUASSERT (false, "");
    }

    std::sort (_wires.begin(), _wires.end(), suStatic::compare_wires_by_id);

    for (sutype::uvi_t i=0; i < _wires.size(); ++i) {
      suWire * wire = _wires[i];
      SUASSERT (i == 0 || wire->id() > _wires[i-1]->id(), "");
    }

    sutype::uvi_t numLostWires = 0;

    for (sutype::uvi_t i=0, k=0; i < _numCreatedWires; ++i) {

      if (k < _wires.size()) {
        
        suWire * wire = _wires[k];
        
        if (wire->id() == (sutype::id_t)i) {
          delete wire;
          ++k;
          continue;
        }
      }
      
      SUISSUE("Wire has been lost") << ": wire id=" << i << " has been lost" << std::endl;
      ++numLostWires;
    }

    if (numLostWires != 0) {
      SUINFO(1) << "Lost " << numLostWires << " wires." << std::endl;
      SUASSERT (false, "");
    }
    
  } // end of suWireManager::~suWireManager


  // ------------------------------------------------------------
  // -
  // --- Public static methods
  // -
  // ------------------------------------------------------------

  // static
  void suWireManager::delete_instance ()
  {
    if (suWireManager::_instance)
      delete suWireManager::_instance;
  
    suWireManager::_instance = 0;
    
  } // end of suWireManager::delete_instance


  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  //
  sutype::svi_t suWireManager::get_wire_usage (const suWire * wire)
    const
  {
    SUASSERT (wire, "");

    const sutype::id_t id = wire->id();
    
    SUASSERT (id >= 0, "");
    SUASSERT (id < (sutype::id_t)_wireUsage.size(), "");
    
    return _wireUsage[id];
    
  } // end of suWireManager::get_wire_usage

  //
  bool suWireManager::wire_is_obsolete (const suWire * wire)
    const
  {
    SUASSERT (wire, "");

    const sutype::id_t id = wire->id();

    SUASSERT (id >= 0, "");
    SUASSERT (id < (sutype::id_t)_wireObsolete.size(), "");

    return _wireObsolete[id];
    
  } // end of suWireManager::wire_is_obsolete

  //
  bool suWireManager::wire_is_illegal (const suWire * wire)
    const
  {
    SUASSERT (wire, "");

    const sutype::id_t id = wire->id();

    SUASSERT (id >= 0, "");
    SUASSERT (id < (sutype::id_t)_wireIllegal.size(), "");

    return _wireIllegal[id];
    
  } // end of suWireManager::wire_is_illegal

  //
  void suWireManager::mark_wire_as_obsolete (const suWire * wire)
  {
    SUASSERT (wire, "");
    
    const sutype::id_t id = wire->id();

    SUASSERT (id >= 0, "");
    SUASSERT (id < (sutype::id_t)_wireObsolete.size(), "");

    SUASSERT (_wireObsolete[id] == false, "");

    _wireObsolete[id] = true;
     
  } // end of suWireManager::mark_wire_as_obsolete

  //
  void suWireManager::mark_wire_as_illegal (const suWire * wire)
  {
    SUASSERT (wire, "");

    const sutype::id_t id = wire->id();

    SUASSERT (id >= 0, "");
    SUASSERT (id < (sutype::id_t)_wireIllegal.size(), "");

    SUASSERT (_wireIllegal[id] == false, "");

    _wireIllegal[id] = true;
     
  } // end of suWireManager::mark_wire_as_illegal

  //
  void suWireManager::keep_wire_as_illegal_if_it_is_really_illegal (const suWire * wire)
  {
    if (wire == 0) return; // do nothing

    //SUINFO(1) << "keep_wire_as_illegal_if_it_is_really_illegal: " << wire->to_str() << std::endl;

    if (wire_is_illegal (wire)) return; // do nothing

    if (wire->satindex() != sutype::UNDEFINED_SAT_INDEX) return; // do nothing; only legal wires can have satindex
    
    if (!suSatRouter::instance()->wire_is_legal_slow_check (wire)) {
      SUASSERT (get_wire_usage (wire) == 1, "");
      mark_wire_as_illegal (wire);
    }
    
  } // end of suWireManager::keep_wire_as_illegal_if_it_is_really_illegal

  //
  void suWireManager::release_wire (suWire * wire,
                                    bool verbose)
  {
    // I can release a null wire to simplify a code where this wire is released
    if (!wire) {
      return;
    }
    
    release_wire_ (wire, verbose);

    sutype::svi_t wireusage = get_wire_usage (wire);
    SUASSERT (wireusage >= 0, "");
    
    if (wireusage == 0) {
      unregister_wire_ (wire);
    }
    
  } // end of suWireManager::release_wire
  
  //
  suWire * suWireManager::create_wire_from_dcoords (const suNet * net,
                                                    const suLayer * layer,
                                                    sutype::dcoord_t x1,
                                                    sutype::dcoord_t y1,
                                                    sutype::dcoord_t x2,
                                                    sutype::dcoord_t y2,
                                                    sutype::wire_type_t wiretype)
  {
    // create seed wire
    suWire * wire = create_or_reuse_a_wire_ (net, layer);
    
    // geometry
    wire->_rect.xl (std::min (x1, x2));
    wire->_rect.yl (std::min (y1, y2));
    wire->_rect.xh (std::max (x1, x2));
    wire->_rect.yh (std::max (y1, y2));

    // type
    wire->_type = wiretype;

    return register_or_replace_wire_ (wire);
        
  } // end of suWireManager::create_wire_from_dcoords

  //
  suWire * suWireManager::create_wire_from_edge_side (const suNet * net,
                                                      const suLayer * layer,
                                                      sutype::dcoord_t edgel,
                                                      sutype::dcoord_t edgeh,
                                                      sutype::dcoord_t sidel,
                                                      sutype::dcoord_t sideh,
                                                      sutype::wire_type_t wiretype)
  {
    // checks
    SUASSERT (edgel <= edgeh, "");
    SUASSERT (sidel <= sideh, "");

    // create seed wire
    suWire * wire = create_or_reuse_a_wire_ (net, layer);
    
    // geometry
    wire->_rect.edgel (layer->pgd(), edgel);
    wire->_rect.edgeh (layer->pgd(), edgeh);
    wire->_rect.sidel (layer->pgd(), sidel);
    wire->_rect.sideh (layer->pgd(), sideh);

    // type
    wire->_type = wiretype;

    return register_or_replace_wire_ (wire);
        
  } // end of suWireManager::create_wire_from_edge_side
  
  //
  suWire * suWireManager::create_wire_from_wire (const suNet * net,
                                                 const suWire * refwire,
                                                 sutype::wire_type_t wiretype)
  {
    // create seed wire
    suWire * wire = create_or_reuse_a_wire_ (net, refwire->layer());

    // geometry
    wire->_rect.copy (refwire->_rect);

    // type
    wire->_type = wiretype;
    
    return register_or_replace_wire_ (wire);
        
  } // end of suWireManager::create_wire_from_wire

  //
  suWire * suWireManager::create_wire_from_wire_then_shift (const suNet * net,
                                                            const suWire * refwire,
                                                            sutype::dcoord_t dx,
                                                            sutype::dcoord_t dy,
                                                            sutype::wire_type_t wiretype)
  {
    // create seed wire
    suWire * wire = create_or_reuse_a_wire_ (net, refwire->layer());

    // geometry
    wire->_rect.copy (refwire->_rect);
    wire->_rect.shift (dx, dy);

    // type
    wire->_type = wiretype;
    
    return register_or_replace_wire_ (wire);
        
  } // end of suWireManager::create_wire_from_wire_then_shift

  //
  void suWireManager::merge_wires (sutype::wires_t & wires)
  {
    std::map<sutype::id_t,sutype::wires_t> wiresPerNet;

    for (const auto & iter : wires) {

      suWire * wire = iter;
      const suNet * net = wire->net();
      SUASSERT (net, "");
      
      wiresPerNet[net->id()].push_back (wire);
    }

    wires.clear();

    for (auto & iter1 : wiresPerNet) {
      
      sutype::wires_t & netwires = iter1.second;
      
      std::map<sutype::id_t,sutype::wires_t> wiresPerLayer;

      for (const auto & iter2 : netwires) {

        suWire * wire = iter2;
        const suLayer * layer = wire->layer();
        SUASSERT (layer, "");

        wiresPerLayer[layer->pers_id()].push_back (wire);
      }
      
      for (auto & iter2 : wiresPerLayer) {
        
        sutype::wires_t & layerwires = iter2.second;
        
        merge_wires_ (layerwires);

        // store back
        wires.insert (wires.end(), layerwires.begin(), layerwires.end());
      }
    }

  } // end of suWireManager::merge_wires

  //
  void suWireManager::reserve_gid_for_wire (suWire * wire,
                                            sutype::id_t gid,
                                            bool checkIfPreReserved)
  {
    if (checkIfPreReserved) {
      if (_reservedGids.count (gid) == 0) return;
    }
    
    if (_reservedGids.at (gid) != 0) {
      SUASSERT (false, "gid=" << gid << " is used for two or even more wires: wire0=" << _reservedGids.at(gid)->to_str() << "; wire1=" << wire->to_str());
    }
    
    _reservedGids [gid] = wire;
    
  } // end of reserve_gid_for_wire  
  
  //
  suWire * suWireManager::get_wire_by_reserved_gid (sutype::id_t gid)
    const
  {
    if (_reservedGids.count (gid) == 0) {
      SUISSUE("Could not get a wire by gid") << ": gid=" << gid << std::endl;
      //SUASSERT (false, "Could not get a wire by gid=" << gid);
      return 0;
    }
    
    return _reservedGids.at (gid);
    
  } // end of suWireManager::get_wire_by_reserved_gid
  
  //
  void suWireManager::set_permanent_gid (suWire * wire,
                                         sutype::id_t gid)
  {
    SUASSERT (gid >= 0, "");
    SUASSERT (wire->gid() == sutype::UNDEFINED_GLOBAL_ID, "gid=" << gid << "; wire=" << wire->to_str());
    
    if (gid >= (sutype::id_t)_gidToWire.size()) {
      _gidToWire.resize (gid+1, (suWire*)0);
    }
    
    SUASSERT (_gidToWire [gid] == 0, "Two wires use the same gid=" << gid << ": wire1=" << _gidToWire[gid]->to_str() << "; wire2=" << wire->to_str());
    
    _gidToWire [gid] = wire;
    
    wire->_gid = gid;
    
  } // end of suWireManager::set_permanent_gid
    
  //
  sutype::wires_t suWireManager::get_wires_by_permanent_gid (const sutype::ids_t & gids,
                                                             const std::string & msg)
    const
  {
    SUASSERT (!gids.empty(), msg << " has an empty list of wires.");
    
    sutype::wires_t wires;
    
    std::set<sutype::id_t> usedWireGIDs;

    bool ok = true;
    
    for (const auto & iter1 : gids) {
        
      sutype::id_t gid = iter1;
       
      if (usedWireGIDs.count (gid) > 0) {
        SUASSERT (false, "Wire gid=" << gid << " is used more than once in a single" << msg << "; it's better to review the input; it may be a sign of a hidden problem");
        continue;
      }
      
      usedWireGIDs.insert (gid);
      
      if (gid < 0 || gid >= (sutype::id_t)_gidToWire.size()) {
        SUISSUE("Could not get a wire by gid") << ": " << gid << " to form a " << msg << std::endl;
        ok = false;
        continue;
      }

      suWire * wire = _gidToWire [gid];

      if (wire == 0) {
        SUISSUE("Could not get a wire by gid") << ": " << gid << " to form a " << msg << std::endl;
        ok = false;
        continue;
      }

      if (wire->gid() != gid) {
        SUISSUE("Could not get a wire by gid") << ": " << gid << " to form a " << msg << std::endl;
        ok = false;
        continue;
      }

      if (!wire->is (sutype::wt_preroute)) {
        SUISSUE("Unexpected wire type") << ": " << wire->to_str() << std::endl;
        SUASSERT (false, "");
        ok = false;
        continue;
      }
      
      wires.push_back (wire);

      if (wire->net() != wires.front()->net()) {
        SUISSUE("Wires belong to different nets") << ": Wires of one " << msg << " belong to different nets; wire1=" << wires.front()->to_str() << "; wire2=" << wire->to_str() << std::endl;
        SUASSERT (false, "");
        ok = false;
        continue;
      }
    }
    
    if (wires.empty()) {
      SUISSUE("Cound not get enough wires") << ": to form a " << msg << ". Read messaged above." << std::endl;
      //SUASSERT (false, "");
      ok = false;
    }
    
    std::sort (wires.begin(), wires.end(), suStatic::compare_wires_by_gid);

    if (0 && !ok) {
      SUASSERT (false, "Aborted because of errors found above. Read error messages.");
    }
    
    return wires;
    
  } // end of suWireManager::get_wires_by_permanent_gid
  
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
  suWire * suWireManager::create_or_reuse_a_wire_ (const suNet * net,
                                                   const suLayer * layer)
  {
    if (_wires.empty()) {

      const unsigned num = 1024;
      
      for (unsigned n=0; n < num; ++n) {
        
        suWire * wire = new suWire ();
        
        SUASSERT (wire->id() == (sutype::id_t)_numCreatedWires, "");
        ++_numCreatedWires;
        
        _wires.push_back (wire);
        
        const sutype::id_t id = wire->id();
        SUASSERT (id >= 0, "");
        
        // extend vectors 
        for (sutype::id_t k = _wireUsage   .size(); k <= id; ++k) { _wireUsage   .push_back (0);     }
        for (sutype::id_t k = _wireIllegal .size(); k <= id; ++k) { _wireIllegal .push_back (false); }
        for (sutype::id_t k = _wireObsolete.size(); k <= id; ++k) { _wireObsolete.push_back (false); }
      }
    }
    
    SUASSERT (!_wires.empty(), "");
    
    suWire * wire = _wires.back();
    _wires.pop_back ();

    // check
    SUASSERT (get_wire_usage   (wire) == 0,     "");
    SUASSERT (wire_is_illegal  (wire) == false, "");
    SUASSERT (wire_is_obsolete (wire) == false, "");
    
    // clean
    wire->_rect.clear();
    wire->_satindex = sutype::UNDEFINED_SAT_INDEX;
    wire->_gid      = sutype::UNDEFINED_GLOBAL_ID;
    wire->_dir      = sutype::wd_any;
    wire->_type     = sutype::wt_undefined;
    
    // set
    wire->_net   = net;
    wire->_layer = layer;
    
    // make a record
    increment_wire_usage (wire);
    
    return wire;
    
  } // end of suWireManager::create_or_reuse_a_wire_

  //
  void suWireManager::merge_wires_ (sutype::wires_t & wires)
  {
    if (wires.size() <= 1) return;

    for (sutype::svi_t i=0; i < (sutype::svi_t)wires.size(); ++i) {

      for (sutype::svi_t k=i+1; k < (sutype::svi_t)wires.size(); ++k) {
        
        bool merged = merge_two_wires_if_possible_to_the_first_wire_ (i, k, wires);
        if (!merged) continue;

        wires[k] = wires.back();
        wires.pop_back();
        k = i;
      }
    }
    
  } // end of suWireManager::merge_wires_

  //
  bool suWireManager::merge_two_wires_if_possible_to_the_first_wire_ (sutype::svi_t i,
                                                                      sutype::svi_t k,
                                                                      sutype::wires_t & wires)
  {
    SUASSERT (i >= 0, "");
    SUASSERT (k > i, "");
    SUASSERT (i < (sutype::svi_t)wires.size(), "");
    SUASSERT (k < (sutype::svi_t)wires.size(), "");

    suWire * wire1 = wires[i];
    suWire * wire2 = wires[k];

    SUASSERT (wire1->net() == wire2->net(), "");
    SUASSERT (wire1->layer() == wire2->layer(), "");
    
    // wire1 covers wire2
    if (wire1->rect().covers_compeletely (wire2->rect())) {

      inherit_gid_ (wire1, wire2);      
      release_wire (wire2);
      return true;
    }

    // wire2 covers wire1
    if (wire2->rect().covers_compeletely (wire1->rect())) {
      
      inherit_gid_ (wire2, wire1);
      release_wire (wire1);
      wires[i] = wire2;
      return true;
    }

    sutype::wire_type_t wiretype = sutype::wt_undefined;

    if (wire1->is (sutype::wt_preroute) || wire2->is (sutype::wt_preroute))
      wiretype = sutype::wt_preroute;
    
    // merge vertical wires
    if (wire1->rect().xl() == wire2->rect().xl() &&
        wire1->rect().xh() == wire2->rect().xh() &&
        wire1->rect().distance_to (wire2->rect(), sutype::go_ver) == 0) {
      
      sutype::dcoord_t xl = wire1->rect().xl();
      sutype::dcoord_t xh = wire1->rect().xh();
      sutype::dcoord_t yl = std::min (wire1->rect().yl(), wire2->rect().yl());
      sutype::dcoord_t yh = std::max (wire1->rect().yh(), wire2->rect().yh());
      
      suWire * newwire = create_wire_from_dcoords (wire1->net(),
                                                   wire1->layer(),
                                                   xl,
                                                   yl,
                                                   xh,
                                                   yh,
                                                   wiretype);
      
      inherit_gid_ (newwire, wire1);
      inherit_gid_ (newwire, wire2);

      release_wire (wire1);
      release_wire (wire2);
      
      wires[i] = newwire;

      return true;
    }
    
    // merge horizontal wires
    if (wire1->rect().yl() == wire2->rect().yl() &&
        wire1->rect().yh() == wire2->rect().yh() &&
        wire1->rect().distance_to (wire2->rect(), sutype::go_hor) == 0) {

      sutype::dcoord_t xl = std::min (wire1->rect().xl(), wire2->rect().xl());
      sutype::dcoord_t xh = std::max (wire1->rect().xh(), wire2->rect().xh());
      sutype::dcoord_t yl = wire1->rect().yl();
      sutype::dcoord_t yh = wire1->rect().yh();

      suWire * newwire = create_wire_from_dcoords (wire1->net(),
                                                   wire1->layer(),
                                                   xl,
                                                   yl,
                                                   xh,
                                                   yh,
                                                   wiretype);

      inherit_gid_ (newwire, wire1);
      inherit_gid_ (newwire, wire2);

      release_wire (wire1);
      release_wire (wire2);
      
      wires[i] = newwire;
      
      return true;
    }
    
    return false;
    
  } // end of suWireManager::merge_two_wires_if_possible_to_the_first_wire_

  //
  void suWireManager::unregister_wire_ (suWire * wire0)
  {
    const suLayer * layer = wire0->layer();
    SUASSERT (layer->pers_id() >= 0 && layer->pers_id() < (sutype::id_t)_wiresPerLayers.size(), "");
    
    sutype::dcoord_t sidel0 = layer->has_pgd() ? wire0->sidel() : wire0->rect().yl();
    sutype::dcoord_t sideh0 = layer->has_pgd() ? wire0->sideh() : wire0->rect().yh();
    sutype::dcoord_t edgel0 = layer->has_pgd() ? wire0->edgel() : wire0->rect().xl();
    
    sutype::wires_t & wires = _wiresPerLayers [layer->pers_id()][sidel0][sideh0][edgel0];

    for (sutype::uvi_t i=0; i < wires.size(); ++i) {

      suWire * wire1 = wires[i];
      if (wire1 != wire0) continue;

      wires[i] = wires.back();
      wires.pop_back();
      return;
    }

    SUASSERT (false, "Could not unregister a wire: " << wire0->to_str());
    
  } // end of suWireManager::unregister_wire_
  
  //
  suWire * suWireManager::register_or_replace_wire_ (suWire * wire0,
                                                     bool replace)
  {    
    SUASSERT (wire0->_rect.xl() <= wire0->_rect.xh(), "");
    SUASSERT (wire0->_rect.yl() <= wire0->_rect.yh(), "");

    //SUASSERT (wire0->type() != 0, "It's OK to have wires without any type; but I don't create such wires: " << wire0->to_str());
    
    if (_wiresPerLayers.empty()) {
      init_wires_per_layers_ ();
    }
    
    const suLayer * layer = wire0->layer();
    SUASSERT (layer->pers_id() >= 0 && layer->pers_id() < (sutype::id_t)_wiresPerLayers.size(), "");
    
    sutype::dcoord_t sidel0 = layer->has_pgd() ? wire0->sidel() : wire0->rect().yl();
    sutype::dcoord_t sideh0 = layer->has_pgd() ? wire0->sideh() : wire0->rect().yh();
    sutype::dcoord_t edgel0 = layer->has_pgd() ? wire0->edgel() : wire0->rect().xl();
    sutype::dcoord_t edgeh0 = layer->has_pgd() ? wire0->edgeh() : wire0->rect().xh();
    
    sutype::wires_t & wires = _wiresPerLayers [layer->pers_id()][sidel0][sideh0][edgel0];
    
    if (replace) {
      
      for (const auto & iter : wires) {
        
        suWire * wire1 = iter;
        SUASSERT (wire1->layer() == wire0->layer(), "");
        
        if (wire1->_dir  != wire0->_dir)  continue;
        if (wire1->_type != wire0->_type) continue;
        if (wire1->net() != wire0->net()) continue;
        
        sutype::dcoord_t edgeh1 = layer->has_pgd() ? wire1->edgeh() : wire1->rect().xh();
        if (edgeh1 != edgeh0) continue;
        
        // debug
        if (_debugid >= 0) {
          if (wire0->id() == _debugid) {
            SUINFO(1) << "SKIPPED wire0: " << wire0->to_str() << "; REUSED wire1: " << wire1->to_str() << std::endl;
          }
          if (wire1->id() == _debugid) {
            SUINFO(1) << "REUSED wire1: " << wire1->to_str() << "; instead of wire0: " << wire0->to_str() << std::endl;
          }
        }
        //
        
        release_wire_ (wire0, true);
        increment_wire_usage (wire1);
        
        return wire1;
      }
    }
    
    // register
    wires.push_back (wire0);

    // debug
    if (_debugid >= 0) {
      if (wire0->id() == _debugid) {
        SUINFO(1) << "REGISTERED a wire: " << wire0->to_str() << std::endl;
      }
    }
    //
    
    return wire0;
    
  } // suWireManager::register_or_replace_wire_

  //
  void suWireManager::release_wire_ (suWire * wire,
                                     bool verbose)
  {
    // in this mode, wire is deleted at the end of the flow
    if (!verbose) {
      wire->_net = 0;
    }
    
    // clean up a feature
    decrement_wire_usage_ (wire);

    sutype::svi_t wireusage = get_wire_usage (wire);
    SUASSERT (wireusage >= 0, "");
    
    if (wireusage == 0) {

      // clean up a feature; 
      if (wire->gid() != sutype::UNDEFINED_GLOBAL_ID) {
        
        if (verbose) {
          //SUISSUE("") << "Removed a wire with non-default gid=" << wire->gid() << ": " << wire->to_str() << std::endl;
          //SUASSERT (wire->gid() != 461904, "Aborted by Nikolai");
        }
        _gidToWire[wire->gid()] = 0;
      }
      
      _wires.push_back (wire);
      
      // clean up a feature; release a sat index
      if (wire->satindex() != sutype::UNDEFINED_SAT_INDEX) {
        suSatRouter::instance()->unregister_a_sat_wire (wire);
      }

      sutype::id_t id = wire->id();
      SUASSERT (id >= 0, "");

      // clean up a feature
      SUASSERT (id < (sutype::id_t)_wireObsolete.size(), "");
      _wireObsolete[id] = false;

      // clean up a feature
      SUASSERT (id < (sutype::id_t)_wireIllegal.size(), "");
      _wireIllegal[id] = false;
      
      // report
      if (_debugid >= 0) {
        if (wire->id() == _debugid) {
          SUINFO(1) << "RELEASED a wire: " << wire->to_str() << "; #releases=" << _num_releases << std::endl;
        }
      }
    }
    
  } // end of suWireManager::release_wire_

  //
  void suWireManager::init_wires_per_layers_ ()
  {
    SUASSERT (_wiresPerLayers.empty(), "");

    const sutype::layers_tc & idToLayer = suLayerManager::instance()->idToLayer();

    _wiresPerLayers.resize (idToLayer.size());
    
  } // end of suWireManager::init_wires_per_layers_

  //
  void suWireManager::increment_wire_usage (suWire * wire)
  {
    const sutype::id_t id = wire->id();

    SUASSERT (id >= 0, "");
    SUASSERT (id < (sutype::id_t)_wireUsage.size(), "");
    
    SUASSERT (_wireUsage[id] >= 0, "");
    
    ++_wireUsage[id];

    if (_debugid >= 0) {
      if (wire->id() == _debugid) {
        SUINFO(1) << "INCREMENTED wire usage: " << wire->to_str() << std::endl;
      }
    }
    
  } // end of suWireManager::increment_wire_usage
  
  //
  void suWireManager::decrement_wire_usage_ (suWire * wire)
  {
    const sutype::id_t id = wire->id();
    
    SUASSERT (id >= 0, "");
    SUASSERT (id < (sutype::id_t)_wireUsage.size(), "id=" << id << "; _wireUsage.size()=" << _wireUsage.size());

    if (_wireUsage[id] <= 0) {
      
      SUISSUE("Released wire is not marked as used") << ": Released wire id=" << id << " is not marked as used." << std::endl;
      
      if (std::find (_wires.begin(), _wires.end(), wire) != _wires.end()) {
        SUISSUE("Wire was already saved in the list of wires") << ": Wire id=" << id << std::endl;
      }
      
      //SUASSERT (false, "");
      return;
    }
    
    --_wireUsage [id];
    
    SUASSERT (_wireUsage [id] >= 0, "");

    if (_debugid >= 0) {
      if (wire->id() == _debugid) {
        SUINFO(1) << "DECREMENTED wire usage: " << wire->to_str() << std::endl;
      }
    }
    
  } // end of suWireManager::decrement_wire_usage_

  //
  void suWireManager::inherit_gid_ (suWire * wire1,
                                    suWire * wire2)
  {    
    if (wire2->gid() == sutype::UNDEFINED_GLOBAL_ID) return;

    if (wire1->gid() == sutype::UNDEFINED_GLOBAL_ID) {
     
      sutype::id_t gid = wire2->gid();
      SUASSERT (_gidToWire[gid] == wire2, "");

      wire1->_gid = gid;
      wire2->_gid = sutype::UNDEFINED_GLOBAL_ID;
      _gidToWire[gid] = wire1;
      
      return;
    }

    SUISSUE("Could not inherit GID")
      << ": Two wires were merged but wire1 gid=" << wire1->gid() << " could not inherit gid=" << wire2->gid() << " of wire2: wire1=" << wire1->to_str() << "; wire2=" << wire2->to_str()
      << std::endl;
    
  } // end of suWireManager::inherit_gid_
  
} // end of namespace amsr

// end of suWireManager.cpp
