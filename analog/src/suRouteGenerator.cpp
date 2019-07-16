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
//! \date   Thu Mar 29 12:51:12 2018

//! \file   suRouteGenerator.cpp
//! \brief  A collection of methods of the class suRouteGenerator.

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
#include <suStatic.h>

// other application includes
#include <suCellManager.h>
#include <suCellMaster.h>
#include <suClauseBank.h>
#include <suGenerator.h>
#include <suGeneratorInstance.h>
#include <suGlobalRouter.h>
#include <suGrid.h>
#include <suErrorManager.h>
#include <suLayer.h>
#include <suLayerManager.h>
#include <suLayoutFunc.h>
#include <suLayoutLeaf.h>
#include <suLayoutNode.h>
#include <suMetalTemplate.h>
#include <suMetalTemplateInstance.h>
#include <suMetalTemplateManager.h>
#include <suNet.h>
#include <suOptionManager.h>
#include <suRegion.h>
#include <suRoute.h>
#include <suSatRouter.h>
#include <suSatSolverWrapper.h>
#include <suStatic.h>
#include <suViaField.h>
#include <suWire.h>
#include <suWireManager.h>

// module include
#include <suRouteGenerator.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  //
  sutype::id_t suRouteGenerator::_uniqueId = 0;


  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  //
  suRouteGenerator::suRouteGenerator (const suNet * n,
                                      suWire * w1,
                                      suWire * w2)
  {
    init_ ();
 
    _net = n;
    _wire1 = (w1->layer()->level() < w2->layer()->level()) ? w1 : w2;
    _wire2 = (_wire1 == w1) ? w2 : w1;

    _wires1and2.push_back (_wire1);
    _wires1and2.push_back (_wire2);
    
    SUASSERT (_net, "");
    SUASSERT (_wire1, "");
    SUASSERT (_wire2, "");
    SUASSERT (_wire1->net() == _net, "");
    SUASSERT (_wire2->net() == _net, "");

    _layer1 = _wire1->layer();
    _layer2 = _wire2->layer();

    SUASSERT (_layer1->is (sutype::lt_metal | sutype::lt_wire), "wire1=" << _wire1->to_str());
    SUASSERT (_layer2->is (sutype::lt_metal | sutype::lt_wire), "wire2=" << _wire2->to_str());
    SUASSERT (_layer1->pgd() == sutype::go_ver || _layer1->pgd() == sutype::go_hor, "");
    SUASSERT (_layer2->pgd() == sutype::go_ver || _layer2->pgd() == sutype::go_hor, "");

    SUASSERT (_debugid == -1, "");

    _optionRouteFullBbox = false;//suOptionManager::instance()->get_boolean_option ("route_full_bbox", false);
    _optionRouteFreeForm = false;//suOptionManager::instance()->get_boolean_option ("route_free_form", false);
    
    _debugid = -1;
    SUINFO(0 || _debugid >= 0)
      << "Created suRouteGenerator id=" << _id << " (debugid=" << _debugid << ")"
      << "; wire1=" << _wire1->to_str()
      << "; wire2=" << _wire2->to_str()
      << std::endl;

    // debug
    if (0 && _id == 1) {

      _regions.clear();
      _regions.push_back (suGlobalRouter::instance()->get_region(1,2));
      _regions.push_back (suGlobalRouter::instance()->get_region(2,2));
      _regions.push_back (suGlobalRouter::instance()->get_region(2,3));
      _regions.push_back (suGlobalRouter::instance()->get_region(2,4));
      _regions.push_back (suGlobalRouter::instance()->get_region(3,4));
      _regions.push_back (suGlobalRouter::instance()->get_region(4,4));
    }

    // debug
    if (0 && !_regions.empty()) {

      for (const auto & iter : _regions)  {
        const suRegion * region = iter;
        suErrorManager::instance()->add_error (std::string ("tunnel_") + suStatic::dcoord_to_str (_id),
                                               region->bbox());
      }
    }
    
  } // end of suRouteGenerator

  //
  suRouteGenerator::~suRouteGenerator ()
  {
    clear_via_fields_ ();

    if (_id == _debugid) {
      //SUABORT;
    }
    
  } // end of suRouteGenerator::suRouteGenerator
  
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
  suRoute * suRouteGenerator::connect_wires_by_preroutes (const std::map<suWire *, sutype::uvi_t> & wireToGroupIndex)
  {
    SUASSERT (_route == 0, "");

    bool wiresAreConnected = false;
    if (_wire1->is (sutype::wt_preroute) && _wire2->is (sutype::wt_preroute)) {
      wiresAreConnected = try_to_connect_wires_by_preroutes_ (wireToGroupIndex);
    }

    if (!wiresAreConnected) {
      SUASSERT (_route == 0, "");
      return _route;
    }

    SUASSERT (_route, "");
    
    simplify_route_ ();
    
    return _route;
    
  } // end of suRouteGenerator::connect_wires_by_preroutes

  //
  suRoute * suRouteGenerator::create_route (const std::map<suWire *, sutype::uvi_t> & wireToGroupIndex)
  {
    const bool np = (_id == _debugid);

    SUINFO(np) << "Create a route between wire1=" << _wire1->to_str() << " and wire2=" << _wire2->to_str() << std::endl;
    
    bool ok = init_route_ ();
    if (!ok) {
      SUISSUE("Could not create a route for wires") << ": wire1=" << _wire1->to_str() << " and wire2=" << _wire2->to_str() << std::endl;
      return 0;
    }

    bool routeIsFeasible = true;
    
    bool wiresAreConnected = false;
    if (_wire1->is (sutype::wt_preroute) && _wire2->is (sutype::wt_preroute)) {
      wiresAreConnected = try_to_connect_wires_by_preroutes_ (wireToGroupIndex);
    }
    
    if (wiresAreConnected) {

      // do nothing
      // do not create any extra wires

      SUASSERT (_route, "");
      return _route;
    }

    // direct I-route
    if (_layer1 == _layer2 && _wire1->sidel() == _wire2->sidel() && _wire1->sideh() == _wire2->sideh()) {
      create_layout_fill_a_gap_between_input_wires_ ();
    }
    
    if (_optionRouteFreeForm) {
      
      // not implemented yet
      create_layout_free_form_ ();
    }

    else if (_optionRouteFullBbox) {
      
      int mindepth = 0;
      int maxdepth = 3;
      
      for (int depth = mindepth; depth <= maxdepth; ++depth) {
        create_layout_enumerate_shunts_between_wires_of_the_same_layer_ (depth);
      }      
    }
    
    else if (_layer1->base() == _layer2->base()) {
      
      // fill a gap between input wires
      if (_wire1->sidel() == _wire2->sidel() && _wire1->sideh() == _wire2->sideh()) {
        
        if (_layer1 == _layer2) {
          //create_layout_fill_a_gap_between_input_wires_ ();
        }
//         else {
//           SUASSERT (false, "Two wires of different colors share the same track.");
//           suLayoutFunc * layoutfunc2 = create_route_option_ ();
//           layoutfunc2->add_leaf (suSatSolverWrapper::instance()->get_constant(0));
//           routeIsFeasible = false;
//         }
      }
      
      // do not connect parallel trunks of the same layer by any shunt
      else if (_wire1->is (sutype::wt_trunk) && _wire2->is (sutype::wt_trunk)) {
        
        //SUISSUE("The router does not connect parallel trunks of the same layer by any shunt") << ": wire1=" << _wire1->to_str() << "; wire2=" << _wire2->to_str() << std::endl;
        suLayoutFunc * layoutfunc2 = create_route_option_ ();
        layoutfunc2->add_leaf (suSatSolverWrapper::instance()->get_constant(0));
        routeIsFeasible = false;
      }
      
      // create shunts 1+ layer below and above
      else {

        int mindepth = 0;
        int maxdepth = 1;

        for (int depth = mindepth; depth <= maxdepth; ++depth) {
          create_layout_enumerate_shunts_between_wires_of_the_same_layer_ (depth);
        }
        
      }
    }
    
    else {
      create_layout_connect_wires_on_different_layers_ ();
    }

    SUASSERT (_route, "");
    SUASSERT (_route->layoutFunc(), "");
    SUASSERT (_route->layoutFunc()->is_valid(), "");
    
    if (_route && np) {
      SUINFO(np) << "#########################################################################" << std::endl;
      SUINFO(np) << "Found route (before simplify):" << std::endl;
      suSatRouter::print_layout_node (std::cout, _route->layoutFunc(), "  ");
    }
    
    // post-processing
    // can delete unfeasible route
    if (_route) {
      simplify_route_ ();
    }
    
    if (!routeIsFeasible) {
      SUASSERT (_route == 0, "Must become null here after simplify");
    }
    
    //
    if (_route && np) {
      SUINFO(np) << "#########################################################################" << std::endl;
      SUINFO(np) << "Found route (after simplify):" << std::endl;
      suSatRouter::print_layout_node (std::cout, _route->layoutFunc(), "  ");
    }
    
    return _route;
    
  } // end of suRouteGenerator::create_route

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
  bool suRouteGenerator::init_route_ ()
  {
    SUASSERT (_route == 0, "");
    SUASSERT (_layoutfunc0 == 0, "");
    SUASSERT (_layoutfunc1 == 0, "");
    SUASSERT (_wire1->satindex() != sutype::UNDEFINED_SAT_INDEX, "");
    SUASSERT (_wire2->satindex() != sutype::UNDEFINED_SAT_INDEX, "");

    if (suSatSolverWrapper::instance()->is_constant (0, _wire1->satindex())) return false;
    if (suSatSolverWrapper::instance()->is_constant (0, _wire2->satindex())) return false;
    
    if (suSatRouter::instance()->wires_are_in_conflict (_wire1, _wire2)) {
      return false;
    }
    
    // these two starting wires are used in other algorithms
    _route = new suRoute ();
    _route->add_wire (_wire1); // input wire (sutype::wt_preroute || sutype::wt_trunk)
    _route->add_wire (_wire2); // input wire (sutype::wt_preroute || sutype::wt_trunk)
            
    _layoutfunc0 = _route->layoutFunc();
    SUASSERT (_layoutfunc0, "");
    
    _layoutfunc0->func (sutype::logic_func_and);
    _layoutfunc0->sign (sutype::unary_sign_just);
    
    _layoutfunc0->add_leaf (_wire1->satindex());
    _layoutfunc0->add_leaf (_wire2->satindex());
    
    suWireManager::instance()->increment_wire_usage (_wire1);
    suWireManager::instance()->increment_wire_usage (_wire2);

    // if I don't like this route, the SAT solver can set this extra satindex as a constant_0 to forbid this route
    // otherwise, there may be very strange side effects during optimize_the_number_of_routes_ sutype::om_minimize
    sutype::satindex_t routeenabler = suSatSolverWrapper::instance()->get_next_sat_index ();
    _routeEnablers.push_back (routeenabler);
    _layoutfunc0->add_leaf (routeenabler);
    
    // in this OR, I collect all possible routes between _wire1 and _wire2
    _layoutfunc1 = _layoutfunc0->add_func (sutype::logic_func_or, sutype::unary_sign_just, SUFILELINE_DLF);
    
    return true;
    
  } // end of suRouteGenerator::init_route_

  //
  void suRouteGenerator::simplify_route_ ()
  {
    if (_optionRouteFullBbox) return; // specific case; not so many preroutes

    const bool np = (_id == _debugid);
    
    SUASSERT (_route, "");
    SUASSERT (_route->layoutFunc()->is_valid(), "");

    const sutype::wires_t & routewires = _route->wires();

    for (const auto & iter : routewires) {
      suWire * wire = iter;
      SUASSERT (wire->satindex() != sutype::UNDEFINED_SAT_INDEX, "");
    }
      
    _route->layoutFunc()->simplify();
    
    SUASSERT (_route->layoutFunc()->sign() == sutype::unary_sign_just, "");
    
    if (_route->layoutFunc()->nodes().size() == 1) {
      
      //suSatRouter::print_layout_node (std::cout, _route->layoutFunc(), "  ");
      
      SUASSERT (_route->layoutFunc()->nodes().front()->is_leaf(), "");
      sutype::satindex_t satindex = _route->layoutFunc()->nodes().front()->to_leaf()->satindex();
      
      if (suSatSolverWrapper::instance()->is_constant (0, satindex)) {
        SUINFO(np) << "Found unfeasible route." << std::endl;
        delete_route_ ();
        return;
      }
    }
    
    _route->release_useless_wires ();
    
  } // end of suRouteGenerator::simplify_route_
  
  //
  void suRouteGenerator::delete_route_ ()
  {
    SUASSERT (_routeEnablers.size() == 1, "");
    
    for (const auto & iter : _routeEnablers) {
      
      sutype::satindex_t satindex = iter;
      
      bool released = suSatSolverWrapper::instance()->return_sat_index (satindex);
      SUASSERT (released, "");
    }
    
    delete _route;
    
    _route = 0;
    
  } // end of suRouteGenerator::delete_route_
  
  //
  bool suRouteGenerator::try_to_connect_wires_by_preroutes_ (const std::map<suWire *, sutype::uvi_t> & wireToGroupIndex)
  {
    const bool np = (_id == _debugid);
    
    SUASSERT (_net, "");
    SUASSERT (_wire1, "");
    SUASSERT (_wire2, "");

    bool reachedTargetWire = false;

    // use pre-computed info
    if (!wireToGroupIndex.empty()) {

      SUASSERT (!_wire1->is (sutype::wt_preroute) || wireToGroupIndex.count (_wire1) > 0, "");
      SUASSERT (!_wire2->is (sutype::wt_preroute) || wireToGroupIndex.count (_wire2) > 0, "");
      
      if (wireToGroupIndex.count (_wire1) > 0 && wireToGroupIndex.count (_wire2) > 0) {
        if (wireToGroupIndex.at (_wire1) == wireToGroupIndex.at (_wire2))
          reachedTargetWire = true;
      }
    }

    // find a path on-fly here
    else {

      SUASSERT (false, "obsolete");
    
      const sutype::wires_t & netwires = _net->wires();
      const sutype::generatorinstances_t & netgeneratorinstances = _net->generatorinstances();

      sutype::wires_t visitedwires;
      sutype::generatorinstances_t visitedgeneratorinstances;

      std::vector<bool> wiresInUse (netwires.size(), false);
      std::vector<bool> gisInUse (netgeneratorinstances.size(), false);

      bool foundwire1 = false;
      bool foundwire2 = false;

      for (sutype::uvi_t i=0; i < netwires.size(); ++i) {
     
        suWire * wire = netwires[i];
        if (wire == _wire1) { foundwire1 = true; wiresInUse[i] = true; }
        if (wire == _wire2) { foundwire2 = true; }
      }
    
      SUASSERT (foundwire1, "Could not find wire " << _wire1->to_str());
      SUASSERT (foundwire2, "Could not find wire " << _wire2->to_str());
    
      const bool collectAllConnectedWires = false;
    
      find_a_path_between_wires_ (_wire1,
                                  _wire2,
                                  netwires,
                                  netgeneratorinstances,
                                  visitedwires,
                                  visitedgeneratorinstances,
                                  wiresInUse,
                                  gisInUse,
                                  reachedTargetWire,
                                  collectAllConnectedWires);
    }

    if (!reachedTargetWire) return false;

    SUINFO(np) << "Found a routed path between two wires: wire1=" << _wire1->to_str() << "; wire2=" << _wire2->to_str() << std::endl;

    if (_route == 0) {
      bool ok = init_route_ ();
      SUASSERT (ok, "");
    }

    SUASSERT (_route, "");
    SUASSERT (_layoutfunc1, "");
    
    suLayoutFunc * layoutfunc2 = create_route_option_ ();
    
    layoutfunc2->add_leaf (suSatSolverWrapper::instance()->get_constant(1));
        
    return true;
    
  } // end of suRouteGenerator::try_to_connect_wires_by_preroutes_
  
  //
  void suRouteGenerator::find_a_path_between_wires_ (suWire * startwire,
                                                     suWire * tartgetwire,
                                                     const sutype::wires_t & netwires,
                                                     const sutype::generatorinstances_t & netgeneratorinstances,
                                                     sutype::wires_t & visitedwires,
                                                     sutype::generatorinstances_t & visitedgeneratorinstances,
                                                     std::vector<bool> & wiresInUse,
                                                     std::vector<bool> & gisInUse,
                                                     bool & reachedTargetWire,
                                                     const bool collectAllConnectedWires)
    const
  {
    SUASSERT (startwire, "");

    if (tartgetwire) {
      // do nothing
    }
    else {
      // it a legal mode; tartget wire can be null
      // in this case the procedure just collects all wires connected with the start wire
      SUASSERT (collectAllConnectedWires, "");
    }

    for (sutype::uvi_t i=0; i < netwires.size(); ++i) {

      if (wiresInUse[i]) continue;

      suWire * wire = netwires[i];
      SUASSERT (wire, "");
      
      if (suStatic::wires_are_electrically_connected (startwire, wire)) {
      
        if (collectAllConnectedWires) {
          visitedwires.push_back (wire);
        }
        
        wiresInUse[i] = true;
        
        if (wire == tartgetwire) {
          reachedTargetWire = true;
        }

        // mission completed
        if (reachedTargetWire && !collectAllConnectedWires) return;
        
        find_a_path_between_wires_ (wire,
                                    tartgetwire,
                                    netwires,
                                    netgeneratorinstances,
                                    visitedwires,
                                    visitedgeneratorinstances,
                                    wiresInUse,
                                    gisInUse,
                                    reachedTargetWire,
                                    collectAllConnectedWires);

        // mission completed
        if (reachedTargetWire && !collectAllConnectedWires) return;
      }
    }

    for (sutype::uvi_t i=0; i < netgeneratorinstances.size(); ++i) {

      if (gisInUse[i]) continue;

      suGeneratorInstance * gi = netgeneratorinstances[i];
      SUASSERT (gi, "");

      const sutype::wires_t & giwires = gi->wires();

      for (sutype::uvi_t k=0; k < giwires.size(); ++k) {

        suWire * giwire = giwires[k];
        SUASSERT (giwire, "");
        if (giwire->layer() != gi->generator()->get_cut_layer()) continue;

        if (suStatic::wires_are_electrically_connected (startwire, giwire)) {

          if (collectAllConnectedWires) {
            visitedgeneratorinstances.push_back (gi);
          }
          
          gisInUse[i] = true;

          find_a_path_between_wires_ (giwire,
                                      tartgetwire,
                                      netwires,
                                      netgeneratorinstances,
                                      visitedwires,
                                      visitedgeneratorinstances,
                                      wiresInUse,
                                      gisInUse,
                                      reachedTargetWire,
                                      collectAllConnectedWires);

          // mission completed
          if (reachedTargetWire && !collectAllConnectedWires) return;
          
          break;
        }
      }
    }
    
  } // end of suRouteGenerator::find_a_path_between_wires_
  
  //
  void suRouteGenerator::create_layout_fill_a_gap_between_input_wires_ ()
  {    
    suLayoutFunc * layoutfunc2 = create_route_option_ ();

    sutype::dcoord_t e1 = std::max (_wire1->edgel(), _wire2->edgel());
    sutype::dcoord_t e2 = std::min (_wire1->edgeh(), _wire2->edgeh());
    
    // fill a cut between _wire1 and _wire2
    if (e1 > e2) {
      
      // can't create a new wire
      if (_layer1->fixed()) {
        SUISSUE("Could not create a route between two wires") << ": wire1=" << _wire1->to_str() << "; wire2=" << _wire2->to_str() << std::endl;
        layoutfunc2->add_leaf (suSatSolverWrapper::instance()->get_constant(0));
        return;
      }
      
      // this wire fills a gap
      suWire * wire = suWireManager::instance()->create_wire_from_edge_side (_net,
                                                                             _layer1,
                                                                             e2,
                                                                             e1,
                                                                             _wire1->sidel(),
                                                                             _wire1->sideh(),
                                                                             sutype::wt_route); // a route fills a cut between two wires

      wire = serve_just_created_wire_ (wire);
      
      if (!wire) {
        layoutfunc2->add_leaf (suSatSolverWrapper::instance()->get_constant(0));
        return;
      }
      
      _route->add_wire (wire); // this wire fills a gap (sutype::wt_route)
      suSatRouter::instance()->register_a_sat_wire (wire);
      sutype::satindex_t wiresatindex = wire->satindex();
      layoutfunc2->add_leaf (wiresatindex);
    }
    
    // wires overlap; don't need an extra wire
    else {
      layoutfunc2->add_leaf (suSatSolverWrapper::instance()->get_constant(1));
    }
    
  } // end of suRouteGenerator::create_layout_fill_a_gap_between_input_wires_

  //
  void suRouteGenerator::create_layout_free_form_ ()
  {
    suLayoutFunc * layoutfunc2 = create_route_option_();

    sutype::satindex_t routeenabler = suSatSolverWrapper::instance()->get_next_sat_index ();
    _routeEnablers.push_back (routeenabler);
    layoutfunc2->add_leaf (routeenabler);

    const sutype::layers_tc & layers = suLayerManager::instance()->layers();
    SUASSERT (!layers.empty(), "");
    
    suRectangle transitionrect;
    calculate_transition_region_ (_wire1, _wire2, transitionrect);

    SUASSERT (_wirelayers.empty(), "");
    SUASSERT (_vialayers.empty(), "");
    SUASSERT (_viaFields.empty(), "");

    for (sutype::uvi_t i=0; i < layers.size(); ++i) {

      const suLayer * layer = layers[i];
      if (!layer->is_base()) continue;
      if (!layer->is (sutype::lt_metal)) continue;
      _wirelayers.push_back (layer);
    }

    std::sort (_wirelayers.begin(), _wirelayers.end(), suStatic::compare_layers_by_level);
    
    const sutype::metaltemplateinstances_t & mtis = suMetalTemplateManager::instance()->allMetalTemplateInstances();

    std::vector<sutype::wires_t> wiresPerLayer;
    wiresPerLayer.resize (_wirelayers.size());

    for (sutype::uvi_t i=0; i < _wirelayers.size(); ++i) {

      const suLayer * wirelayer = _wirelayers[i];

      for (sutype::uvi_t k=0; k < mtis.size(); ++k) {

        const suMetalTemplateInstance * mti = mtis[k];
        if (mti->metalTemplate()->baseLayer() != wirelayer) continue;

        // debug
        if (0) {

          suErrorManager::instance()->add_error (std::string ("trrect_") + suStatic::dcoord_to_str (_id),
                                                 transitionrect);
        }
      
        sutype::wires_t discretewires;
        mti->create_wires_in_rectangle (transitionrect, _net, sutype::wt_route, true, discretewires); // discrete route wires
      
        for (const auto & iter1 : discretewires) {
        
          suWire * wire = iter1;
          wire = serve_just_created_wire_ (wire); // free form; WIP

          if (!wire) continue;
          wiresPerLayer[i].push_back (wire);
        }
      }
    }

    bool ok = true;
    
    for (sutype::uvi_t i=0; i < _wirelayers.size(); ++i) {

      const suLayer * wirelayer0 = _wirelayers[i];
      const sutype::wires_t & discretewires0 = wiresPerLayer[i];
      
      if (!discretewires0.empty()) continue;

      // found a wire layer without available wires

      // all wire layers below _layer1 are unfeasible
      if (wirelayer0->level() < _layer1->level()) {
        for (sutype::uvi_t k=0; k < i; ++k) {
          sutype::wires_t & discretewires1 = wiresPerLayer[k];
          for (const auto & iter1 : discretewires1) {
            suWire * wire = iter1;
            suWireManager::instance()->release_wire (wire);
          }
          discretewires1.clear();
        }
      }
      // all wire layers above _layer2 are unfeasible
      else if (wirelayer0->level() > _layer2->level()) {
        for (sutype::uvi_t k=i+1; k < _wirelayers.size(); ++k) {
          sutype::wires_t & discretewires1 = wiresPerLayer[k];
          for (const auto & iter1 : discretewires1) {
            suWire * wire = iter1;
            suWireManager::instance()->release_wire (wire);
          }
          discretewires1.clear();
        }
      }
      else {
        ok = false;
        break;
      }
    }

    // there're now wires between _layer0 and _layer1
    if (!ok) {
      for (sutype::uvi_t i=0; i < _wirelayers.size(); ++i) {
        const sutype::wires_t & discretewires = wiresPerLayer[i];
        for (const auto & iter1 : discretewires) {
          suWire * wire = iter1;
          suWireManager::instance()->release_wire (wire);
        }
      }

      layoutfunc2->add_leaf (suSatSolverWrapper::instance()->get_constant (0));
      return;
    }

    // trim useless wire layers
    sutype::uvi_t counter = 0;

    for (sutype::uvi_t i=0; i < _wirelayers.size(); ++i) {

      const sutype::wires_t & discretewires0 = wiresPerLayer[i];
      if (discretewires0.empty()) continue;
      
      _wirelayers[counter] = _wirelayers[i];
      wiresPerLayer[counter] = wiresPerLayer[i];
      ++counter;
    }

    _wirelayers.resize (counter);
    wiresPerLayer.resize (counter);

    // find via layers
    for (sutype::uvi_t i=0; i+1 < _wirelayers.size(); ++i) {

      const suLayer * wirelayer0 = _wirelayers[i];
      const suLayer * wirelayer1 = _wirelayers[i+1];
      
      const suLayer * vialayer = suLayerManager::instance()->get_base_via_layer (wirelayer0, wirelayer1);      
      SUASSERT (vialayer, "wirelayer0=" << wirelayer0->to_str() << "; wirelayer1=" << wirelayer1->to_str() << "; layer1=" << _layer1->to_str() << "; layer2=" << _layer2->to_str());
      SUASSERT (vialayer->is_base(), "");
      
      _vialayers.push_back (vialayer);
    }

    // false: create a rectange of vias 
    ok = create_via_fields_ (false);
    if (!ok) {
      SUASSERT (false, "unexpected");
      layoutfunc2->add_leaf (suSatSolverWrapper::instance()->get_constant (0));
      return;
    }

    ok = sat_problem_every_via_field_must_have_at_least_one_via_ (layoutfunc2, true, false);
    if (!ok) {
      SUASSERT (false, "unexpected");
      layoutfunc2->add_leaf (suSatSolverWrapper::instance()->get_constant (0));
      return;
    }

    for (sutype::uvi_t i=0; i < _wirelayers.size(); ++i) {

      const sutype::wires_t & discretewires = wiresPerLayer[i];

      suLayoutFunc * layoutfunc3 = 0;

      for (const auto & iter1 : discretewires) {

        suWire * wire = iter1;
        
        suSatRouter::instance()->register_a_sat_wire (wire);
        
        if (layoutfunc3 == 0)
          layoutfunc3 = layoutfunc2->add_func (sutype::logic_func_or, sutype::unary_sign_just, SUFILELINE_DLF);
        
        layoutfunc3->add_leaf (wire->satindex());
      }
    }
        
  } // end of suRouteGenerator::create_layout_free_form_

  //
  void suRouteGenerator::create_layout_enumerate_shunts_between_wires_of_the_same_layer_ (int depth)
  {
    const bool np = (_id == _debugid);

    // depth = -1: unlimited layer stack
    //SUASSERT (depth == -1 || depth == 0 || depth == 1, "");
    //SUASSERT (_layer1->base() == _layer2->base(), "");
    
    const sutype::layers_tc & layers = suLayerManager::instance()->layers();
    SUASSERT (!layers.empty(), "");
    
    // mode 1: find a shunt below
    // mode 2: find a shunt above
    for (int mode = 0; mode <= 1; ++mode) {

      suLayoutFunc * layoutfunc2 = create_route_option_();

      clear_via_fields_ ();

      bool above = (bool)mode;

      // init wire stack
      _wirelayers.push_back (_layer1);

      bool routeIsFeasible = true;
      const suLayer * wirelayer = _layer1;

      for (int d=0; depth < 0 || d <= depth; ++d) {

        const suLayer * shuntlayer = suLayerManager::instance()->get_base_adjacent_layer (wirelayer, sutype::lt_metal | sutype::lt_wire, above, 0);
        
        if (shuntlayer == 0) {
          if (depth < 0) break; // unlimited depth
          SUINFO(np) << "Found no layer " << (above ? "above" : "below") << "; depth=" << depth << "; d=" << d << "; wirelayer=" << wirelayer->name() << std::endl;
          routeIsFeasible = false;
          break;
        }

        if (shuntlayer->level() < suLayerManager::instance()->lowestDRLayer()->level()) {
          if (depth < 0) break; // unlimited depth
          SUINFO(np) << "Layer is not for routing: " << shuntlayer->to_str() << std::endl;
          routeIsFeasible = false;
          break;
        }

        if (suLayerManager::instance()->highestDRLayer() && shuntlayer->level() > suLayerManager::instance()->highestDRLayer()->level()) {
          if (depth < 0) break; // unlimited depth
          SUINFO(np) << "Layer is not for routing: " << shuntlayer->to_str() << std::endl;
          routeIsFeasible = false;
          break;
        }
        
        _wirelayers.push_back (shuntlayer);
        wirelayer = shuntlayer;
      }
      
      if (_wirelayers.size() == 1) {
        SUINFO(np) << "Found no layers " << (above ? "above" : "below") << "; depth=" << depth << "; wirelayer=" << _layer1->name() << std::endl;
        routeIsFeasible = false;
      }
      
      if (!routeIsFeasible) {
        layoutfunc2->add_leaf (suSatSolverWrapper::instance()->get_constant (0));
        continue;
      }

      // complete wire stack
      SUASSERT (_wirelayers.size() >= 2, "mode=" << mode);

      sutype::svi_t index0 = (sutype::svi_t)_wirelayers.size() - 2;
      sutype::svi_t index1 = 1;

      for (sutype::svi_t i = index0; i >= index1; --i) {
        const suLayer * wirelayer = _wirelayers[i];
        _wirelayers.push_back (wirelayer);
        if (wirelayer == _layer2) break; // to support wires on differnt layers
      }
      
      // complete wire stack
      if (_wirelayers.back() != _layer2) {
        _wirelayers.push_back (_layer2);
      }

      if (_layer1 == _layer2 && (_wirelayers.size() % 2) != 1) {

        SUISSUE("Unexpected number of layers")
          << ": " << _wirelayers.size()
          << std::endl;
        
        for (sutype::uvi_t i=0; i < _wirelayers.size(); ++i) {
          const suLayer * layer = _wirelayers[i];
          SUINFO(1) << layer->to_str() << std::endl;
        }

        SUASSERT (false, "");
      }

      if (_layer1 == _layer2) {
        SUASSERT ((_wirelayers.size() % 2) == 1, "");
      }

      // complete via stack
      for (sutype::uvi_t i=0; i+1 < _wirelayers.size(); ++i) {
        
        const suLayer * wirelayer0 = _wirelayers[i];
        const suLayer * wirelayer1 = _wirelayers[i+1];

        //SUINFO(np) << "wirelayer0=" << wirelayer0->to_str() << "; wirelayer1=" << wirelayer1->to_str() << std::endl;
        
        const suLayer * vialayer = suLayerManager::instance()->get_base_via_layer (wirelayer0, wirelayer1);
        SUASSERT (vialayer, "wirelayer0=" << wirelayer0->to_str() << "; wirelayer1=" << wirelayer1->to_str() << "; layer1=" << _layer1->to_str() << "; layer2=" << _layer2->to_str());
        SUASSERT (vialayer->is_base(), "");
        
        _vialayers.push_back (vialayer);
      }
      
      if (_layer1 == _layer2) {
        SUASSERT ((_vialayers.size() % 2) == 0, "");
      }
      
      SUASSERT (_wirelayers.size() == _vialayers.size() + 1, "");

      // dummy check
      if (_layer1->level() == _layer2->level()) {
        
        for (sutype::uvi_t i=0; i < _wirelayers.size() / 2; ++i) {

          const suLayer * wirelayer0 = _wirelayers [(int)i];
          const suLayer * wirelayer1 = _wirelayers [(int)_wirelayers.size() - 1 - i];

          SUASSERT (wirelayer0->base() == wirelayer1->base(), "");
        }

        // dummy check
        for (sutype::uvi_t i=0; i < _vialayers.size() / 2; ++i) {

          const suLayer * vialayer0 = _vialayers [(int)i];
          const suLayer * vialayer1 = _vialayers [(int)_vialayers.size() - 1 - i];

          SUASSERT (vialayer0 == vialayer1, "");
        }
      }
      // end of dummy check
      
      bool ok = create_via_fields_ (true);
      if (!ok) {
        layoutfunc2->add_leaf (suSatSolverWrapper::instance()->get_constant (0));
        continue;
      }
      
      ok = convert_via_fields_to_sat_problem_ (layoutfunc2);
      if (!ok) {
        layoutfunc2->add_leaf (suSatSolverWrapper::instance()->get_constant (0));
        continue;
      }
    }
    
  } // end of suRouteGenerator::create_layout_enumerate_shunts_between_wires_of_the_same_layer_
  
  //
  void suRouteGenerator::create_layout_connect_wires_on_different_layers_ ()
  {
    const bool np = (_id == _debugid);    
    SUINFO(np) << "Create a route between wires on different layers: wire1=" << _wire1->to_str() << " and wire2=" << _wire2->to_str() << std::endl;

    suLayoutFunc * layoutfunc2 = create_route_option_();
        
    const sutype::layers_tc & layers = suLayerManager::instance()->layers();
    SUASSERT (!layers.empty(), "");
    
    for (const auto & iter : layers) {
      
      const suLayer * layer = iter;
      if (!layer->is_base()) continue;

      if (
          layer->is (sutype::lt_via) &&
          layer->level() > _layer1->level() &&
          layer->level() < _layer2->level()
          ) {

        _vialayers.push_back (layer);
      }

      if (
          layer->is (sutype::lt_wire) &&
          layer->level() >= _layer1->level() &&
          layer->level() <= _layer2->level()
          ) {
        
        _wirelayers.push_back (layer);
      }
    }

    SUASSERT (_wirelayers.size() == _vialayers.size() + 1, "");

    bool ok = create_via_fields_ (true);
    
    if (!ok) {
      SUINFO(np) << "Could not create via fields" << std::endl;
      layoutfunc2->add_leaf (suSatSolverWrapper::instance()->get_constant (0));
      return;
    }

    ok = convert_via_fields_to_sat_problem_ (layoutfunc2);

    if (!ok) {
      SUINFO(np) << "Could not convert via fields to SAT problem" << std::endl;
      layoutfunc2->add_leaf (suSatSolverWrapper::instance()->get_constant (0));
      return;
    }
    
  } // end of suRouteGenerator::create_layout_connect_wires_on_different_layers_

  //
  void suRouteGenerator::clear_via_fields_ ()
  {
    for (const auto & iter : _viaFields) {
      suViaField * vf = iter;
      delete vf;
    }

    _vialayers .clear ();
    _wirelayers.clear ();
    _viaFields .clear ();
    
  } // end of suRouteGenerator::clear_via_fields_

  //
  void suRouteGenerator::remove_via_option_ (sutype::uvi_t v,
                                             sutype::dcoord_t dx,
                                             sutype::dcoord_t dy)
  {
    SUASSERT (v < _viaFields.size(), "");

    suViaField * vf = _viaFields[v];
    
    sutype::viaoptions_t & vos = vf->viaOptions();

    sutype::uvi_t counter = 0;

    for (sutype::uvi_t i=0; i < vos.size(); ++i) {

      const sutype::viaoption_t & vo = vos[i]; 
      
      sutype::dcoord_t dx0 = std::get<0>(vo);
      sutype::dcoord_t dy0 = std::get<1>(vo);
      //const sutype::generatorinstances_t & gis0 = std::get<2>(vo);
      
      if (dx0 != dx || dy0 != dy) {
        vos[counter] = vo;
        ++counter;
        continue;
      }
    }

    if (vos.size() != counter) {
      vos.resize (counter);
    }
    else {
      SUASSERT (false, "");
    }
    
  } // end of suRouteGenerator::remove_via_option_
  
  //
  void suRouteGenerator::remove_via_options_ (sutype::uvi_t v,
                                              sutype::dcoord_t dcoord,
                                              sutype::grid_orientation_t gd)
  {
    SUASSERT (v < _viaFields.size(), "");
    SUASSERT (gd == sutype::go_ver || gd == sutype::go_hor, "");
    
    suViaField * vf = _viaFields[v];
    
    sutype::viaoptions_t & vos = vf->viaOptions();
    
    sutype::uvi_t counter = 0;

    for (sutype::uvi_t i=0; i < vos.size(); ++i) {

      const sutype::viaoption_t & vo = vos[i]; 
      
      sutype::dcoord_t dx = std::get<0>(vo);
      sutype::dcoord_t dy = std::get<1>(vo);
      //const sutype::generatorinstances_t & gis = std::get<2>(vo);
      
      sutype::dcoord_t dcoord0 = (gd == sutype::go_ver) ? dx : dy;
      
      if (dcoord0 != dcoord) {
        vos[counter] = vo;
        ++counter;
        continue;
      }
    }

    if (vos.size() != counter) {
      vos.resize (counter);
    }
    else {
      SUASSERT (false, "");
    }
    
  } // end of suRouteGenerator::remove_via_options_

  //
  std::map<sutype::dcoordpair_t,sutype::clause_t> suRouteGenerator::get_dcoords_and_satindices_ (sutype::uvi_t v)
    const
  {
    SUASSERT (v < _viaFields.size(), "");
    
    std::map<sutype::dcoordpair_t,sutype::clause_t> satindicesPerDxDy;

    suViaField * vf = _viaFields[v];
    const sutype::viaoptions_t & vos = vf->viaOptions();

    for (const auto & iter1 : vos) {
      
      const sutype::viaoption_t & vo = iter1;
      
      sutype::dcoord_t dx                      = std::get<0>(vo);
      sutype::dcoord_t dy                      = std::get<1>(vo);
      const sutype::generatorinstances_t & gis = std::get<2>(vo);

      const sutype::dcoordpair_t key (dx, dy);

      for (const auto & iter2 : gis) {
        
        suGeneratorInstance * gi = iter2;
        SUASSERT (gi->legal(), "");
        
        sutype::satindex_t satindex = gi->layoutFunc()->satindex();
        SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");

        satindicesPerDxDy[key].push_back (satindex);
      }
      
      SUASSERT (satindicesPerDxDy[key].size() == gis.size(), "");
    }
    
    return satindicesPerDxDy;
    
  } // end of suRouteGenerator::get_dcoords_and_satindices_

  //
  std::map<sutype::dcoord_t,sutype::clause_t> suRouteGenerator::get_dcoords_and_satindices_ (sutype::uvi_t v,
                                                                                             sutype::grid_orientation_t gd)
    const
  {
    SUASSERT (v < _viaFields.size(), "");

    std::map<sutype::dcoord_t,sutype::clause_t> satindicesPerDcoord;
  
    suViaField * vf = _viaFields[v];
    const sutype::viaoptions_t & vos = vf->viaOptions();
      
    for (const auto & iter1 : vos) {
      
      const sutype::viaoption_t & vo = iter1;
      
      sutype::dcoord_t dx                      = std::get<0>(vo);
      sutype::dcoord_t dy                      = std::get<1>(vo);
      const sutype::generatorinstances_t & gis = std::get<2>(vo);

      sutype::dcoord_t dcoord = (gd == sutype::go_ver) ? dx : dy;
      
      for (const auto & iter2 : gis) {
        
        suGeneratorInstance * gi = iter2;
        SUASSERT (gi->legal(), "");
        
        sutype::satindex_t satindex = gi->layoutFunc()->satindex();
        SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");
        
        satindicesPerDcoord[dcoord].push_back (satindex);
      }
    }

    return satindicesPerDcoord;
    
  } // end of suRouteGenerator::get_dcoords_and_satindices_

  // createLineOfViasForFirstAndLastViaFields=1: create only a line of vias for the first and last via fields
  bool suRouteGenerator::create_via_fields_ (bool createLineOfViasForFirstAndLastViaFields)
  {
    const bool np = (_id == _debugid);

    if (np) {

      SUINFO(np) << "wire1: " << _wire1->to_str() << std::endl;

      for (sutype::uvi_t i=0; i < _vialayers.size(); ++i) {
          
        if (i < _wirelayers.size()) {
          const suLayer * wirelayer = _wirelayers [i];
          SUINFO(np) << "  " << wirelayer->to_str() << std::endl;
        }

        const suLayer * vialayer = _vialayers [i];
        SUINFO(np) << "  " << vialayer->to_str() << std::endl;
      }

      SUINFO(np) << "wire2: " << _wire2->to_str() << std::endl;
    }

    SUASSERT (_viaFields.empty(), "");
    
    const sutype::grids_t & grids = suSatRouter::instance()->_grids;
    SUASSERT (!grids.empty(), "");

    suRectangle defaulttransitionrect;
    calculate_transition_region_ (_wire1, _wire2, defaulttransitionrect);

    const suRectangle & extrabbox = suGlobalRouter::instance()->get_region(0,0)->bbox();
    const sutype::dcoord_t extray = (extrabbox.h() * 0) / 100;
    const sutype::dcoord_t extrax = (extrabbox.w() * 0) / 100;
    
    // debug only: dump defaulttransitionrect to Genesys
    if (np) {
      
      suErrorManager::instance()->add_error (std::string ("trrect_") + suStatic::dcoord_to_str (_id),
                                             defaulttransitionrect);

      suErrorManager::instance()->add_error (std::string ("trrect_") + suStatic::dcoord_to_str (_id),
                                             _wire1->rect());

      suErrorManager::instance()->add_error (std::string ("trrect_") + suStatic::dcoord_to_str (_id),
                                             _wire2->rect());
    }
    //
    
    suRectangle prevtransitionrect = defaulttransitionrect;
    
    SUINFO(np) << "Found " << _vialayers.size() << " via layers" << std::endl;

    for (sutype::uvi_t i=0; i < _vialayers.size(); ++i) {
      
      const suLayer * vialayer = _vialayers[i];
      
      suViaField * viafield = new suViaField (vialayer);
      _viaFields.push_back (viafield);
      
      suRectangle transitionrect = defaulttransitionrect;
      
      // a line of vias at the start
      if (createLineOfViasForFirstAndLastViaFields && i == 0) {

        sutype::dcoord_t centerline = _wire1->sidec();
        
        if (_layer1->pgd() == sutype::go_ver) {
          transitionrect.xl (centerline);
          transitionrect.xh (centerline);
          if (i+1 != _vialayers.size()) { // i.e. we have 1+ via fields
            transitionrect.yl (transitionrect.yl() - extray);
            transitionrect.yh (transitionrect.yh() + extray);
          }
        }
        else {
          transitionrect.yl (centerline);
          transitionrect.yh (centerline);
          if (i+1 != _vialayers.size()) { // i.e. we have 1+ via fields
            transitionrect.xl (transitionrect.xl() - extrax);
            transitionrect.xh (transitionrect.xh() + extrax);
          }
        }
      }

      // a line of vias at the end
      if (createLineOfViasForFirstAndLastViaFields && i+1 == _vialayers.size()) {
        
        sutype::dcoord_t centerline = _wire2->sidec();
        
        if (_layer2->pgd() == sutype::go_ver) {
          transitionrect.xl (centerline);
          transitionrect.xh (centerline);
          if (i != 0) { // i.e. we have 1+ via fields
            transitionrect.yl (transitionrect.yl() - extray);
            transitionrect.yh (transitionrect.yh() + extray);
          }
        }
        else {
          transitionrect.yl (centerline);
          transitionrect.yh (centerline);
          if (i != 0) { // i.e. we have 1+ via fields
            transitionrect.xl (transitionrect.xl() - extrax);
            transitionrect.xh (transitionrect.xh() + extrax);
          }
        }
      }

      // a rectangle of vias in the middle
      if (!createLineOfViasForFirstAndLastViaFields || (i > 0 && i+1 < _vialayers.size())) {
        
        const suLayer * wirelayer = _wirelayers [i];
        
        if (wirelayer->pgd() == sutype::go_ver) {
          transitionrect.yl (transitionrect.yl() - extray);
          transitionrect.yh (transitionrect.yh() + extray);
        }
        else {
          transitionrect.xl (transitionrect.xl() - extrax);
          transitionrect.xh (transitionrect.xh() + extrax);
        }
      }
      
      if (1) {

        if (i > 0) {

          const suLayer * wirelayer = _wirelayers [i];

          if (wirelayer->pgd() == sutype::go_ver) {
            transitionrect.xl (prevtransitionrect.xl());
            transitionrect.xh (prevtransitionrect.xh());
          }
          else {
            transitionrect.yl (prevtransitionrect.yl());
            transitionrect.yh (prevtransitionrect.yh());
          }
        }
        
        prevtransitionrect = transitionrect;
      }
      
      // find matching grids
      //SUINFO(1) << "#grids: " << grids.size() << std::endl;

      SUINFO(np)
        << "Find via options"
        << "; viafield=" << viafield->to_str()
        << "; defaulttransitionrect=" << defaulttransitionrect.to_str()
        << "; transitionrect=" << transitionrect.to_str()
        << std::endl;

      for (const auto & iter : grids) {
        
        const suGrid * grid = iter;
        
        const sutype::generators_tc & generators = grid->get_generators_of_base_layer (vialayer);
        if (generators.empty()) continue;

        // extend transitionrect a bit
        int dxl = -1;
        int dyl = -1;
        int dxh = 1;
        int dyh = 1;
        
        if (transitionrect.w() == 0) {
          dxl = dxh = 0;
        }
        
        if (transitionrect.h() == 0) {
          dyl = dyh = 0;
        }

        // keep default transitionrect
        dxl = dyl = dxh = dyh = 0;
        
        sutype::dcoords_t xdcoords;
        sutype::dcoords_t ydcoords;
        grid->get_dcoords (transitionrect, xdcoords, ydcoords, dxl, dyl, dxh, dyh);
        
        const suMetalTemplateInstance * vermti = grid->get_mti (sutype::go_ver);
        const suMetalTemplateInstance * hormti = grid->get_mti (sutype::go_hor);

        SUASSERT (hormti, "");
        SUASSERT (vermti, "");

        SUINFO(np)
          << "  Found a grid"
          << ": xdcoords=" << suStatic::to_str (xdcoords)
          << "; ydcoords=" << suStatic::to_str (ydcoords)
          << "; vermti=" << vermti->to_str()
          << "; hormti=" << hormti->to_str()
          << std::endl;

        // debug
        if (0) {

          for (const auto & iter1 : xdcoords) {

            sutype::dcoord_t xc = iter1;
            sutype::dcoord_t xl = xc - 10;
            sutype::dcoord_t xh = xc + 10;
            sutype::dcoord_t yl = ydcoords.front();
            sutype::dcoord_t yh = ydcoords.back();
            
            suErrorManager::instance()->add_error (std::string ("vf_") + suStatic::dcoord_to_str (_id) + "_" + viafield->layer()->name(),
                                                   xl, yl, xh, yh);
          }

          for (const auto & iter1 : ydcoords) {

            sutype::dcoord_t yc = iter1;
            sutype::dcoord_t yl = yc - 10;
            sutype::dcoord_t yh = yc + 10;
            sutype::dcoord_t xl = xdcoords.front();
            sutype::dcoord_t xh = xdcoords.back();
            
            suErrorManager::instance()->add_error (std::string ("vf_") + suStatic::dcoord_to_str (_id) + "_" + viafield->layer()->name(),
                                                   xl, yl, xh, yh);
          }
        }
        // end of debug
        
        for (const auto & iter1 : xdcoords) {
          sutype::dcoord_t dx = iter1;
          for (const auto & iter2 : ydcoords) {
            sutype::dcoord_t dy = iter2;
            if (!vermti->region().has_point (dx, dy)) continue;
            if (!hormti->region().has_point (dx, dy)) continue;
            if (!point_is_in_tunnel_ (dx, dy)) continue;

            sutype::generatorinstances_t gis = create_generator_instances_ (grid, dx, dy);
            if (gis.empty()) continue;

            viafield->add_via_option (sutype::viaoption_t (dx, dy, gis));
          }
        }
      }

      if (createLineOfViasForFirstAndLastViaFields && viafield->viaOptions().empty()) {
        SUINFO(np) << "No options remained for viafield=" << viafield->to_str() << std::endl;
        return false;
      }
    }
    
    SUASSERT (_viaFields.size() == _vialayers.size(), "");
    
    return true;
    
  } // end of suRouteGenerator::create_via_fields_
  
  //
  bool suRouteGenerator::convert_via_fields_to_sat_problem_ (suLayoutFunc * layoutfunc2)
  {
    const bool np = (_id == _debugid);

    SUASSERT (_viaFields.size() == _vialayers.size(), "");
    
    bool ok = true;

    for (int modeemit = 1; modeemit <= 2; ++modeemit) {

      const bool emit = (modeemit == 2) ? true : false;
      
      bool modified = false;
      
      if (1 && ok) {
        ok = sat_problem_create_first_and_last_wires_ (layoutfunc2, emit, modified);
        if (!ok) {
          SUINFO(np) << "A problem found during sat_problem_create_first_and_last_wires_" << std::endl;
          break;
        }
      }

      if (1 && ok) {
        
        // new (disabled for a while)
        if (0 && _optionRouteFullBbox) {
          ok = sat_problem_lower_via_cannot_appear_without_another_via_ (layoutfunc2, emit, modified);
          if (!ok) {
            SUINFO(np) << "A problem found during sat_problem_lower_via_cannot_appear_without_another_via_" << std::endl;
            break;
          }
        }
        
        // old
        else {
          ok = sat_problem_lower_via_cannot_appear_without_at_least_one_upper_via_at_the_same_dcoord_ (layoutfunc2, emit, modified);
          if (!ok) {
            SUINFO(np) << "A problem found during sat_problem_lower_via_cannot_appear_without_at_least_one_upper_via_at_the_same_dcoord_" << std::endl;
            break;
          }
        }
      }
      
      if (1 && ok) {
        ok = sat_problem_vias_on_adjacent_via_fields_cannot_apper_without_a_wire_in_between_ (layoutfunc2, emit, modified);
        if (!ok) {
          SUINFO(np) << "A problem found during sat_problem_vias_on_adjacent_via_fields_cannot_apper_without_a_wire_in_between_" << std::endl;
          break;
        }
      }
      
      SUASSERT (!emit || !modified, "");
      
      if (modified) modeemit = 0;
    }
    
    if (1 && ok) {
      bool minNumVias = true;
      bool maxNumVias = _optionRouteFullBbox;
      ok = sat_problem_every_via_field_must_have_at_least_one_via_ (layoutfunc2, minNumVias, maxNumVias);
      SUASSERT (ok, "");
    }

//     if (0 && ok) {
//       ok = sat_problem_every_layer_may_have_only_one_shunt_wire_ (layoutfunc2);
//       SUASSERT (ok, "");
//     }
    
    // this is an unfeasible routing option
    if (!ok) {
      layoutfunc2->add_leaf (suSatSolverWrapper::instance()->get_constant (0));
      return false;
    }

    sutype::satindex_t routeenabler = suSatSolverWrapper::instance()->get_next_sat_index ();
    _routeEnablers.push_back (routeenabler);
    layoutfunc2->add_leaf (routeenabler);
    
    return true;
    
  } // end of suRouteGenerator::convert_via_fields_to_sat_problem_

  // create pair constraints: a via can't appear without another via at the same coord one level upper/below   
  bool suRouteGenerator::sat_problem_lower_via_cannot_appear_without_at_least_one_upper_via_at_the_same_dcoord_ (suLayoutFunc * layoutfunc2,
                                                                                                                 const bool emit,
                                                                                                                 bool & modified)
  {
    const bool np = (_id == _debugid);

    SUASSERT (layoutfunc2, "");
    SUASSERT (layoutfunc2->func() == sutype::logic_func_and, "");
    SUASSERT (layoutfunc2->sign() == sutype::unary_sign_just, "");

    for (int modeemit = 1; modeemit <= 2; ++modeemit) {

      // emit is not allowed
      if (modeemit == 2 && !emit) break;
    
      for (sutype::uvi_t v=0; v+1 < _viaFields.size(); ++v) {
      
        sutype::grid_orientation_t currgd = _wirelayers[v+1]->pgd();

        const sutype::uvi_t v0 = v;
        const sutype::uvi_t v1 = v + 1;
      
        // dx for currgd=ver
        // dy for currgd=hor
        std::map<sutype::dcoord_t,sutype::clause_t> satindicesPerDcoord0 = get_dcoords_and_satindices_ (v0, currgd);
        std::map<sutype::dcoord_t,sutype::clause_t> satindicesPerDcoord1 = get_dcoords_and_satindices_ (v1, currgd);
        
        std::set<sutype::dcoord_t> commoncoords;
        
        for (const auto & iter : satindicesPerDcoord0) {
          sutype::dcoord_t dcoord = iter.first;
          if (satindicesPerDcoord1.count (dcoord) == 0) continue;
          commoncoords.insert (dcoord);
        }

        if (commoncoords.empty()) {
          SUINFO(np)
            << "Found no common coords"
            << "; currgd=" << suStatic::grid_orientation_2_str (currgd)
            << "; " << ((currgd == sutype::go_ver) ? "dx" : "dy")
            << "; v0=" << v0
            << "; v1=" << v1
            << "; vf0=" << _viaFields[v0]->to_str()
            << "; vf1=" << _viaFields[v1]->to_str()
            << std::endl;
          return false;
        }
      
        // remove vias at some dcoords that don't have any match
        for (int mode = 0; mode <= 1; ++mode) {
        
          std::map<sutype::dcoord_t,sutype::clause_t> & satindicesPerDcoord = (mode == 0) ? satindicesPerDcoord0 : satindicesPerDcoord1;
          if (satindicesPerDcoord.size() == commoncoords.size()) continue; // nothing to remove; no unique coords
        
          // collect unique coords -- unique coords belong only to satindicesPerDcoord0/1 and don't belong to satindicesPerDcoord1/0
          std::set<sutype::dcoord_t> uniquecoords;
          for (const auto & iter : satindicesPerDcoord) {
            sutype::dcoord_t dcoord = iter.first;
            if (commoncoords.count (dcoord) > 0) continue;
            uniquecoords.insert (dcoord);
          }
          SUASSERT (uniquecoords.size() + commoncoords.size() == satindicesPerDcoord.size(), "");

          const sutype::uvi_t v = (mode == 0) ? v0 : v1;
        
          // remove unique coords
          for (const auto & iter : uniquecoords) {
          
            sutype::dcoord_t dcoord = iter;
          
            // remove locally
            SUASSERT (satindicesPerDcoord.count (dcoord) != 0, "");
            satindicesPerDcoord.erase (dcoord);
            SUASSERT (satindicesPerDcoord.count (dcoord) == 0, "");

            // remove globally
            remove_via_options_ (v, dcoord, currgd);
            SUASSERT (!emit, "");
            modified = true;
            SUASSERT (modeemit == 0 || modeemit == 1, "");
            modeemit = 0;
          }
        }
      
        SUASSERT (satindicesPerDcoord0.size() == commoncoords.size(), "");
        SUASSERT (satindicesPerDcoord1.size() == commoncoords.size(), "");

        // not ready to emit
        if (modeemit != 2) {
          continue;
        }
        
        // emit to sat
        for (const auto & iter : commoncoords) {
          
          sutype::dcoord_t dcoord = iter;
          SUASSERT (satindicesPerDcoord0.count (dcoord) > 0, "");
          SUASSERT (satindicesPerDcoord1.count (dcoord) > 0, "");

          // tofix?? originally it was <= 1; I made it <= 0
          for (int mode = 0; mode <= 0; ++mode) {
        
            const sutype::clause_t & clause0 = (mode == 0) ? satindicesPerDcoord0.at (dcoord) : satindicesPerDcoord1.at (dcoord);
            const sutype::clause_t & clause1 = (mode == 0) ? satindicesPerDcoord1.at (dcoord) : satindicesPerDcoord0.at (dcoord);
          
            SUASSERT (!clause0.empty(), "");
            SUASSERT (!clause1.empty(), "");
          
            // clause0 cannot appear wihout clause1
            suLayoutFunc * layoutfunc3 = layoutfunc2->add_func (sutype::logic_func_or, sutype::unary_sign_just, SUFILELINE_DLF);
            
            suLayoutFunc * layoutfunc4 = layoutfunc3->add_func (sutype::logic_func_or, sutype::unary_sign_not, SUFILELINE_DLF);

            // every satindex is a gi
            for (const auto & iter2 : clause0) {
              layoutfunc4->add_leaf (iter2);
            }
            
            // every satindex is a gi
            for (const auto & iter2 : clause1) {
              layoutfunc3->add_leaf (iter2);
            }
            
          }
        }
      }
      
    } // for(modeemit)
    
    SUASSERT (!emit || !modified, "");

    return true;

  } // end of suRouteGenerator::sat_problem_lower_via_cannot_appear_without_at_least_one_upper_via_at_the_same_dcoord_

  // two vias at levels v and v+1 can't appear without a wire in between
  bool suRouteGenerator::sat_problem_vias_on_adjacent_via_fields_cannot_apper_without_a_wire_in_between_ (suLayoutFunc * layoutfunc2,
                                                                                                          const bool emit,
                                                                                                          bool & modified)
  {
    const bool np = (_id == _debugid);
    const bool dumpwires = (0 && np);

    SUASSERT (layoutfunc2, "");
    SUASSERT (layoutfunc2->func() == sutype::logic_func_and, "");
    SUASSERT (layoutfunc2->sign() == sutype::unary_sign_just, "");
    SUASSERT (!_viaFields.empty(), "");
    
    for (int modeemit = 1; modeemit <= 2; ++modeemit) {

      // emit is not allowed
      if (modeemit == 2 && !emit) break;

      // pre-compule all via fields
      std::vector<std::map<sutype::dcoordpair_t,sutype::clause_t> > satindicesPerDcoordAll;
      for (sutype::uvi_t v=0; v < _viaFields.size(); ++v) {
        satindicesPerDcoordAll.push_back (get_dcoords_and_satindices_ (v));
      }
      
      for (sutype::uvi_t v=0; v+1 < _viaFields.size(); ++v) {

        SUASSERT (v+1 < _wirelayers.size(), "");

        sutype::uvi_t wireLayerIndex = v+1;
        
        const suLayer * wirelayer = _wirelayers [wireLayerIndex];
        sutype::grid_orientation_t currgd = wirelayer->pgd();
        
        const sutype::uvi_t v0 = v;
        const sutype::uvi_t v1 = v + 1;

        const std::map<sutype::dcoordpair_t,sutype::clause_t> & satindicesPerDxDy0 = satindicesPerDcoordAll[v0]; // get_dcoords_and_satindices_ (v0);
        const std::map<sutype::dcoordpair_t,sutype::clause_t> & satindicesPerDxDy1 = satindicesPerDcoordAll[v1]; // get_dcoords_and_satindices_ (v1);
        
        if (satindicesPerDxDy0.empty()) return false;
        if (satindicesPerDxDy1.empty()) return false;

        const sutype::wires_t & allNetPreroutes = _net->wires();
        
        // find any preroutes of wirelayer
        sutype::wires_t netPreroutesOfTargetLayerSelection0;
        
        for (const auto & iter0 : allNetPreroutes) {
          suWire * netwire = iter0;
          if (netwire->layer()->base() != wirelayer->base()) continue;
          netPreroutesOfTargetLayerSelection0.push_back (netwire);
        }
        
        // Feb 12, 2019; this's a very good approximation but prerouted wires are not always feasible because of difffent reasons, e.g. via rules
        // disabled for a while
        netPreroutesOfTargetLayerSelection0.clear();
        
        // find even better preroutes on top of prerouted _wire1 and _wire2
        sutype::wires_t netPreroutesOfTargetLayerSelection1;
        
        for (const auto & iter0 : netPreroutesOfTargetLayerSelection0) {
          
          suWire * netwire = iter0;
          
          for (const auto & iter1 : _wires1and2) {
            
            suWire * termwire = iter1;
            SUASSERT (termwire == _wire1 || termwire == _wire2, "");
            
            if (!termwire->is (sutype::wt_preroute)) continue;
            if (termwire->layer()->pgd() == netwire->layer()->pgd()) continue;
            if (!termwire->rect().has_at_least_common_point (netwire->rect())) continue;

            netPreroutesOfTargetLayerSelection1.push_back (netwire);
            break;
          }
        }
        
        sutype::wires_t & netPreroutesOfTargetLayer = (!netPreroutesOfTargetLayerSelection1.empty()) ? netPreroutesOfTargetLayerSelection1 : netPreroutesOfTargetLayerSelection0;
        
        bool foundWireThatTouchesExistingPreroute = false;

        // if at least one wire touches one of preroutes - I leave only wires that touch existing preroutes
        // modeCheckWithPreroutes 1: check if we have at least one wire that touch one of preroutes
        // modeCheckWithPreroutes 2: ordinary execution but if foundWireThatTouchesExistingPreroute I trim wires those don't touch any preroutes
        for (int modeCheckWithPreroutes = 1; modeCheckWithPreroutes <= 2; ++modeCheckWithPreroutes) {

          SUINFO(np) << "####################################################################" << std::endl;
          SUINFO(np) << "emit (parameter): " << emit << std::endl;
          SUINFO(np) << "modeemit (iter): " << modeemit << std::endl;
          SUINFO(np) << "modeCheckWithPreroutes (iter): " << modeCheckWithPreroutes << std::endl;
          SUINFO(np) << "foundWireThatTouchesExistingPreroute (flag): " << foundWireThatTouchesExistingPreroute << std::endl;

          if (modeCheckWithPreroutes == 1) {
            SUASSERT (foundWireThatTouchesExistingPreroute == false, "Must be false here");
            if (netPreroutesOfTargetLayer.empty()) continue; // nothing to do
          }
          else if (modeCheckWithPreroutes == 2) {
            SUASSERT (foundWireThatTouchesExistingPreroute == (!netPreroutesOfTargetLayer.empty()), "");
          }
          else {
            SUASSERT (false, "");
          }
          
          for (const auto & iter0 : satindicesPerDxDy0) {

            if (modeCheckWithPreroutes == 1 && foundWireThatTouchesExistingPreroute) break; // nothing to find more

            const sutype::dcoordpair_t & dxdy0   = iter0.first;
            const sutype::clause_t & satindices0 = iter0.second;
            sutype::dcoord_t dx0 = dxdy0.first;
            sutype::dcoord_t dy0 = dxdy0.second;

            sutype::uvi_t numfeasiblewires = 0;

            for (const auto & iter1 : satindicesPerDxDy1) {
              
              // dummy check
              if (modeCheckWithPreroutes == 1) {
                SUASSERT (foundWireThatTouchesExistingPreroute == false, "");
              }

              const sutype::dcoordpair_t & dxdy1   = iter1.first;
              const sutype::clause_t & satindices1 = iter1.second;
              sutype::dcoord_t dx1 = dxdy1.first;
              sutype::dcoord_t dy1 = dxdy1.second;

              if (currgd == sutype::go_ver && dx1 != dx0) continue;
              if (currgd == sutype::go_hor && dy1 != dy0) continue;
          
              sutype::dcoord_t edge0 = (currgd == sutype::go_ver) ? dy0 : dx0;
              sutype::dcoord_t edge1 = (currgd == sutype::go_ver) ? dy1 : dx1;
          
              sutype::dcoord_t edgel = std::min (edge0, edge1);
              sutype::dcoord_t edgeh = std::max (edge0, edge1);

              sutype::dcoord_t sidec = (currgd == sutype::go_ver) ? dx0 : dy0;                                                                          

              // a shunt wire in between
              suWire * wire = suMetalTemplateManager::instance()->create_wire_to_match_metal_template_instance (_net,
                                                                                                                wirelayer,
                                                                                                                edgel,
                                                                                                                edgeh,
                                                                                                                sidec,
                                                                                                                sidec,
                                                                                                                sutype::wt_shunt);
              wire = serve_just_created_wire_ (wire);
              
              sutype::satindex_t wiresatindex = sutype::UNDEFINED_SAT_INDEX;

              if (!wire) {
                wiresatindex = suSatSolverWrapper::instance()->get_constant (0);
              }
              
              // if at least one wire touches one of preroutes - I leave only wires that touch existing preroutes
              if (wire && !netPreroutesOfTargetLayer.empty()) {

                if        (modeCheckWithPreroutes == 1) { SUASSERT (foundWireThatTouchesExistingPreroute == false, "");
                } else if (modeCheckWithPreroutes == 2) { SUASSERT (foundWireThatTouchesExistingPreroute == true,  "");
                } else {
                  SUASSERT (false, "");
                }
                
                bool wireTouchesExistingPreroute = false;

                for (const auto & iter2 : netPreroutesOfTargetLayer) {

                  suWire * netwire = iter2;
                  
                  if (netwire->rect().has_at_least_common_point (wire->rect()) &&
                      netwire->layer() == wire->layer() &&
                      netwire->sidel() == wire->sidel() &&
                      netwire->sideh() == wire->sideh()) {

                    wireTouchesExistingPreroute = true;
                    break;
                  }
                }

                if (modeCheckWithPreroutes == 1) {
                  if (wireTouchesExistingPreroute) {
                    SUASSERT (foundWireThatTouchesExistingPreroute == false, "");
                    foundWireThatTouchesExistingPreroute = true;
                    //SUINFO(1) << "foundWireThatTouchesExistingPreroute" << std::endl;
                  }
                  else {
                    // it's ok
                  }
                }
                
                else if (modeCheckWithPreroutes == 2) {
                  if (wireTouchesExistingPreroute) {
                    // do nothing; wire is legal
                  }
                  else {
                    SUASSERT (foundWireThatTouchesExistingPreroute, "");
                    //SUINFO(1) << "Pruned wire that doesnot touch any existing preroute." << std::endl;
                    suWireManager::instance()->release_wire (wire);
                    wiresatindex = suSatSolverWrapper::instance()->get_constant (0);
                    wire = 0;
                  }
                }

                else {
                  SUASSERT (false, "");
                }
              }

              if (wire) {
                ++numfeasiblewires;
                SUINFO(np) << "  Found feasible wire: " << wire->to_str() << std::endl;

                // debug
                if (dumpwires) {
                  std::string debugnetname (_net->name() + "_debug");
                  suNet * debugnet = suCellManager::instance()->topCellMaster()->create_net_if_needed (debugnetname);
                  debugnet->add_wire (suWireManager::instance()->create_wire_from_wire (debugnet, wire)); // final wire
                }
                //
              }
              
              // do not emit is this mode
              if (modeCheckWithPreroutes == 1) {
                
                if (wire) {
                  suWireManager::instance()->release_wire (wire);
                }
                
                if (foundWireThatTouchesExistingPreroute)
                  break;
                else
                  continue;
              }
            
              // not ready to emit
              if (modeemit != 2) {
                if (wire) {
                  suWireManager::instance()->release_wire (wire);
                }
                continue;
              }
              
              if (wire) {
                
                bool wireIsCoveredByPreroute = _net->wire_is_covered_by_a_preroute (wire);

                // release a wire; use constant_1
                if (wireIsCoveredByPreroute) {
                  
                  //SUISSUE("Found a redundant wire") << ": " << wire->to_str() << std::endl;
                  
                  suWireManager::instance()->release_wire (wire);
                  wiresatindex = suSatSolverWrapper::instance()->get_constant (1);
                  wire = 0;
                }
                
                // create a new wire
                else {

                  SUINFO(np)
                    << "Added a wire: " << wire->to_str()
                    << "; wirelayer=" << wirelayer->to_str()
                    << "; v0=" << v0
                    << "; v1=" << v1
                    << "; dx0=" << dx0
                    << "; dy0=" << dy0
                    << "; dx1=" << dx1
                    << "; dy1=" << dy1
                    << std::endl;
                
                  _route->add_wire (wire); // a viafiedld's wire (sutype::wt_shunt)
                
                  if (wire->satindex() == sutype::UNDEFINED_SAT_INDEX) {
                    suSatRouter::instance()->register_a_sat_wire (wire);
                  }
                  wiresatindex = wire->satindex();
                }
              }
              else {
                SUASSERT (suSatSolverWrapper::instance()->is_constant (0, wiresatindex), "");
              }
              
              suLayoutFunc * layoutfunc3 = layoutfunc2->add_func (sutype::logic_func_or, sutype::unary_sign_just, SUFILELINE_DLF);
              
              // a solid wire between two target vias
              layoutfunc3->add_leaf (wiresatindex);

              // disabled for a while; creates opens
              if (0) {
                
                // other vias between two target vias
                // Here, I want to collect all vias those are electrically connected to wirelayer and within those bounds
                // wirelayer
                // currgd (ver/hor)
                // sidec (ver: x; hor: y)
                // edgel (ver: y; hor: x)
                // edgeh (ver: y; hor: x)
                sutype::clause_t & satindices2 = get_satindices_in_bounds_ (satindicesPerDcoordAll,
                                                                            wirelayer,
                                                                            v+1,
                                                                            sidec,
                                                                            edgel,
                                                                            edgeh);

                //SUINFO(1) << "Found " << satindices2.size() << " vias in between." << std::endl;
              
                for (const auto & iter2 : satindices2) {
                  layoutfunc3->add_leaf (iter2);
                }
                
                suClauseBank::return_clause (satindices2);
              }
              
              suLayoutFunc * layoutfunc4 = layoutfunc3->add_func (sutype::logic_func_and, sutype::unary_sign_not, SUFILELINE_DLF);

              // every satindex is a gi
              suLayoutFunc * layoutfunc50 = layoutfunc4->add_func (sutype::logic_func_or, sutype::unary_sign_just, SUFILELINE_DLF);
              for (const auto & iter2 : satindices0) {
                layoutfunc50->add_leaf (iter2);
              }

              // every satindex is a gi
              suLayoutFunc * layoutfunc51 = layoutfunc4->add_func (sutype::logic_func_or, sutype::unary_sign_just, SUFILELINE_DLF);
              for (const auto & iter2 : satindices1) {
                layoutfunc51->add_leaf (iter2);
              }
            }

            if (modeCheckWithPreroutes == 1) {
              continue; // do nothing
            }

            if (numfeasiblewires != 0) continue;

            // Found an unfeasible via option
            remove_via_option_ (v0, dx0, dy0);
            SUASSERT (!emit, "");
            modified = true;
            SUASSERT (modeemit == 0 || modeemit == 1, "");
            modeemit = 0;
          }

          // debug
          if (dumpwires) {
            SULABEL("");
            suCellManager::instance()->dump_out_file ("out/" + suCellManager::instance()->topCellMaster()->name() + ".lgf");
            SUABORT;
          }
          //

          // clear the list as useless
          if (modeCheckWithPreroutes == 1 && !foundWireThatTouchesExistingPreroute) {
            
            if (0 && emit) {
              SUISSUE("Net has preroutes but none of wires touch these preroutes")
                << ": Net " << _net->name() << " has " << netPreroutesOfTargetLayer.size() << " preroutes on layer " << wirelayer->name() << " but none of wires touch these preroutes"
                << "; id=" << _id
                << "; wire1=" << _wire1->to_str()
                << "; wire2=" << _wire2->to_str()
                << std::endl;
              
              SUASSERT (false, "");
            }
            
            netPreroutesOfTargetLayer.clear();
          }
          
        } // for(modeCheckWithPreroutes)
      }
    }

    SUASSERT (!emit || !modified, "");

    return true;
    
  } // end of suRouteGenerator::sat_problem_vias_on_adjacent_via_fields_cannot_apper_without_a_wire_in_between_

  // the first wire & the last wire connect _wire1 and _wire2 with vias
  bool suRouteGenerator::sat_problem_create_first_and_last_wires_ (suLayoutFunc * layoutfunc2, // top-level AND for this route
                                                                   const bool emit,
                                                                   bool & modified)
  {
    SUASSERT (layoutfunc2, "");
    SUASSERT (layoutfunc2->func() == sutype::logic_func_and, "");
    SUASSERT (layoutfunc2->sign() == sutype::unary_sign_just, "");
    SUASSERT (!_viaFields.empty(), "");

    for (int modeemit = 1; modeemit <= 2; ++modeemit) {

      // emit is not allowed
      if (modeemit == 2 && !emit) break;

      // mode 1: extend _wire1
      // mode 2: extend _wire2
      for (int mode = 1; mode <= 2; ++mode) {

        //suLayoutFunc * layoutfunc3 = 0; // will be OR of options

        suWire * wire1or2     = (mode == 1) ? _wire1 : _wire2;
        const sutype::uvi_t v = (mode == 1) ? 0      : _viaFields.size()-1;
        
        const sutype::grid_orientation_t wirepgd = wire1or2->layer()->pgd();
        const sutype::grid_orientation_t wireogd = suStatic::invert_gd (wirepgd);
        
        //
        //    _wire1/_wire2
        //   +-------------+
        //   |             |
        // ------------------- sidec (center line) -----  +    +     +     +
        //   |             |                              (eges; via options)
        //   +-------------+
        //
        std::map<sutype::dcoord_t,sutype::clause_t> satindicesPerSide = get_dcoords_and_satindices_ (v, wirepgd);
        std::map<sutype::dcoord_t,sutype::clause_t> satindicesPerEdge = get_dcoords_and_satindices_ (v, wireogd);
        
        // no feasible vias
        if (satindicesPerEdge.empty()) {
          return false;
        }

        SUASSERT (satindicesPerSide.size() == 1, "Only one center line is expected");
      
        for (const auto & iter1 : satindicesPerEdge) {
        
          sutype::dcoord_t edge0              = iter1.first;
          const sutype::clause_t & satindices = iter1.second;
          SUASSERT (!satindices.empty(), "");
        
          // don't need a wire to connect _wire1/_wire2 with a via
          if (edge0 >= wire1or2->edgel() && edge0 <= wire1or2->edgeh()) {
            continue;
          }

          sutype::dcoord_t edgel = std::min (wire1or2->edgeh(), edge0);
          sutype::dcoord_t edgeh = std::max (wire1or2->edgel(), edge0);
          
          // don't extend M1 terminals
          if (0) {
            if (wire1or2->layer()->level() <= suLayerManager::instance()->lowestDRLayer()->level()) {
              remove_via_options_ (v, edge0, wireogd);
              modified = true;
              continue;
            }
          }
          
          SUASSERT (edgel < edgeh, "edge0=" << edge0 << "; wire1or2=" << wire1or2->to_str());
          SUASSERT (edge0 == edgel || edge0 == edgeh, "");
          
          // first/last wire
          suWire * wire = suWireManager::instance()->create_wire_from_edge_side (wire1or2->net(),
                                                                                 wire1or2->layer(),
                                                                                 edgel,
                                                                                 edgeh,
                                                                                 wire1or2->sidel(),
                                                                                 wire1or2->sideh(),
                                                                                 sutype::wt_route);

          wire = serve_just_created_wire_ (wire);
          
          // this case is covered by create_layout_fill_a_gap_between_input_wires_
          if (wire &&
              _layer1 == _layer2 && wire->layer() == _layer1 &&
              wire->rect().has_at_least_common_point (_wire1->rect()) &&
              wire->rect().has_at_least_common_point (_wire2->rect())) {
            
            suWireManager::instance()->release_wire (wire);
            wire = 0;
          }
          
          // gis at this dcoord=edge0 are not feasible
          if (!wire) {
            
            remove_via_options_ (v, edge0, wireogd);
            SUASSERT (!emit, "");
            modified = true;
            SUASSERT (modeemit == 0 || modeemit == 1, "");
            modeemit = 0;
            continue;
          }

          // not ready to emit
          if (modeemit != 2) {
            suWireManager::instance()->release_wire (wire);
            continue;
          }

          SUASSERT (wire, "");

          bool wireIsCoveredByPreroute = _net->wire_is_covered_by_a_preroute (wire);

          sutype::satindex_t wiresatindex = sutype::UNDEFINED_SAT_INDEX;
          
          // release a wire; use constant_1
          if (wireIsCoveredByPreroute) {
            
            //SUISSUE("Found a redundant wire") << ": " << wire->to_str() << std::endl;
            
            suWireManager::instance()->release_wire (wire);
            wiresatindex = suSatSolverWrapper::instance()->get_constant (1);
            wire = 0;
          }
          
          else {
          
            // time to emit
            // first/last wire
            _route->add_wire (wire); // first/last wire (sutype::wt_route)
            suSatRouter::instance()->register_a_sat_wire (wire);
            wiresatindex = wire->satindex();
          }
          
          // gis can't appear at dcoord=edge0 without a wire between _wire1/_wire2 and this dcoord=edge0
          suLayoutFunc * layoutfunc3 = layoutfunc2->add_func (sutype::logic_func_or, sutype::unary_sign_just, SUFILELINE_DLF);

          layoutfunc3->add_leaf (wiresatindex);

          // every satindex is a gi
          suLayoutFunc * layoutfunc4 = layoutfunc3->add_func (sutype::logic_func_or, sutype::unary_sign_not, SUFILELINE_DLF);
          for (const auto & iter2 : satindices) {
            layoutfunc4->add_leaf (iter2);
          }
          
        }
      }
    } // for(modeemit)
    
    SUASSERT (!emit || !modified, "");

    return true;
    
  } // end of suRouteGenerator::sat_problem_create_first_and_last_wires_

  // maxNumVias doesn't work; SAT goes out of memory; disabled for a while; there's maybe a bug in bc_less_or_equal
  bool suRouteGenerator::sat_problem_every_via_field_must_have_at_least_one_via_ (suLayoutFunc * layoutfunc2,
                                                                                  bool minNumVias,
                                                                                  bool maxNumVias)
  {
    std::map<const suLayer *, sutype::uvi_t, suLayer::cmp_const_ptr> numLayers;
    std::map<const suLayer *, sutype::clause_t, suLayer::cmp_const_ptr> viasPerLayer;

    for (sutype::uvi_t v=0; v < _viaFields.size(); ++v) {

      suViaField * vf = _viaFields[v];
      
      const suLayer * layer = vf->layer();
      SUASSERT (layer, "");
      SUASSERT (layer->is_base(), "");

      ++numLayers[layer];
      
      const sutype::viaoptions_t & vos = vf->viaOptions();

      suLayoutFunc * layoutfunc3 = 0;
      
      for (const auto & iter1 : vos) {
        
        const sutype::viaoption_t & vo = iter1;
        
        //sutype::dcoord_t dx = std::get<0>(vo);
        //sutype::dcoord_t dy = std::get<1>(vo);
        const sutype::generatorinstances_t & gis = std::get<2>(vo);
                
        for (const auto & iter2 : gis) {
          
          suGeneratorInstance * gi = iter2;
          SUASSERT (gi->legal(), "");
          
          sutype::satindex_t gisatindex = gi->layoutFunc()->satindex();
          SUASSERT (gisatindex != sutype::UNDEFINED_SAT_INDEX, "");

          // debug
          if (0) {
            
            suErrorManager::instance()->add_error (gi->generator()->get_cut_layer()->base()->name() + "_" + _net->name() + "_" + suStatic::dcoord_to_str(v),
                                                   gi->get_cut_wire()->rect());
          }

          viasPerLayer[layer].push_back (gisatindex);

          if (layoutfunc3 == 0)
            layoutfunc3 = layoutfunc2->add_func (sutype::logic_func_or, sutype::unary_sign_just, SUFILELINE_DLF);
          
          layoutfunc3->add_leaf (gisatindex);
        }
      }
      
      // via field has no feasible vias
      if (layoutfunc3 == 0) {
        SUASSERT (false, "");
        return false;
      }
    }

    for (const auto & iter0 : numLayers) {

      sutype::uvi_t count = iter0.second;
      SUASSERT (count > 0, "");
      SUASSERT (count == 1 || count == 2, "");
      if (count == 1) continue;

      const suLayer * layer = iter0.first;

      SUASSERT (viasPerLayer.count (layer) > 0, "");

      sutype::clause_t & satindices = viasPerLayer.at (layer);
      suStatic::sort_and_leave_unique (satindices);
      
      // not enough different vias
      if (satindices.size() < count) {
        layoutfunc2->add_leaf (suSatSolverWrapper::instance()->get_constant(0));
        continue;
      }

      // minimal count
      if (minNumVias) {
        sutype::layoutnodes_t nodes;
        for (sutype::uvi_t w=0; w < satindices.size(); ++w) {
          nodes.push_back (suLayoutNode::create_leaf (satindices[w]));
        }
        
        suLayoutFunc * layoutfunc3 = suLayoutNode::create_counter (sutype::bc_greater_or_equal, sutype::unary_sign_just, nodes, count);
        layoutfunc2->add_node (layoutfunc3);
      }

      // maximal count
      if (0 && maxNumVias) {
        sutype::layoutnodes_t nodes;
        for (sutype::uvi_t w=0; w < satindices.size(); ++w) {
          nodes.push_back (suLayoutNode::create_leaf (satindices[w]));
        }
        
        suLayoutFunc * layoutfunc3 = suLayoutNode::create_counter (sutype::bc_less_or_equal, sutype::unary_sign_just, nodes, count);
        layoutfunc2->add_node (layoutfunc3);
      }
    }
    
    return true;
    
  } // end of suRouteGenerator::sat_problem_every_via_field_must_have_at_least_one_via_
  
  //
  bool suRouteGenerator::sat_problem_lower_via_cannot_appear_without_another_via_ (suLayoutFunc * layoutfunc2, // top-level AND for this route
                                                                                   const bool emit,
                                                                                   bool & modified)
  {
    const bool np = (_id == _debugid);

    SUASSERT (layoutfunc2, "");
    SUASSERT (layoutfunc2->func() == sutype::logic_func_and, "");
    SUASSERT (layoutfunc2->sign() == sutype::unary_sign_just, "");

    std::map<sutype::satindex_t, std::map<sutype::id_t, sutype::clause_t> > mandatorySatindices;

    for (int modeemit = 1; modeemit <= 2; ++modeemit) {

      // emit is not allowed
      if (modeemit == 2 && !emit) break;
    
      for (sutype::uvi_t v=0; v+1 < _viaFields.size(); ++v) {
      
        sutype::grid_orientation_t currgd = _wirelayers[v+1]->pgd();

        const sutype::uvi_t v0 = v;
        const sutype::uvi_t v1 = v + 1;

        const sutype::id_t layerid0 = _viaFields[v0]->layer()->base()->pers_id();
        const sutype::id_t layerid1 = _viaFields[v1]->layer()->base()->pers_id();
      
        // dx for currgd=ver
        // dy for currgd=hor
        std::map<sutype::dcoord_t,sutype::clause_t> satindicesPerDcoord0 = get_dcoords_and_satindices_ (v0, currgd);
        std::map<sutype::dcoord_t,sutype::clause_t> satindicesPerDcoord1 = get_dcoords_and_satindices_ (v1, currgd);
        
        std::set<sutype::dcoord_t> commoncoords;
        
        for (const auto & iter : satindicesPerDcoord0) {
          sutype::dcoord_t dcoord = iter.first;
          if (satindicesPerDcoord1.count (dcoord) == 0) continue;
          commoncoords.insert (dcoord);
        }

        if (commoncoords.empty()) {
          SUINFO(np)
            << "Found no common coords"
            << "; currgd=" << suStatic::grid_orientation_2_str (currgd)
            << "; " << ((currgd == sutype::go_ver) ? "dx" : "dy")
            << "; v0=" << v0
            << "; v1=" << v1
            << "; vf0=" << _viaFields[v0]->to_str()
            << "; vf1=" << _viaFields[v1]->to_str()
            << std::endl;
          return false;
        }
      
        // remove vias at some dcoords that don't have any match
        for (int mode = 0; mode <= 1; ++mode) {
        
          std::map<sutype::dcoord_t,sutype::clause_t> & satindicesPerDcoord = (mode == 0) ? satindicesPerDcoord0 : satindicesPerDcoord1;
          if (satindicesPerDcoord.size() == commoncoords.size()) continue; // nothing to remove; no unique coords
        
          // collect unique coords -- unique coords belong only to satindicesPerDcoord0/1 and don't belong to satindicesPerDcoord1/0
          std::set<sutype::dcoord_t> uniquecoords;
          for (const auto & iter : satindicesPerDcoord) {
            sutype::dcoord_t dcoord = iter.first;
            if (commoncoords.count (dcoord) > 0) continue;
            uniquecoords.insert (dcoord);
          }
          SUASSERT (uniquecoords.size() + commoncoords.size() == satindicesPerDcoord.size(), "");

          const sutype::uvi_t v = (mode == 0) ? v0 : v1;
        
          // remove unique coords
          for (const auto & iter : uniquecoords) {
          
            sutype::dcoord_t dcoord = iter;
          
            // remove locally
            SUASSERT (satindicesPerDcoord.count (dcoord) != 0, "");
            satindicesPerDcoord.erase (dcoord);
            SUASSERT (satindicesPerDcoord.count (dcoord) == 0, "");

            // remove globally
            remove_via_options_ (v, dcoord, currgd);
            SUASSERT (!emit, "");
            modified = true;
            SUASSERT (modeemit == 0 || modeemit == 1, "");
            modeemit = 0; // set to 0 to start from scratch
          }
        }
        
        SUASSERT (satindicesPerDcoord0.size() == commoncoords.size(), "");
        SUASSERT (satindicesPerDcoord1.size() == commoncoords.size(), "");
        
        for (const auto & iter1 : commoncoords) {
          
          sutype::dcoord_t dcoord = iter1;
          SUASSERT (satindicesPerDcoord0.count (dcoord) > 0, "");
          SUASSERT (satindicesPerDcoord1.count (dcoord) > 0, "");

          for (int mode = 0; mode <= 1; ++mode) {
        
            const sutype::clause_t & clause0 = (mode == 0) ? satindicesPerDcoord0.at (dcoord) : satindicesPerDcoord1.at (dcoord);
            const sutype::clause_t & clause1 = (mode == 0) ? satindicesPerDcoord1.at (dcoord) : satindicesPerDcoord0.at (dcoord);
          
            SUASSERT (!clause0.empty(), "");
            SUASSERT (!clause1.empty(), "");

            //const sutype::id_t id0 = (mode == 0) ? layerid0 : layerid1;
            const sutype::id_t id1 = (mode == 0) ? layerid1 : layerid0;

            for (const auto & iter2 : clause0) {
              sutype::satindex_t satindex0 = iter2;
              for (const auto & iter3 : clause1) {
                sutype::satindex_t satindex1 = iter3;
                if (satindex1 == satindex0) continue;
                mandatorySatindices[satindex0][id1].push_back (satindex1);
              }
            }
          }
        }
      }
    }
      
    for (auto & iter0 : mandatorySatindices) {
      for (auto & iter1 : iter0.second) {
      
        sutype::clause_t & satindices1 = iter1.second;
        suStatic::sort_and_leave_unique (satindices1);
      }
    }
    
    if (emit) {
      
      for (const auto & iter0 : mandatorySatindices) {

        sutype::satindex_t satindex0 = iter0.first;
        const std::map<sutype::id_t, sutype::clause_t> & mandatorySatindicesPerLayer = iter0.second;

        // via can't appear without a via on every adjacent layer
        suLayoutFunc * layoutfunc3 = layoutfunc2->add_func (sutype::logic_func_or, sutype::unary_sign_just, SUFILELINE_DLF);
        
        layoutfunc3->add_leaf (-satindex0);

        suLayoutFunc * layoutfunc4 = layoutfunc3->add_func (sutype::logic_func_and, sutype::unary_sign_just, SUFILELINE_DLF);

        for (const auto & iter1 : mandatorySatindicesPerLayer) {
          
          const sutype::clause_t & layersatindices1 = iter1.second;
          suLayoutFunc * layoutfunc5 = layoutfunc4->add_func (sutype::logic_func_or, sutype::unary_sign_just, SUFILELINE_DLF);
          for (const auto & iter2 : layersatindices1) {
            layoutfunc5->add_leaf (iter2);
          }
        }
      }
    }
    
    SUASSERT (!emit || !modified, "");
    
    return true;
    
  } // end of suRouteGenerator::sat_problem_lower_via_cannot_appear_without_another_via_

  //
  bool suRouteGenerator::sat_problem_every_layer_may_have_only_one_shunt_wire_ (suLayoutFunc * layoutfunc2)
  {
    SUASSERT (false, "obsolete");
    return true;

    /*
    std::map<const suLayer *, std::set<suWire*, suWire::cmp_ptr>, suLayer::cmp_const_ptr> wiresPerLayer;

    const sutype::wires_t & wires = _route->wires();

    for (const auto & iter : wires) {

      suWire * wire = iter;
      if (!wire->is (sutype::wt_shunt)) continue;
      
      wiresPerLayer [wire->layer()->base()].insert (wire);
    }

    const sutype::uvi_t maxnumwires = 1;

    for (const auto & iter1 : wiresPerLayer) {

      //const suLayer * layer = iter1.first;
      const auto & wires = iter1.second;
      SUASSERT (!wires.empty(), "");
      
      if (wires.size() <= maxnumwires) continue;

      //SUINFO(1) << layer->name() << "; #wires=" << wires.size() << std::endl;
      
      sutype::layoutnodes_t nodes;
      
      for (const auto & iter2 : wires) {

        suWire * wire = iter2;
        sutype::satindex_t satindex = wire->satindex();
        SUASSERT (wire != sutype::UNDEFINED_SAT_INDEX, "");
        nodes.push_back (suLayoutNode::create_leaf (satindex));
      }

      suLayoutFunc * layoutfunc3 = suLayoutNode::create_counter (sutype::bc_less_or_equal, sutype::unary_sign_just, nodes, maxnumwires);
      
      layoutfunc2->add_node (layoutfunc3);
    }

    //SUINFO(1) << "Done." << std::endl;
    
    return true;
    */
    
  } // end of suRouteGenerator::sat_problem_every_layer_may_have_only_one_shunt_wire_
  
  //
  void suRouteGenerator::calculate_transition_region_ (const suWire * wire1,
                                                       const suWire * wire2,
                                                       suRectangle & transitionrect)
    const
  {
    SUASSERT (wire1->is (sutype::wt_preroute) || wire1->is (sutype::wt_trunk), "");
    SUASSERT (wire2->is (sutype::wt_preroute) || wire2->is (sutype::wt_trunk), "");

    const suLayer * layer1 = wire1->layer();
    const suLayer * layer2 = wire2->layer();

    SUASSERT (layer1, "");
    SUASSERT (layer2, "");
    SUASSERT (layer1->pgd() == sutype::go_ver || layer1->pgd() == sutype::go_hor, "");
    SUASSERT (layer2->pgd() == sutype::go_ver || layer2->pgd() == sutype::go_hor, "");

    if (_optionRouteFullBbox) {
      calculate_transition_region_maximal_ (wire1, wire2, transitionrect);
    }
    else if (layer1->pgd() == layer2->pgd()) {
      calculate_transition_region_for_collineral_wires_ (wire1, wire2, transitionrect);
    }
    else {
      calculate_transition_region_for_perpendicular_wires_ (wire1, wire2, transitionrect);
    }

    const suRectangle & extrabbox = suGlobalRouter::instance()->get_region(0,0)->bbox();
    const sutype::dcoord_t extray = (extrabbox.h() * 0) / 100;
    const sutype::dcoord_t extrax = (extrabbox.w() * 0) / 100;
    
    sutype::dcoord_t h = transitionrect.h();
    sutype::dcoord_t w = transitionrect.w();

    if (h < extray) {
      sutype::dcoord_t dh = (extray - h) / 2;
      transitionrect.yl (transitionrect.yl() - dh);
      transitionrect.yh (transitionrect.yh() + dh);
    }

    if (w < extrax) {
      sutype::dcoord_t dw = (extrax - w) / 2;
      transitionrect.xl (transitionrect.xl() - dw);
      transitionrect.xh (transitionrect.xh() + dw);
    }
    
  } // end of suRouteGenerator::calculate_transition_region_

  void suRouteGenerator::calculate_transition_region_maximal_ (const suWire * wire1,
                                                               const suWire * wire2,
                                                               suRectangle & transitionrect)
    const
  {
    sutype::dcoord_t minx = std::min (wire1->rect().xl(), wire2->rect().xl());
    sutype::dcoord_t maxx = std::max (wire1->rect().xh(), wire2->rect().xh());
    sutype::dcoord_t miny = std::min (wire1->rect().yl(), wire2->rect().yl());
    sutype::dcoord_t maxy = std::max (wire1->rect().yh(), wire2->rect().yh());
    
    const suRectangle & extrabbox = suGlobalRouter::instance()->get_region(0,0)->bbox();
    const sutype::dcoord_t extray = (extrabbox.h() * 50) / 100;
    const sutype::dcoord_t extrax = (extrabbox.w() * 50) / 100;

    transitionrect.xl (minx - extrax);
    transitionrect.xh (maxx + extrax);
    transitionrect.yl (miny - extray);
    transitionrect.yh (maxy + extray);

    // debug
    if (0) {

      suErrorManager::instance()->add_error (std::string ("wire1_") + suStatic::dcoord_to_str (_id),
                                             wire1->rect());

      suErrorManager::instance()->add_error (std::string ("wire2_") + suStatic::dcoord_to_str (_id),
                                             wire2->rect());

      suErrorManager::instance()->add_error (std::string ("deftrrect_") + suStatic::dcoord_to_str (_id),
                                             transitionrect);
      
    }
 
  } // end of suRouteGenerator::calculate_transition_region_maximal_

  //
  void suRouteGenerator::calculate_transition_region_for_perpendicular_wires_ (const suWire * wire1,
                                                                               const suWire * wire2,
                                                                               suRectangle & transitionrect)
    const
  {
    const bool np = (_id == _debugid);

    SUINFO(np)
      << "calculate_transition_region_for_perpendicular_wires_:"
      << "; wire1=" << wire1->to_str()
      << "; wire2=" << wire2->to_str()
      << std::endl;

    SUASSERT (wire1->net() == wire2->net(), "");
    SUASSERT (wire1->net() != 0, "");

    const suLayer * layer1 = wire1->layer();
    const suLayer * layer2 = wire2->layer();
    SUASSERT (layer1->pgd() != layer2->pgd(), "");

    sutype::dcoord_t centerx = (layer1->pgd() == sutype::go_ver) ? wire1->sidec() : wire2->sidec();
    sutype::dcoord_t centery = (layer1->pgd() == sutype::go_ver) ? wire2->sidec() : wire1->sidec();

    const suRectangle & extrabbox = suGlobalRouter::instance()->get_region(0,0)->bbox();
    const sutype::dcoord_t extray = (extrabbox.h() * 50) / 100;
    const sutype::dcoord_t extrax = (extrabbox.w() * 50) / 100;

    transitionrect.xl (centerx - extrax);
    transitionrect.xh (centerx + extrax);
    transitionrect.yl (centery - extray);
    transitionrect.yh (centery + extray);

    // debug only
    sutype::dcoord_t transitionrect_xl = transitionrect.xl();
    sutype::dcoord_t transitionrect_xh = transitionrect.xh();
    sutype::dcoord_t transitionrect_yl = transitionrect.yl();
    sutype::dcoord_t transitionrect_yh = transitionrect.yh();

    // debug
    if (0) {

      suErrorManager::instance()->add_error (std::string ("wire1_") + suStatic::dcoord_to_str (_id),
                                             wire1->rect());

      suErrorManager::instance()->add_error (std::string ("wire2_") + suStatic::dcoord_to_str (_id),
                                             wire2->rect());

      suErrorManager::instance()->add_error (std::string ("deftrrect_") + suStatic::dcoord_to_str (_id),
                                             transitionrect);
      
    }

    // mode1 0: wireA = wire1; wireB = wire2;
    // mode1 1: wireA = wire2; wireB = wire1;
    for (int mode1 = 0; mode1 < 2; ++mode1) {

      const suWire * wireA = (mode1 == 0) ? wire1 : wire2;
      const suWire * wireB = (wireA == wire1) ? wire2 : wire1;

      if (!wireA->is (sutype::wt_preroute)) continue;

      const suLayer * layerA = wireA->layer();
      const suLayer * layerB = wireB->layer();

      const unsigned numskiptracks = 0;
      
      // mode2 0: check wires upper
      // mode2 1: check wires lower
      for (int mode2 = 0; mode2 < 1; ++mode2) {

        const bool shuntLayerIsUpper = (mode2 == 0) ? true : false;

        if (shuntLayerIsUpper == true  && layerB->level() < layerA->level()) continue;
        if (shuntLayerIsUpper == false && layerB->level() > layerA->level()) continue;
      
        const suLayer * shuntlayer = suLayerManager::instance()->get_base_adjacent_layer (layerA, sutype::lt_metal | sutype::lt_wire, shuntLayerIsUpper, numskiptracks);
        if (shuntlayer == 0) continue;
      
        sutype::wires_t wiresOnLayerB;

        const sutype::wires_t & netwires = wireA->net()->wires();
        //const sutype::generatorinstances_t & netgis = wireA->net()->generatorinstances(); // not implemented yet

        // find prerouted wires above and extend defaul transitionrect to be able to use these preroutes
        for (const auto & iter : netwires) {

          suWire * netwire = iter;
        
          if (netwire->layer()->base() != shuntlayer) continue;
          if (!netwire->rect().has_at_least_common_point (wireA->rect())) continue;

          SUASSERT (netwire->is (sutype::wt_preroute), netwire->to_str());

          // extend transition rect for vertical wire
          if (layerA->pgd() == sutype::go_ver) {
            transitionrect.yl (std::min (transitionrect.yl(), netwire->rect().yl()));
            transitionrect.yh (std::max (transitionrect.yh(), netwire->rect().yh()));
          }
          // extend transition rect for horizontal wire
          else {
            transitionrect.xl (std::min (transitionrect.xl(), netwire->rect().xl()));
            transitionrect.xh (std::max (transitionrect.xh(), netwire->rect().xh()));
          }
        }
      }
    }

    if (transitionrect_xl != transitionrect.xl()) { SUINFO(np) << "Extended transitionrect XL: previous=" << transitionrect_xl << "; present=" << transitionrect.xl() << std::endl; }
    if (transitionrect_xh != transitionrect.xh()) { SUINFO(np) << "Extended transitionrect XH: previous=" << transitionrect_xh << "; present=" << transitionrect.xh() << std::endl; }
    if (transitionrect_yl != transitionrect.yl()) { SUINFO(np) << "Extended transitionrect YL: previous=" << transitionrect_yl << "; present=" << transitionrect.yl() << std::endl; }
    if (transitionrect_yh != transitionrect.yh()) { SUINFO(np) << "Extended transitionrect YH: previous=" << transitionrect_yh << "; present=" << transitionrect.yh() << std::endl; }
    
  } // end of suRouteGenerator::calculate_transition_region_for_perpendicular_wires_

  //
  void suRouteGenerator::calculate_transition_region_for_collineral_wires_ (const suWire * wire1,
                                                                            const suWire * wire2,
                                                                            suRectangle & transitionrect)
    const
  {
    const suLayer * layer1 = wire1->layer();
    const suLayer * layer2 = wire2->layer();
    SUASSERT (layer1->pgd() == layer2->pgd(), "");
    
    sutype::dcoord_t edgel1 = wire1->edgel();
    sutype::dcoord_t edgeh1 = wire1->edgeh();
    sutype::dcoord_t sidec1 = wire1->sidec();

    sutype::dcoord_t edgel2 = wire2->edgel();
    sutype::dcoord_t edgeh2 = wire2->edgeh();
    sutype::dcoord_t sidec2 = wire2->sidec();
    
    sutype::dcoord_t regionsidel = std::min (sidec1, sidec2);
    sutype::dcoord_t regionsideh = std::max (sidec1, sidec2);

    sutype::dcoord_t e1 = std::max (edgel1, edgel2);
    sutype::dcoord_t e2 = std::min (edgeh1, edgeh2);

    sutype::dcoord_t regionedgel = std::min (e1, e2);
    sutype::dcoord_t regionedgeh = std::max (e1, e2);
    
    if (wire1->is (sutype::wt_preroute)) {
      regionedgel = std::min (wire1->edgel(), regionedgel);
      regionedgeh = std::max (wire1->edgeh(), regionedgeh);
    }
    
    if (wire2->is (sutype::wt_preroute)) {
      regionedgel = std::min (wire2->edgel(), regionedgel);
      regionedgeh = std::max (wire2->edgeh(), regionedgeh);
    }
    
    if (layer1->pgd() == sutype::go_ver) {

      transitionrect.xl (regionsidel);
      transitionrect.xh (regionsideh);
      transitionrect.yl (regionedgel);
      transitionrect.yh (regionedgeh);
    }

    else {

      transitionrect.yl (regionsidel);
      transitionrect.yh (regionsideh);
      transitionrect.xl (regionedgel);
      transitionrect.xh (regionedgeh);
    }

    const suRectangle & extrabbox = suGlobalRouter::instance()->get_region(0,0)->bbox();

    if (wire1->is (sutype::wt_preroute) && wire2->is (sutype::wt_preroute)) {
            
      const sutype::dcoord_t extray = (extrabbox.h() * 50) / 100;
      const sutype::dcoord_t extrax = (extrabbox.w() * 50) / 100;
            
      if (layer1->pgd() == sutype::go_ver) {
        transitionrect.yl (transitionrect.yl() - extray);
        transitionrect.yh (transitionrect.yh() + extray);
      }

      if (layer1->pgd() == sutype::go_hor) {
        transitionrect.xl (transitionrect.xl() - extrax);
        transitionrect.xh (transitionrect.xh() + extrax);
      }
    }

    // Nikolai; June 12, 2019
    if (1) {
      
      const sutype::dcoord_t extray = (extrabbox.h() * 100) / 100;
      const sutype::dcoord_t extrax = (extrabbox.w() * 100) / 100;

      if (layer1->pgd() == sutype::go_ver && transitionrect.h() < extray) {

        sutype::dcoord_t e = (extray - transitionrect.h()) / 2;
        transitionrect.yl (transitionrect.yl() - e);
        transitionrect.yh (transitionrect.yh() + e);
      }

      if (layer1->pgd() == sutype::go_hor && transitionrect.w() < extrax) {
        
        sutype::dcoord_t e = (extrax - transitionrect.w()) / 2;
        transitionrect.xl (transitionrect.xl() - e);
        transitionrect.xh (transitionrect.xh() + e);
      }
    }
    
  } // end of suRouteGenerator::calculate_transition_region_for_collineral_wires_

  //
  sutype::generatorinstances_t suRouteGenerator::create_generator_instances_ (const suGrid * grid,
                                                                              const sutype::dcoord_t dx,
                                                                              const sutype::dcoord_t dy)
    const
  {
    const bool createMaxSizeGeneratorsOnly = true;

    sutype::generatorinstances_t gis;

    const sutype::generators_tc & generators = grid->get_generators_at_dpoint (dx, dy);

    // all generator instances are expected to have the same width or the same height
    sutype::dcoord_t commonw = -1;
    sutype::dcoord_t commonh = -1;
    sutype::dcoord_t maxw = -1;
    sutype::dcoord_t maxh = -1;
    //anton June 12, 2018. Added maxArea
    sutype::dcoord_t maxArea = -1;
    
    bool gisHaveTheSameWidth  = true;
    bool gisHaveTheSameHeight = true;
          
    for (const auto & iter4 : generators) {

      const suGenerator * generator = iter4;
      sutype::dcoord_t w = generator->get_shape_width  ();
      sutype::dcoord_t h = generator->get_shape_height ();

      SUASSERT (w > 0, "");
      SUASSERT (h > 0, "");

      double area = w * h;

      if (area > maxArea) {
        maxArea = area;
      }
      
      if (gisHaveTheSameWidth) {
        
        if (commonw < 0) commonw = w;
        
        if (commonw != w) {
          gisHaveTheSameWidth = false;
          commonw = -1;
        }
      }

      if (gisHaveTheSameHeight) {

        if (commonh < 0) commonh = h;
        
        if (commonh != h) {
          gisHaveTheSameHeight = false;
          commonh = -1;
        }
      }

      maxw = (maxw < 0) ? w : std::max (maxw, w);
      maxh = (maxh < 0) ? h : std::max (maxh, h);
    }

    //anton June 12, 2018 Use Max area via
    //SUASSERT (gisHaveTheSameWidth || gisHaveTheSameHeight, "");

    for (const auto & iter4 : generators) {
            
      const suGenerator * generator = iter4;
      
      // use only max-size generators
      if (createMaxSizeGeneratorsOnly) {
        
        if (gisHaveTheSameWidth  && generator->get_shape_height() < maxh) continue;
        if (gisHaveTheSameHeight && generator->get_shape_width () < maxw) continue;
        
        //anton June 12 2018
        if (!gisHaveTheSameHeight && !gisHaveTheSameHeight && generator->get_shape_height() * generator->get_shape_width () != maxArea) continue;
      }
      
      suGeneratorInstance * gi = suSatRouter::instance()->create_generator_instance_ (generator, _net, dx, dy, sutype::wt_route, false);
      if (!gi->legal()) continue;

      sutype::satindex_t gisatindex = gi->layoutFunc()->satindex();
      SUASSERT (gisatindex != sutype::UNDEFINED_SAT_INDEX, "");
      
      if (suSatSolverWrapper::instance()->is_constant (0, gisatindex)) {
        SUASSERT (false, "Found an unfeasible gi: " << gi->to_str());
      }

      if (suSatSolverWrapper::instance()->is_constant (1, gisatindex)) {
        //SUASSERT (false, "Found a prerouted gi: " << gi->to_str() << "; wires: " << suStatic::to_str (gi->wires()));
      }
      
      gis.push_back (gi);
    }

    //SUASSERT (!gis.empty(), "");
    
    return gis;
    
  } // end of suRouteGenerator::create_generator_instances_

  //
  suWire * suRouteGenerator::serve_just_created_wire_ (suWire * wire)
    const
  {
    if (!wire) return 0;
    
    if (1) {
      suWireManager::instance()->keep_wire_as_illegal_if_it_is_really_illegal (wire);
    }
        
    else {
      if (wire && wire->satindex() == sutype::UNDEFINED_SAT_INDEX && !suSatRouter::instance()->wire_is_legal_slow_check (wire)) {
        SUASSERT (suWireManager::instance()->get_wire_usage (wire) == 1, "");
        suWireManager::instance()->release_wire (wire);
        return 0;
      }
    }

    SUASSERT (wire, "");
    
    // re-use precomputed info
    if (suWireManager::instance()->wire_is_illegal (wire)) {
      
      SUASSERT (wire->satindex() == sutype::UNDEFINED_SAT_INDEX, "");
      
      if (suWireManager::instance()->get_wire_usage (wire) > 1) {
        SUASSERT (suWireManager::instance()->get_wire_usage (wire) == 2, "");
        suWireManager::instance()->release_wire (wire);
      }
      SUASSERT (suWireManager::instance()->get_wire_usage (wire) == 1, "");
      SUASSERT (suWireManager::instance()->wire_is_illegal (wire), "");
      return 0;
    }

    SUASSERT (wire, "");

//     if (!_regions.empty()) {

//       bool wireTouchesBbox = false;

//       for (const auto & iter : _regions) {
//         const suRegion * region = iter;
//         if (wire->rect().has_at_least_common_point (region->bbox())) {
//           wireTouchesBbox = true;
//           break;
//         }
//       }

//       //SUINFO(1) << "I am here: " << wire->to_str() << "; wireTouchesBbox=" << wireTouchesBbox << std::endl;

//       if (!wireTouchesBbox) {
//         suWireManager::instance()->release_wire (wire);
//         return 0;
//       }
//     }

    return wire;
    
  } // end of suRouteGenerator::serve_just_created_wire_

  // other vias between two target vias
  // Here, I want to collect all vias those are electrically connected to wirelayer and within those bounds
  // wirelayer
  // currgd (ver/hor)
  // sidec (ver: x; hor: y)
  // edgel (ver: y; hor: x)
  // edgeh (ver: y; hor: x)
  sutype::clause_t & suRouteGenerator::get_satindices_in_bounds_ (const std::vector<std::map<sutype::dcoordpair_t,sutype::clause_t> > & satindicesPerDcoordAll,
                                                                  const suLayer * targetwirelayer,
                                                                  const sutype::svi_t wireLayerIndexToSkip,
                                                                  const sutype::dcoord_t sidec,
                                                                  const sutype::dcoord_t edgel,
                                                                  const sutype::dcoord_t edgeh)
    const
  {
    SUASSERT (targetwirelayer, "");
    SUASSERT (edgel <= edgeh, "edgel=" << edgel << "; edgeh=" << edgeh);
    SUASSERT (satindicesPerDcoordAll.size() == _viaFields.size(), "");
    
    sutype::clause_t & satindices = suClauseBank::loan_clause();

    // no vias in between
    if (edgel == edgeh) {
      return satindices;
    }
    
    // dx for currgd=ver
    // dy for currgd=hor

    for (sutype::uvi_t v=0; v+1 < _viaFields.size(); ++v) {

      sutype::uvi_t wireLayerIndex = v+1;
      if (wireLayerIndexToSkip >= 0 && wireLayerIndexToSkip == (sutype::svi_t)wireLayerIndex) continue;
      
      SUASSERT (wireLayerIndex < _wirelayers.size(), "");
      
      const suLayer * wirelayer = _wirelayers [v+1];
      if (wirelayer != targetwirelayer) continue;

      sutype::grid_orientation_t currgd = wirelayer->pgd();
      
      for (sutype::uvi_t v0 = v; v0 <= v+1; ++v0) {
        
        SUASSERT (v0 >= 0 && v0 < satindicesPerDcoordAll.size(), "");
        const std::map<sutype::dcoordpair_t,sutype::clause_t> & satindicesPerDxDy0 = satindicesPerDcoordAll[v0];
        
        for (const auto & iter0 : satindicesPerDxDy0) {

          const sutype::dcoordpair_t & dxdy0   = iter0.first;
          const sutype::clause_t & satindices0 = iter0.second;
          
          sutype::dcoord_t dx0 = dxdy0.first;
          sutype::dcoord_t dy0 = dxdy0.second;

          sutype::dcoord_t side0 = (currgd == sutype::go_ver) ? dx0 : dy0;
          sutype::dcoord_t edge0 = (currgd == sutype::go_ver) ? dy0 : dx0;

          if (side0 != sidec) continue;
          if (edge0 <= edgel || edge0 >= edgeh) continue;

          for (const auto & iter1 : satindices0) {
            sutype::satindex_t satindex0 = iter1;
            SUASSERT (satindex0 != sutype::UNDEFINED_SAT_INDEX, "");
            satindices.push_back (satindex0);
          }
        }
      }
    }

    suStatic::sort_and_leave_unique (satindices);
    
    return satindices;
    
  } // end of suRouteGenerator::get_satindices_in_bounds_

  //
  suLayoutFunc * suRouteGenerator::create_route_option_ ()
  {
    return _layoutfunc1->add_func (sutype::logic_func_and, sutype::unary_sign_just, SUFILELINE_DLF);
    
  } // end of suRouteGenerator::create_route_option_

  //
  bool suRouteGenerator::point_is_in_tunnel_ (sutype::dcoord_t dx,
                                              sutype::dcoord_t dy)
    const
  {
    if (_regions.empty()) return true;

    
    for (const auto & iter : _regions) {
      const suRegion * region = iter;
      if (region->bbox().has_point (dx, dy)) return true;
    }

    return false;
    
  } // end of suRouteGenerator::point_is_in_tunnel_
    
} // end of namespace amsr

// end of suRouteGenerator.cpp
