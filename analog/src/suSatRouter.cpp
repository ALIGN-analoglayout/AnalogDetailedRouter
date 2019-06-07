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
//! \date   Fri Oct 13 11:44:37 2017

//! \file   suSatRouter.cpp
//! \brief  A collection of methods of the class suSatRouter.

// std includes
#include <algorithm>
#include <fstream>
#include <iostream>
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
#include <suClauseBank.h>
#include <suConnectedEntity.h>
#include <suGenerator.h>
#include <suGeneratorManager.h>
#include <suGeneratorInstance.h>
#include <suGlobalRoute.h>
#include <suGlobalRouter.h>
#include <suGrid.h>
#include <suErrorManager.h>
#include <suLayoutFunc.h>
#include <suLayoutLeaf.h>
#include <suLayoutNode.h>
#include <suLayer.h>
#include <suLayerManager.h>
#include <suMetalTemplate.h>
#include <suMetalTemplateInstance.h>
#include <suMetalTemplateManager.h>
#include <suNet.h>
#include <suOptionManager.h>
#include <suPattern.h>
#include <suPatternInstance.h>
#include <suPatternManager.h>
#include <suRectangle.h>
#include <suRegion.h>
#include <suRoute.h>
#include <suRouteGenerator.h>
#include <suRuleManager.h>
#include <suSatSolverWrapper.h>
#include <suStatic.h>
#include <suTie.h>
#include <suTimeManager.h>
#include <suToken.h>
#include <suTokenParser.h>
#include <suWire.h>
#include <suWireManager.h>

// module include
#include <suSatRouter.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  suSatRouter * suSatRouter::_instance = 0;


  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  void suSatRouter::init_ ()
  {
    _option_may_skip_global_routes = true;
    _option_assert_illegal_input_wires = false;
    _option_simplify_routes = true;

    // I call simplify in many places and frequently
    // it's too heavy if I route in a full bbox around terminals
    if (suOptionManager::instance()->get_boolean_option ("route_full_bbox", false))
      _option_simplify_routes = false;
    
  } // end of init_

  //
  suSatRouter::~suSatRouter ()
  {
    SUINFO(1) << "Delete suSatRouter" << std::endl;

    SUINFO(1) << "Delete ties" << std::endl;

    for (const auto & iter1 : _ties) {
      auto & netties = iter1.second;
      for (const auto & iter2 : netties) {
        suTie * tie = iter2;
        delete tie;        
      }
    }

    SUINFO(1) << "Delete generator instances" << std::endl;
    
    for (const auto & iter0 : _generatorinstances) {
      const auto & container1 = iter0.second;
      for (const auto & iter1 : container1) {
        const auto & container2 = iter1.second;
        for (const auto & iter2 : container2) {
          const auto & container3 = iter2.second;
          for (const auto & iter3 : container3) {
            const auto & container4 = iter3.second;
            for (const auto & iter4 : container4) {
              const auto & container5 = iter4.second;
              for (const auto & iter5 : container5) {

                suGeneratorInstance * gi = iter5;
                delete gi;
              }
            }
          }
        }
      }
    }
        
    SUINFO(1) << "Delete connected entities" << std::endl;
    
    for (const auto & iter1 : _connectedEntities) {
      auto & objects = iter1.second;
      for (const auto & iter2 : objects) {
        suConnectedEntity * ce = iter2;
        ce->clear_wires (); // all wires are stored either in _satindexToPointer or inside nets
        delete iter2;
      }
    }

    SUINFO(1) << "Release SAT wires" << std::endl;
    
    // release the rest of registered SAT wires
    for (sutype::uvi_t i=0; i < _satindexToPointer.size(); ++i) {
      
      suWire * wire = _satindexToPointer[i];
      if (!wire) continue;
      
      wire->satindex (sutype::UNDEFINED_SAT_INDEX);

      sutype::svi_t wireusage = suWireManager::instance()->get_wire_usage (wire);
      SUASSERT (wireusage > 0, "");

      for (sutype::svi_t u=0; u < wireusage; ++u) {
        SUASSERT (suWireManager::instance()->get_wire_usage (wire) > 0, "");
        suWireManager::instance()->release_wire (wire, false);
      }
    }

    for (const auto & iter : _grids) {
      suGrid * grid = iter;
      delete grid;
    }
    
  } // end of suSatRouter::~suSatRouter

  // ------------------------------------------------------------
  // -
  // --- Public static methods
  // -
  // ------------------------------------------------------------

  // static
  void suSatRouter::delete_instance ()
  {
    if (suSatRouter::_instance)
      delete suSatRouter::_instance;
  
    suSatRouter::_instance = 0;
    
  } // end of suSatRouter::delete_instance

  // static
  bool suSatRouter::wires_are_in_conflict (const sutype::wires_t & wires1,
                                           const sutype::wires_t & wires2,
                                           bool verbose)
  {
    for (const auto & iter1 : wires1) {

      const suWire * wire1 = iter1;

      for (const auto & iter2 : wires2) {

        const suWire * wire2 = iter2;

        sutype::wire_conflict_t wireconflict = sutype::wc_undefined;
        sutype::bitid_t targetwireconflict = (sutype::bitid_t)sutype::wc_all_types;
        
        if (suSatRouter::wires_are_in_conflict (wire1, wire2, wireconflict, targetwireconflict, verbose)) return true;
      }
    }
    
    return false;
    
  } // end of suSatRouter::wires_are_in_conflict

  // static
  bool suSatRouter::wires_are_in_conflict (const suWire * wire1,
                                           const sutype::wires_t & wires2,
                                           bool verbose)
  {
    for (const auto & iter2 : wires2) {
      
      const suWire * wire2 = iter2;

      sutype::wire_conflict_t wireconflict = sutype::wc_undefined;
      sutype::bitid_t targetwireconflict = (sutype::bitid_t)sutype::wc_all_types;
      
      if (suSatRouter::wires_are_in_conflict (wire1, wire2, wireconflict, targetwireconflict, verbose)) return true;
    }

    return false;
    
  } // end of suSatRouter::wires_are_in_conflict

  // static
  bool suSatRouter::wires_are_in_conflict (const suWire * wire1,
                                           const suWire * wire2)
  {
    sutype::wire_conflict_t wireconflict = sutype::wc_undefined;
    
    bool ret = suSatRouter::wires_are_in_conflict (wire1,
                                                   wire2,
                                                   wireconflict);
    
    SUASSERT (ret == (wireconflict != sutype::wc_undefined), "");

    return ret;
    
  } // end of suSatRouter::wires_are_in_conflict
  
  // static
  bool suSatRouter::wires_are_in_conflict (const suWire * wire1,
                                           const suWire * wire2,
                                           sutype::wire_conflict_t & wireconflict)
  {

    sutype::bitid_t targetwireconflict = (sutype::bitid_t)sutype::wc_all_types;
    
    bool ret = suSatRouter::wires_are_in_conflict (wire1,
                                                   wire2,
                                                   wireconflict,
                                                   targetwireconflict,
                                                   false);
    
    SUASSERT (ret == (wireconflict != sutype::wc_undefined), "");

    return ret;
    
  } // end of suSatRouter::wires_are_in_conflict

  // core procedure
  // static
  bool suSatRouter::wires_are_in_conflict (const suWire * wire1,
                                           const suWire * wire2,
                                           sutype::wire_conflict_t & wireconflict,
                                           const sutype::bitid_t targetwireconflict,
                                           bool verbose)
  {
    const bool na = false;

    bool ok = true;

    wireconflict = sutype::wc_undefined;

    const suNet * net1 = wire1->net();
    const suNet * net2 = wire2->net();

    if (na) { SUASSERT (net1, ""); }
    if (na) { SUASSERT (net2, ""); }

    const suLayer * layer1 = wire1->layer();
    const suLayer * layer2 = wire2->layer();

    // just a short
    if (ok && (targetwireconflict & sutype::wc_short)) {
      
      if (net1 != net2 &&
          layer1->is_electrically_connected_with (layer2) &&
          wire1->rect().has_at_least_common_point (wire2->rect())
          ) {

        wireconflict = sutype::wc_short;
        ok = false;
      }
    }
    
    // min ete
    if (ok && (targetwireconflict & sutype::wc_minete)) {
      
      if (net1 != net2 &&
          layer1->base() == layer2->base() &&
          layer1->has_pgd() &&
          suRuleManager::instance()->rule_is_defined (sutype::rt_minete, layer1)
          ) {
      
        const sutype::dcoord_t sidel1 = wire1->sidel();
        const sutype::dcoord_t sideh1 = wire1->sideh();
        const sutype::dcoord_t sidel2 = wire2->sidel();
        const sutype::dcoord_t sideh2 = wire2->sideh();
      
        // edges overlap
        if ((std::max (sideh1, sideh2) - std::min (sidel1, sidel2)) <= (sideh1 - sidel1 + sideh2 - sidel2)) {

          const sutype::dcoord_t edgel1 = wire1->edgel();
          const sutype::dcoord_t edgeh1 = wire1->edgeh();
          const sutype::dcoord_t edgel2 = wire2->edgel();
          const sutype::dcoord_t edgeh2 = wire2->edgeh();
        
          const sutype::dcoord_t minete = suRuleManager::instance()->get_rule_value (sutype::rt_minete, layer1);
        
          if (edgel1 > edgeh2 && edgel1 - minete < edgeh2) { wireconflict = sutype::wc_minete; ok = false; }
          if (edgel2 > edgeh1 && edgel2 - minete < edgeh1) { wireconflict = sutype::wc_minete; ok = false; }          
        }
      }
    }
    
    // trunks of the same net can't overlap not to lose trunks and not to lose total wire width
    if (ok && (targetwireconflict & sutype::wc_trunks)) {
      
      if (net1 == net2 &&
          layer1 == layer2 &&
          layer1->has_pgd() &&
          wire1->is (sutype::wt_trunk) &&
          wire2->is (sutype::wt_trunk) &&
          wire1->rect().has_at_least_common_point (wire2->rect())
          ) {
        
        wireconflict = sutype::wc_trunks;
        ok = false;
      }
    }
        
    // colors can't overlap
    if (ok && (targetwireconflict & sutype::wc_colors)) {
      
      if (layer1->base() == layer2->base() &&
          layer1 != layer2 &&
          wire1->rect().has_at_least_common_point (wire2->rect())
          ) {

        wireconflict = sutype::wc_colors;
        ok = false;
      }
    }

    // if a shunt wire A between wires B and C touches any other wire D - it means it can be a shorter route between B and D and C and D
    // disabled for while
    if (0 && ok && (targetwireconflict & sutype::wc_routing)) {

      if (layer1 == layer2 &&
          
          ((wire1->is (sutype::wt_shunt) && wire2->is (sutype::wt_trunk))    ||
           (wire1->is (sutype::wt_shunt) && wire2->is (sutype::wt_route))    ||
           (wire2->is (sutype::wt_shunt) && wire1->is (sutype::wt_trunk))    ||
           (wire2->is (sutype::wt_shunt) && wire1->is (sutype::wt_route))) &&
          
          wire1->rect().has_at_least_common_point (wire2->rect())
          ) {
        
        wireconflict = sutype::wc_routing;
        ok = false;
      }
    }
    
    if (na) { SUASSERT (ok == (wireconflict == sutype::wc_undefined), ""); }
    
    if (!ok) {

      if (verbose) {
        
        SUISSUE("Found a conflict in verbose mode: " + suStatic::wire_conflict_2_str (wireconflict) + ", " + wire1->layer()->base()->name() + " (written an error)")
          << ": wire1=" << wire1->to_str()
          << "; wire2=" << wire2->to_str()
          << std::endl;

        suRectangle errrect;

        wire1->rect().calculate_overlap (wire2->rect(), errrect);

        suErrorManager::instance()->add_error (suStatic::wire_conflict_2_str (wireconflict) + "_" + wire1->layer()->base()->name(),
                                               errrect);
      }
      
      return true; // wires are in conflict
    }
    
    else {
      
      return false; // wires are not in conflict
    }
    
  } // end of suSatRouter::wires_are_in_conflict
  
  // static
  bool suSatRouter::static_wire_covers_edge (sutype::satindex_t wiresatindex,
                                             sutype::dcoord_t edge,
                                             const suLayer * layer,
                                             sutype::clause_t & clause)
  {
    return suSatRouter::instance()->wire_covers_edge_ (wiresatindex, edge, layer, clause);
    
  } // end of suSatRouter::static_wire_covers_edge
  
  // static
  void suSatRouter::print_layout_node (std::ostream & oss,
                                       const suLayoutNode * node,
                                       const std::string & offset)
  {
    SUASSERT (node, "");

    if      (node->is_leaf()) suSatRouter::print_layout_leaf_ (oss, node->to_leaf(), offset);
    else if (node->is_func()) suSatRouter::print_layout_func_ (oss, node->to_func(), offset);
    else {
      SUASSERT (false, "");
    }
    
  } // end of suSatRouter::print_layout_node

  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------
    
  //
  void suSatRouter::dump_connected_entities (const std::string & filename)
    const
  {
    SUINFO(1)
      << "Dump connected entities to file "
      << filename
      << std::endl;

    suStatic::create_parent_dir (filename);

    std::ofstream ofs (filename);
    if (!ofs.is_open()) {
      SUISSUE("Could not open file for writing") << ": " << filename << std::endl;
      SUASSERT (false, "");
    }

    for (const auto & iter1 : _connectedEntities) {
      
      const auto & ces = iter1.second;
      
      for (const auto & iter1 : ces) {
        
        suConnectedEntity * ce = iter1;
        if (!ce->is (sutype::wt_preroute)) continue;

        ofs << std::endl;
        ofs << "ConnectedEntity";

        ofs << " net=" << ce->net()->name();

        ofs << " ceid=" << ce->id();
        
        ofs << " terms=";
        
        const sutype::wires_t & cewires = ce->wires();
        
        // dump gid of wires
        bool needcomma = false;
        for (const auto & iter2 : cewires) {

          suWire * wire = iter2;
          sutype::id_t gid = wire->gid();

          if (needcomma) {
            ofs << ",";
          }
          else {
            needcomma = true;
          }

          ofs << gid;
        }

        ofs << " {" << std::endl;
        //ofs << std::endl;

        // dump wires just as comments
        for (const auto & iter2 : cewires) {
          suWire * wire = iter2;
          ofs << "  " << "# Wire " << wire->to_str() << std::endl;
        }
        //ofs << std::endl;
        
        ofs << "}" << std::endl;
      }
    }

    ofs.close();

    SUINFO(1) << "Written " << filename << std::endl;
    
  } // end of suSatRouter::dump_connected_entities

  //
  void suSatRouter::create_fake_metal_template_instances (bool pgd_tracks,
                                                          bool ogd_grid_lines,
                                                          bool pgd_grid_lines)
    const
  {
    SUINFO(1) 
      << "Create fake metal template instances"
      << "; pgd_tracks=" << pgd_tracks
      << "; ogd_grid_lines=" << ogd_grid_lines
      << "; pgd_grid_lines=" << pgd_grid_lines
      << std::endl;

    const sutype::dcoord_t hw = 1;

    const sutype::metaltemplateinstances_t & mtis = suMetalTemplateManager::instance()->allMetalTemplateInstances();
        
    for (sutype::uvi_t i=0; i < mtis.size(); ++i) {
      
      const suMetalTemplateInstance * mti = mtis[i];
      
      std::string netname1 ("MTI_" + mti->metalTemplate()->name());
      netname1 += std::string ("_absoffset_" + suStatic::dcoord_to_str (mti->shift()));
      netname1 += std::string ("_inv_" + suStatic::dcoord_to_str (mti->isInverted()));

      std::string netname2 = netname1 + "_ogd";
      std::string netname3 = netname1 + "_pgd";
            
      suNet * net1 = 0; // pgd_tracks
      suNet * net2 = 0; // ogd_grid_lines
      suNet * net3 = 0; // pgd_grid_lines

      if (pgd_tracks) {

        if (net1 == 0) {
          net1 = suCellManager::instance()->topCellMaster()->create_net_if_needed (netname1);
        }
        SUASSERT (net1, "");
        
        sutype::wires_t wires;
        mti->create_wires_in_rectangle (mti->region(), net1, sutype::wt_undefined, false, wires); // fake wires
        
        for (const auto & iter : wires) {
          
          suWire * wire = iter;
          net1->add_wire (wire);
        }
      }

      if (pgd_grid_lines) {

        if (net3 == 0) {
          net3 = suCellManager::instance()->topCellMaster()->create_net_if_needed (netname3);
        }
        SUASSERT (net3, "");

        sutype::wires_t wires;
        mti->create_wires_in_rectangle (mti->region(), net3, sutype::wt_undefined, false, wires); // fake wires

        for (const auto & iter : wires) {

          suWire * wire = iter;
          
          sutype::dcoord_t sidec = wire->sidec();
          sutype::dcoord_t sidel = sidec - hw;
          sutype::dcoord_t sideh = sidec + hw;

          suWire * ecowire = suWireManager::instance()->create_wire_from_edge_side (wire->net(),
                                                                                    wire->layer(),
                                                                                    wire->edgel(),
                                                                                    wire->edgeh(),
                                                                                    sidel,
                                                                                    sideh,
                                                                                    sutype::wt_undefined);
          net3->add_wire (ecowire);
          
          suWireManager::instance()->release_wire (wire);
        }
      }
      
      sutype::dcoords_t lineends = mti->get_line_ends_in_rectangle (mti->region());

      for (sutype::uvi_t k=0; k < lineends.size(); ++k) {

        sutype::dcoord_t lineend = lineends[k];

        // default
        sutype::dcoord_t xl = mti->region().xl();
        sutype::dcoord_t yl = mti->region().yl();
        sutype::dcoord_t xh = mti->region().xh();
        sutype::dcoord_t yh = mti->region().yh();
        
        if (mti->metalTemplate()->baseLayer()->pgd() == sutype::go_hor) {
          xl = lineend - hw;
          xh = lineend + hw;
        }
        else {
          yl = lineend - hw;
          yh = lineend + hw;
        }

        if (ogd_grid_lines) {

          if (net2 == 0) {
            net2 = suCellManager::instance()->topCellMaster()->create_net_if_needed (netname2);
          }
          SUASSERT (net2, "");
        
          suWire * wire = suWireManager::instance()->create_wire_from_dcoords (net2, mti->metalTemplate()->baseLayer(), xl, yl, xh, yh); // fake wire
          net2->add_wire (wire);
        }
      }
    }
    
  } // end of suSatRouter::create_fake_metal_template_instances
  
  // for debug only; too CEs as wires in GUI
  void suSatRouter::create_fake_connected_entities ()
    const
  {
    SUINFO(1) << "Create fake connected entities" << std::endl;

    // debug
    if (0) {
      
      const sutype::nets_t & nets = suCellManager::instance()->topCellMaster()->nets();

      for (const auto & iter0 : nets) {

        suNet * net = iter0;
        const sutype::wires_t & wires = net->wires();

        for (sutype::uvi_t i=0; i < wires.size(); ++i) {

          suWire * wire1 = wires[i];

          for (sutype::uvi_t k=i+1; k < wires.size(); ++k) {

            suWire * wire2 = wires[k];
            if (wire1->layer() == wire2->layer()) continue;

            if (!wire1->rect().has_at_least_common_point (wire2->rect())) continue;

            sutype::dcoord_t xl, yl, xh, yh;

            wire1->rect().calculate_overlap (wire2->rect(),
                                             xl, yl, xh, yh);

            xl += 20;
            yl += 20;
            xh -= 20;
            yh -= 20;

            std::cout
              << "Wire net=" << net->name()
              << " layer=via1"
              << " rect=" << xl << ":" << yl << ":" << xh << ":" << yh
              << std::endl;
          }
        }
      }

      SUABORT;
    }
    // debug
    
    for (const auto & iter1 : _connectedEntities) {
      
      auto & ces = iter1.second;
      
      for (sutype::uvi_t i=0; i < ces.size(); ++i) {
        
        suConnectedEntity * ce = ces[i];
        const suNet * net = ce->net();

        std::string netname (net->name());
        netname += std::string ("_ce_" + suStatic::dcoord_to_str (ce->id()));
        netname += std::string ("_" + suStatic::wire_type_2_str (ce->type()));
        
        if (ce->get_interface_layer()) {
          netname += std::string ("_" + ce->get_interface_layer()->name());
        }

        suNet * net2 = suCellManager::instance()->topCellMaster()->create_net_if_needed (netname);
        SUASSERT (net2, "");
        
        for (int mode = 0; mode <= 1; ++mode) {

          const sutype::wires_t & wires = (mode == 0) ? ce->interfaceWires() : ce->wires();
          
          for (const auto & iter : wires) {
          
            suWire * wire = iter;
            //sutype::satindex_t satindex = wire->satindex();
            //SUASSERT (wire->satindex() != sutype::UNDEFINED_SAT_INDEX, "");
            //if (suSatSolverWrapper::instance()->get_modeled_value (satindex) != sutype::bool_true) continue;
          
            suWire * wire2 = suWireManager::instance()->create_wire_from_wire (net2, wire); // fake wire
          
            net2->add_wire (wire2);
          }
        }
      }
    }
    
  } // end of suSatRouter::create_fake_connected_entities

  //
  void suSatRouter::create_fake_ties ()
    const
  {
    SUINFO(1) << "Create fake ties" << std::endl;
    
    for (const auto & iter1 : _ties) {
      
      const auto & ties = iter1.second;
            
      for (const auto & iter2 : ties) {
        
        suTie * tie = iter2;
        const suNet * net = tie->net();

        const sutype::routes_t & routes = tie->routes();
        
        for (sutype::uvi_t r=0; r < routes.size(); ++r) {

          suRoute * route = routes[r];

          suLayoutFunc * layoutFunc = route->layoutFunc();
          SUASSERT (layoutFunc, "");

          sutype::bool_t value = layoutFunc->get_modeled_value ();
          
          if (value != sutype::bool_true) continue;

          std::string netname2 (
                                net->name()
                                + "_tie_" + suStatic::dcoord_to_str (tie->entity0()->id()) + "_" + suStatic::dcoord_to_str (tie->entity1()->id())
                                + "_r_" + suStatic::dcoord_to_str (r)
                                );
          
          suNet * net2 = suCellManager::instance()->topCellMaster()->create_net_if_needed (netname2);
          SUASSERT (net2, "");

          const sutype::wires_t & wires = route->wires();
          SUASSERT (wires.size() >= 2, "");
          
          // first two wires are input wires that this route connects
          for (sutype::uvi_t w=0; w < wires.size(); ++w) {
            
            suWire * wire = wires[w];
            sutype::satindex_t satindex = wire->satindex();
            SUASSERT (wire->satindex() != sutype::UNDEFINED_SAT_INDEX, "");
            if (suSatSolverWrapper::instance()->get_modeled_value (satindex) != sutype::bool_true) continue;
            
            suWire * wire2 = suWireManager::instance()->create_wire_from_wire (net2, wire); // fake wire
            
            net2->add_wire (wire2);

            // one more wire; too see all wires separately
            if (1) {
              
              std::string netname3 (
                                    netname2
                                    + "_sat" + suStatic::dcoord_to_str (wire->satindex())
                                    );
              
              suNet * net3 = suCellManager::instance()->topCellMaster()->create_net_if_needed (netname3);
              SUASSERT (net3, "");
              
              suWire * wire3 = suWireManager::instance()->create_wire_from_wire (net3, wire); // fake wire
              
              net3->add_wire (wire3);
            }
            
          }
        }
      }
    }
    
  } // end of suSatRouter::create_fake_ties

  //
  void suSatRouter::create_fake_sat_wires ()
    const
  {
    SUINFO(1) << "Create fake SAT wires" << std::endl;

    const sutype::nets_tc & netsToRoute = suCellManager::instance()->netsToRoute();
    
    for (sutype::uvi_t i=0; i < _satindexToPointer.size(); ++i) {

      suWire * wire = _satindexToPointer[i];
      if (!wire) continue;

      sutype::satindex_t satindex = wire->satindex();
      SUASSERT (satindex == (sutype::satindex_t)i, "");
      
      if (suSatSolverWrapper::instance()->get_modeled_value (satindex) != sutype::bool_true) continue;
      if (wire->is (sutype::wt_preroute)) continue;

      const suNet * net = wire->net();
      if (std::find (netsToRoute.begin(), netsToRoute.end(), net) == netsToRoute.end()) continue;
            
      std::string netname (
                           net->name()
                           + "_wire"
                           + "_" + suStatic::wire_type_2_str (wire->get_wire_type())
                           + "_id_" + suStatic::dcoord_to_str (wire->id())
                           + "_si_" + suStatic::dcoord_to_str (satindex)
                           );
      
      suNet * net2 = suCellManager::instance()->topCellMaster()->create_net_if_needed (netname);
      SUASSERT (net2, "");

      suWire * wire2 = suWireManager::instance()->create_wire_from_wire (net2, wire); // fake wire
      
      net2->add_wire (wire2);
    }
    
  } // end of suSatRouter::create_fake_sat_wires

  //
  void suSatRouter::prepare_input_wires_and_create_connected_entities ()
  {
    SUINFO(1) << "Prepare input wires and create connected entities." << std::endl;

    const sutype::nets_tc & netsToRoute = suCellManager::instance()->netsToRoute();
    
    const suTokenParser * tokenparser = suGlobalRouter::instance()->grTokenParser();
    
    // all connected entities are stored here as tokens
    sutype::tokens_t cetokens;
    
    if (tokenparser) {
      SUASSERT (tokenparser->rootToken(), "");
      cetokens = tokenparser->rootToken()->get_tokens ("ConnectedEntity");
      SUINFO(1) << "Found " << cetokens.size() << " preliminary connected entities." << std::endl;
    }
    
    // get all wires used by connected entities
    std::set<sutype::id_t> gidsInUse;
    sutype::uvi_t numValidCeTokens = 0;

    std::map<std::string,sutype::uvi_t> messages;
    
    for (sutype::uvi_t i=0; i < cetokens.size(); ++i) {
      
      suToken * token = cetokens[i];
      SUASSERT (token->is_defined ("terms"), "Connected Entity has no terminals");

      const suNet * cenet = 0;

      std::vector<int> rawgids = token->get_list_of_integers ("terms");
      suStatic::sort_and_leave_unique (rawgids);

      sutype::ids_t gids; // collect work gids here

      for (sutype::uvi_t k=0; k < rawgids.size(); ++k) {
      
        sutype::id_t gid = (sutype::id_t)rawgids[k];
        suWire * wire = suWireManager::instance()->get_wire_by_reserved_gid (gid);
        
        if (wire == 0) {
          SUISSUE("Could not get a wire by gid") << ": gid=" << gid << std::endl;
          //SUASSERT (false, "Could not get a wire by gid=" << gid);
          continue;
        }
        
        SUASSERT (wire, "");
        const suNet * wirenet = wire->net();
        if (cenet == 0) cenet = wirenet;
        SUASSERT (cenet, "");
        SUASSERT (cenet == wirenet, "Wires of a connected entity belong to different nets: " << cenet->name() << " and " << wirenet->name());
        
        gids.push_back (gid);
      }

      if (gids.empty()) {
        SUISSUE("Could not get wires for a connected entity") << std::endl;
        //SUASSERT (false, "Could not get wires for a connected entity.");
        continue;
      }

      SUASSERT (cenet, "");
      
      if (std::find (netsToRoute.begin(), netsToRoute.end(), cenet) == netsToRoute.end()) {
        //SUINFO(1) << "Skipped a connected entity of net " << cenet->name() << std::endl;
        ++messages [std::string("Skipped a connected entity of net ") + cenet->name()];
        continue;
      }
      
      cetokens [numValidCeTokens] = token;
      ++numValidCeTokens;

      for (const auto & iter : gids) {
        sutype::id_t gid = (sutype::id_t)iter;
        suWire * wire = suWireManager::instance()->get_wire_by_reserved_gid (gid);
        SUASSERT (wire, "");
        suWireManager::instance()->set_permanent_gid (wire, gid);
      }
    }
    
    // report
    for (const auto & iter : messages) {
      const std::string & msg = iter.first;
      sutype::uvi_t count = iter.second;
      SUINFO(1) << msg << " (" << count << ")" << std::endl;
    }
    
    // trim skipped connected entities
    if (numValidCeTokens != cetokens.size()) {
      cetokens.resize (numValidCeTokens);
    }
    
    // merge input wires
    //anton June12,2018 Commented wire merge to avoid gid removal
    //merge_input_wires ();

    //
    merge_input_wires ();

    // register preroutes and set satindex for every preroute
    register_preroutes_as_sat_objects ();

    // create connected entities for preroutes
    calculate_net_groups_ (cetokens);
       
  } // end of suSatRouter::prepare_input_wires_and_create_connected_entities

  // I-wires between preroutes
  // vias at intersections
  void suSatRouter::create_evident_preroutes ()
  {
    SUINFO(1) << "Create evident preroutes." << std::endl;
    
    optimize_the_number_of_direct_wires_between_preroutes_ (true);
    
  } // end of suSatRouter::create_evident_preroutes

  //
  bool suSatRouter::check_input_wires ()
    const
  {
    const sutype::nets_t & nets = suCellManager::instance()->topCellMaster()->nets();

    unsigned numconflicts = 0;

    for (sutype::uvi_t i=0; i < nets.size(); ++i) {
      
      suNet * net0 = nets[i];

      const sutype::wires_t & wires0 = net0->wires();

      for (sutype::uvi_t k=i+1; k < nets.size(); ++k) {

        suNet * net1 = nets[k];

        const sutype::wires_t & wires1 = net1->wires();

        for (const auto & iter0 : wires0) {

          suWire * wire0 = iter0;

          for (const auto & iter1 : wires1) {
            
            suWire * wire1 = iter1;

            sutype::wire_conflict_t wireconflict = sutype::wc_undefined;
            bool ret = wires_are_in_conflict (wire0, wire1, wireconflict);
            if (!ret) continue;
            
            SUASSERT (wireconflict != sutype::wc_undefined, "");
            
            std::string wcstr = suStatic::wire_conflict_2_str (wireconflict);
            
            SUISSUE("Found an unsolvable conflict between wires (written an error " + wcstr + ")")
              << ": wire0=" << wire0->to_str()
              << "; wire1=" << wire1->to_str()
              << std::endl;

            // default
            sutype::dcoord_t xl = std::min (wire0->rect().xl(), wire1->rect().xl());
            sutype::dcoord_t yl = std::min (wire0->rect().yl(), wire1->rect().yl());
            sutype::dcoord_t xh = std::max (wire0->rect().xh(), wire1->rect().xh());
            sutype::dcoord_t yh = std::max (wire0->rect().yh(), wire1->rect().yh());

            if (wireconflict == sutype::wc_short  ||
                wireconflict == sutype::wc_colors ||
                wireconflict == sutype::wc_trunks ||
                wireconflict == sutype::wc_minete) {

              wire0->rect().calculate_overlap (wire1->rect(), xl, yl, xh, yh);
            }
            else {
              SUASSERT (false, "unexpected wire conflict");
            }
            
            suErrorManager::instance()->add_error (wcstr + "_" + wire0->layer()->name() + "_" + wire1->layer()->name() + "_"+ wire0->net()->name() + "_" + wire1->net()->name(),
                                                   xl, yl, xh, yh);
            ++numconflicts;
          }
        }
      }
    }

    if (numconflicts) {
      SUINFO(1) << "Found " << numconflicts << " unsolvable conflict " << ((numconflicts == 1) ? "" : "s") << " between input wires." << std::endl;
      return false;
    }
    
    return true;
    
  } // end of suSatRouter::check_input_wires

  //
  void suSatRouter::merge_input_wires ()
  {
    const sutype::nets_t & nets = suCellManager::instance()->topCellMaster()->nets();

    for (sutype::uvi_t i=0; i < nets.size(); ++i) {
      
      suNet * net = nets[i];
      SUASSERT (net->wire_data_model_is_legal(), "");
      
      net->merge_wires ();
      SUASSERT (net->wire_data_model_is_legal(), "");
      
      // remove via cuts stored as wires
      net->remove_useless_wires ();
      SUASSERT (net->wire_data_model_is_legal(), "");
    }
    
  } // end of suSatRouter::merge_input_wires
  
  //
  void suSatRouter::register_preroutes_as_sat_objects ()
  {
    SUINFO(1) << "Register pre-routed wires as SAT objects." << std::endl;

    const bool dumpwires = false; // debug only
    
    const sutype::nets_t & nets = suCellManager::instance()->topCellMaster()->nets();
    const sutype::uvi_t numnets = nets.size(); // I take this fixed number; I can append new nets during a debug mode: dumpwires
    
    const bool createExtraMtis = !false; // debug only
    sutype::metaltemplateinstances_t externalmtis; // extra mtis to advise

    // dummy check
    for (sutype::uvi_t i=0; i < numnets; ++i) {
      suNet * net = nets[i];
      SUASSERT (net->wire_data_model_is_legal(), "");
    }
    //
    
    for (sutype::uvi_t i=0; i < numnets; ++i) {
      
      suNet * net = nets[i];
      SUASSERT (net->wire_data_model_is_legal(), "");
      
      const sutype::generatorinstances_t & gis = net->generatorinstances();
      
      for (const auto & iter0 : gis) {

        suGeneratorInstance * preroutedgi = iter0;
        
        // create a clone
        suGeneratorInstance * gi = create_generator_instance_ (preroutedgi->generator(),
                                                               preroutedgi->net(),
                                                               preroutedgi->point().x(),
                                                               preroutedgi->point().y(),
                                                               sutype::wt_preroute,
                                                               true);
        SUASSERT (gi, "");
        
        if (!gi->legal()) {

          SUISSUE("Found an illegal generator instance (written an error illegal_gi)")
            << ": " << gi->to_str()
            << std::endl;

          suErrorManager::instance()->add_error (std::string ("illegal_gi_") + gi->generator()->name(),
                                                 gi->point().x() - 100,
                                                 gi->point().y() - 100,
                                                 gi->point().x() + 100,
                                                 gi->point().y() + 100);

          // more log
          if (1) {
            const sutype::wires_t & giwires = gi->wires();
            for (const auto & iter : giwires) {
              suWire * wire = iter;
              SUINFO(1) << wire->to_str() << std::endl;
              if (!wire_is_legal_slow_check (wire, true)) {
                SUISSUE("Generator instance wire is illegal") << ": " << wire->to_str() << std::endl;
              }
            }
          }
          
          SUASSERT (false, "Found an illegal generator instance: " << gi->to_str());
        }
        
        SUASSERT (gi->layoutFunc(), "");
        SUASSERT (gi->layoutFunc()->satindex() != sutype::UNDEFINED_SAT_INDEX, "");
        
        const sutype::wires_t & giwires = gi->wires();
        for (const auto & iter1 : giwires) {
          suWire * wire = iter1;
          //SUINFO(1) << "Created a wire: " << wire->to_str() << std::endl; // debug only
          SUASSERT (wire->satindex() != sutype::UNDEFINED_SAT_INDEX, "");
          suSatSolverWrapper::instance()->emit_ALWAYS_ONE (wire->satindex());
        }
        suSatSolverWrapper::instance()->emit_ALWAYS_ONE (gi->layoutFunc()->satindex());
      }
      
      const sutype::wires_t & wires = net->wires();
      
      for (sutype::uvi_t k=0; k < wires.size(); ++k) {
        
        suWire * wire = wires[k];
        SUASSERT (wire->is (sutype::wt_preroute), wire->to_str());
        SUASSERT (wire->satindex() == sutype::UNDEFINED_SAT_INDEX, "");
        
        bool ok = true;

        if (ok && wire->layer()->is (sutype::lt_wire)) {
          ok = suMetalTemplateManager::instance()->check_wire_with_metal_template_instances (wire);
        }
        
        if (!ok) {
          
          SUISSUE("Wire doesn't match any available metal template instance") << ": wire " << wire->to_str() << std::endl;
          if (_option_assert_illegal_input_wires) { SUASSERT (false, ""); }

          // debug
          if (dumpwires) {
            std::string debugnetname ("debug_" + wire->net()->name());
            suNet * debugnet = suCellManager::instance()->topCellMaster()->create_net_if_needed (debugnetname);
            suWire * debugwire = suWireManager::instance()->create_wire_from_wire (debugnet, wire); // debug wire
            debugnet->add_wire (debugwire); // final wire
            continue;
          }
          //

          if (createExtraMtis) {
            suMetalTemplateManager::instance()->create_all_possible_mtis (wire, externalmtis);
          }
          
        }
        
        else {

          // do nothing

          // debug
          if (dumpwires) {

            // I have to continue to avoid an assert during subsequent register_a_sat_wire
            continue;
          }
          //
        }

        register_a_sat_wire (wire);
        
        SUASSERT (wire->satindex() != sutype::UNDEFINED_SAT_INDEX, "");
        
        suSatSolverWrapper::instance()->emit_ALWAYS_ONE (wire->satindex());
      }
      
    } // for(nets)

    // dummy check
    for (sutype::uvi_t i=0; i < numnets; ++i) {
      suNet * net = nets[i];
      SUASSERT (net->wire_data_model_is_legal(), "");
    }
    //

    // debug
    if (dumpwires) {
      suCellManager::instance()->dump_out_file ("out/" + suCellManager::instance()->topCellMaster()->name() + ".lgf");
      SUABORT;
    }
    //
        
  } // end of suSatRouter::register_preroutes_as_sat_objects
  
  //
  void suSatRouter::convert_global_routing_to_trunks ()
  {
    const bool np = false;

    const bool option_allow_opens = suOptionManager::instance()->get_boolean_option ("allow_opens", false);
    const bool option_auto_fix_global_routing = suOptionManager::instance()->get_boolean_option ("auto_fix_global_routing", true);
    
    SUINFO(1)
      << "Convert global routing to trunks"
      << "; allow_opens=" << option_allow_opens
      << std::endl;

    sutype::globalroutes_t & globalroutes = suGlobalRouter::instance()->globalRoutes();
    
    const sutype::metaltemplateinstances_t & mtis = suMetalTemplateManager::instance()->allMetalTemplateInstances();

    for (sutype::uvi_t i=0; i < globalroutes.size(); ++i) {

      suGlobalRoute * globalroute = globalroutes[i];
      
      SUINFO(np) << "globalroute=" << globalroute->to_str() << std::endl;
      
      const suNet * net = globalroute->net();
      if (!suCellManager::instance()->route_this_net (net)) continue;
      
      const suLayer * layer = globalroute->layer();
      SUASSERT (layer->is_base(), "");
      const sutype::grid_orientation_t pgd = layer->pgd();
     
      SUASSERT (layer->pgd() == sutype::go_ver || layer->pgd() == sutype::go_hor, "");
      
      sutype::regions_t regions = globalroute->get_regions();
      SUASSERT (!regions.empty(), "");
      SUASSERT (regions.size() >= 2, "global route covers less than 2 regions: " << regions.size());

      SUASSERT (globalroute->length_in_regions () >= 1, "");
      SUASSERT (globalroute->width_in_regions  () == 0, "");
      
      // check:
      // regions are expected to be sorted and adjacent
      for (sutype::uvi_t r=0; r+1 < regions.size(); ++r) {
        
        const suRegion * region0 = regions [r];
        const suRegion * region1 = regions [r+1];
        
        const suRectangle & bbox0 = region0->bbox();
        const suRectangle & bbox1 = region1->bbox();
        
        // regions are expected to be sorted and adjacent
        SUASSERT (bbox0.sidel (pgd) == bbox1.sidel (pgd), "");
        SUASSERT (bbox0.sideh (pgd) == bbox1.sideh (pgd), "");
        SUASSERT (bbox0.edgeh (pgd) == bbox1.edgel (pgd), "");
      }

      //  +-----+-----+-----+-----+
      //  |     |     |     |     |
      //  |  o-----------------o  |  global route
      //  |     |     |     |     |
      //  +-----+-----+-----+-----+

      //  +-----+-----+-----+-----+
      //  |     |     |     |     |
      //  |     [===========]     | --> trunk wire
      //  |     |     |     |     |
      //  +-----+-----+-----+-----+
      //
      suRectangle defaulttrunkbbox;
      defaulttrunkbbox.sidel (pgd, regions.front()->bbox().sidel (pgd));
      defaulttrunkbbox.sideh (pgd, regions.front()->bbox().sideh (pgd));
      defaulttrunkbbox.edgel (pgd, regions.front()->bbox().edgeh (pgd));
      defaulttrunkbbox.edgeh (pgd, regions.back ()->bbox().edgel (pgd));
      
      // extend a little bit
      if (defaulttrunkbbox.edgel (pgd) == defaulttrunkbbox.edgeh (pgd)) {
        defaulttrunkbbox.edgel (pgd, defaulttrunkbbox.edgel (pgd) - 10);
        defaulttrunkbbox.edgeh (pgd, defaulttrunkbbox.edgeh (pgd) + 10);
      }
      
      const suRegion * regionAF = (regions.size() > 1) ? regions[1]                : (const suRegion *)0; // after front
      const suRegion * regionBB = (regions.size() > 1) ? regions[regions.size()-2] : (const suRegion *)0; // before back
      
      sutype::dcoord_t lengthregionAF = regionAF ? (regionAF->bbox().edgeh(pgd) - regionAF->bbox().edgel(pgd)) : 0;
      sutype::dcoord_t lengthregionBB = regionBB ? (regionBB->bbox().edgeh(pgd) - regionBB->bbox().edgel(pgd)) : 0;
      
      SUINFO(np) << "defaulttrunkbbox=" << defaulttrunkbbox.to_str() << std::endl;

      SUASSERT (defaulttrunkbbox.xl() < defaulttrunkbbox.xh(), defaulttrunkbbox.to_str());
      SUASSERT (defaulttrunkbbox.yl() < defaulttrunkbbox.yh(), defaulttrunkbbox.to_str());

      sutype::dcoord_t side2side = defaulttrunkbbox.sideh (pgd) - defaulttrunkbbox.sidel (pgd);
      SUASSERT (side2side > 0, "");
      //side2side = 0;
      
      // to be sure to create at least one trunk outside region bounds
      // I don't want all tracks completely inside bounds
      suRectangle extendedtrunkbbox = defaulttrunkbbox;
      extendedtrunkbbox.sidel (pgd, extendedtrunkbbox.sidel (pgd) - side2side);
      extendedtrunkbbox.sideh (pgd, extendedtrunkbbox.sideh (pgd) + side2side);

      // create side-2-side trunk wires
      sutype::wires_t alltrunkwires;
      
      // create unique trunks
      for (sutype::uvi_t m=0; m < mtis.size(); ++m) {
        
        const suMetalTemplateInstance * mti = mtis[m];
        if (mti->metalTemplate()->baseLayer() != layer) continue;

        sutype::wires_t trunkwires;
        mti->create_wires_in_rectangle (extendedtrunkbbox, net, sutype::wt_trunk, false, trunkwires); // trunk wires

        //SUINFO(1) << "Created " << trunkwires.size() << " trunk wires." << std::endl;
        
        std::sort (trunkwires.begin(), trunkwires.end(), suStatic::compare_wires_by_sidel);

        if (trunkwires.empty()) continue;

        // to be sure to create at least one trunk outside region
        sutype::dcoord_t actualsidel = trunkwires.front()->sidel();
        sutype::dcoord_t actualsideh = trunkwires.back() ->sideh();

//         // debug
//         if (0) {
//           for (sutype::uvi_t t0 = 0; t0 < trunkwires.size(); ++t0) {
//             suWire * trunkwire0 = trunkwires [t0];
//             SUINFO(1) << "trunkwire0=" << trunkwire0->to_str() << std::endl;
//           }
//           SUINFO(1) << "actualsidel=" << actualsidel << std::endl;
//           SUINFO(1) << "actualsideh=" << actualsideh << std::endl;
//           SUINFO(1) << "defaulttrunkbbox=" << defaulttrunkbbox.to_str() << std::endl;
//           SUINFO(1) << "extendedtrunkbbox=" << extendedtrunkbbox.to_str() << std::endl;
//           SUINFO(1) << "side2side=" << side2side << std::endl;
//         }
        
        // check at bottom
        for (sutype::uvi_t t0 = 0; t0 < trunkwires.size(); ++t0) {
        
          suWire * trunkwire0 = trunkwires [t0];
          if (trunkwire0->sideh() >= defaulttrunkbbox.sidel (pgd)) {
            if (trunkwire0->sidel() < defaulttrunkbbox.sidel (pgd)) {
              actualsidel = trunkwire0->sidel();
            }
            break;
          }
          actualsidel = trunkwire0->sidel();
        }

        // check at top
        for (sutype::svi_t t0 = (sutype::svi_t)trunkwires.size() - 1; t0 >= 0; --t0) {

          suWire * trunkwire0 = trunkwires [t0];
          if (trunkwire0->sidel() <= defaulttrunkbbox.sideh (pgd)) {
            if (trunkwire0->sideh() > defaulttrunkbbox.sideh (pgd)) {
              actualsideh = trunkwire0->sideh();
            }
            break;
          }
          actualsideh = trunkwire0->sideh();
        }
        
        //SUASSERT (actualsidel <= defaulttrunkbbox.sidel (pgd), "actualsidel=" << actualsidel << "; defaulttrunkbbox.sidel=" << defaulttrunkbbox.sidel(pgd));
        //SUASSERT (actualsideh >= defaulttrunkbbox.sideh (pgd), "actualsideh=" << actualsideh << "; defaulttrunkbbox.sideh=" << defaulttrunkbbox.sideh(pgd));
        
        // trim trunks outside actuall bbox
        sutype::uvi_t counter=0;

        for (sutype::uvi_t t0 = 0; t0 < trunkwires.size(); ++t0) {
          
          suWire * trunkwire0 = trunkwires [t0];
          
          if (trunkwire0->sidel() < actualsidel ||
              trunkwire0->sideh() > actualsideh) {

            suWireManager::instance()->release_wire (trunkwire0);
            continue;
          }
          
          trunkwires[counter] = trunkwire0;
          ++counter;
        }

        if (counter != trunkwires.size())
          trunkwires.resize (counter);

        alltrunkwires.insert (alltrunkwires.end(), trunkwires.begin(), trunkwires.end());
      }
      
      if (alltrunkwires.empty()) {
        SUISSUE("Global route has no feasible trunk wires") << ": " << globalroute->to_str() << std::endl;
        continue;
      }

      // merge trunk wires that belong to different mtis
      for (sutype::svi_t t0 = 0; t0 < (sutype::svi_t)alltrunkwires.size(); ++t0) {
        
        suWire * trunkwire0 = alltrunkwires[t0];
        
        for (sutype::svi_t t1 = t0+1; t1 < (sutype::svi_t)alltrunkwires.size(); ++t1) {
          
          suWire * trunkwire1 = alltrunkwires[t1];
          
          if (trunkwire0->sidel() != trunkwire1->sidel()) continue;
          if (trunkwire0->sideh() != trunkwire1->sideh()) continue;
          if (trunkwire0->layer() != trunkwire1->layer()) continue;

          sutype::dcoord_t wiresidel = trunkwire0->sidel();
          sutype::dcoord_t wiresideh = trunkwire0->sideh();
          sutype::dcoord_t wireedgel = std::min (trunkwire0->edgel(), trunkwire1->edgel());
          sutype::dcoord_t wireedgeh = std::max (trunkwire0->edgeh(), trunkwire1->edgeh());

          SUASSERT (wireedgel < wireedgeh, "");

          suWire * trunkwire2 = suWireManager::instance()->create_wire_from_edge_side (net,
                                                                                       trunkwire0->layer(),
                                                                                       wireedgel,
                                                                                       wireedgeh,
                                                                                       wiresidel,
                                                                                       wiresideh,
                                                                                       sutype::wt_trunk); // merged trunk wires
          
          suWireManager::instance()->release_wire (trunkwire0);
          suWireManager::instance()->release_wire (trunkwire1);

          alltrunkwires[t0] = trunkwire2;
          alltrunkwires[t1] = alltrunkwires.back();
          alltrunkwires.pop_back();

          --t0;
          break;
        }
      }    
      
      // delete trunkwires which are not trunkbboxedgel-2-trunkbboxedgeh
      sutype::dcoord_t trunkbboxedgel = defaulttrunkbbox.edgel (pgd);
      sutype::dcoord_t trunkbboxedgeh = defaulttrunkbbox.edgeh (pgd);

      for (sutype::svi_t t0 = 0; t0 < (sutype::svi_t)alltrunkwires.size(); ++t0) {

        suWire * trunkwire0 = alltrunkwires[t0];
        //SUASSERT (trunkwire0->edgel() >= trunkbboxedgel, "");
        //SUASSERT (trunkwire0->edgeh() <= trunkbboxedgeh, "");
        
        if (trunkwire0->edgel() <= trunkbboxedgel && trunkwire0->edgeh() >= trunkbboxedgeh) continue;
        
        SUISSUE("Wire can't be a trunk for a global route") << ": wire=" << trunkwire0->to_str() << "gr=" << globalroute->to_str() << std::endl;
        
        suWireManager::instance()->release_wire (trunkwire0);
        
        alltrunkwires[t0] = alltrunkwires.back();
        alltrunkwires.pop_back();
        --t0;
      }

      // leave only unique trunk wires
      // if usage > 1 then another CE owns absolutely the same trunk wire
      for (sutype::svi_t t0=0; t0 < (sutype::svi_t)alltrunkwires.size(); ++t0) {
        
        suWire * trunkwire = alltrunkwires[t0];
        sutype::svi_t usage = suWireManager::instance()->get_wire_usage (trunkwire);
        if (usage == 1) continue;
        
        for (sutype::svi_t u=1; u < usage; ++u) {
          suWireManager::instance()->release_wire (trunkwire);
        }

        alltrunkwires[t0] = alltrunkwires.back();
        alltrunkwires.pop_back();
        --t0;        
      }
      
      if (alltrunkwires.empty()) {
        
        SUISSUE("Global route is unfeasible")
          << ": gr=" << globalroute->to_str()
          << std::endl;

        if (option_auto_fix_global_routing) {
          create_new_global_routes_ (globalroute, true,  globalroutes, 0); // true   = upper
          create_new_global_routes_ (globalroute, false, globalroutes, 0); // falase = below
        }
        
        continue;
      }
      
      std::sort (alltrunkwires.begin(), alltrunkwires.end(), suStatic::compare_wires_by_sidel);

      sutype::dcoord_t minWireWidth = globalroute->minWireWidth();
      
      // trim unfeasible and redundant trunk wires
      if (1) {

        sutype::uvi_t counter = 0;
        sutype::uvi_t numUnfeasible = 0;
        sutype::uvi_t numRedundant = 0;

        for (sutype::uvi_t w=0; w < alltrunkwires.size(); ++w) {
                    
          suWire * trunkwire = alltrunkwires[w];
          SUASSERT (trunkwire->is (sutype::wt_trunk), "");

          // unfeasible
          if (!wire_is_legal_slow_check (trunkwire)) {
            suWireManager::instance()->release_wire (trunkwire);
            ++numUnfeasible;
            continue;
          }

          // redundant
          if (net->wire_is_covered_by_a_preroute (trunkwire)) {
            minWireWidth -= trunkwire->width();
            SUINFO(1) << "Wire is redundant: " << trunkwire->to_str() << std::endl;
            suWireManager::instance()->release_wire (trunkwire);
            ++numRedundant;
            continue;
          }
          
          alltrunkwires[counter] = trunkwire;
          ++counter;
        }

        if (counter != alltrunkwires.size())
          alltrunkwires.resize (counter);

        // no available tracks
        if (alltrunkwires.empty() && numRedundant == 0) {
          
          SUISSUE("Global route is unfeasible")
            << ": " << globalroute->to_str()
            << std::endl;

          if (option_auto_fix_global_routing) {
            create_new_global_routes_ (globalroute, true,  globalroutes, 0); // true  = upper
            create_new_global_routes_ (globalroute, false, globalroutes, 0); // false = below
          }

          continue;
        }
        
        if (alltrunkwires.empty() ||
            (minWireWidth <= 0 && numRedundant > 0)) {
          
          SUISSUE("Global route is redundant")
            << ": " << globalroute->to_str()
            << "; unfeasible wires: " << numUnfeasible
            << "; redundant wires: " << numRedundant
            << "; remained wires: " << alltrunkwires.size()
            << "; remained target wire width: " << minWireWidth
            << std::endl;

          // release remaining wires
          for (sutype::uvi_t w=0; w < alltrunkwires.size(); ++w) {
            suWire * trunkwire = alltrunkwires[w];
            suWireManager::instance()->release_wire (trunkwire);
          }
          
          continue;
        }
      }

      SUASSERT (!alltrunkwires.empty(), "");
      std::sort (alltrunkwires.begin(), alltrunkwires.end(), suStatic::compare_wires_by_sidel);

      // not sure this tiny GR connects anything real
      // create a few GRs below
      if (globalroute->length_in_regions() < suGlobalRouter::instance()->reasonable_gr_length()) {
        
        sutype::uvi_t numgrs0 = globalroutes.size();
        
        if (option_auto_fix_global_routing) {
          create_new_global_routes_ (globalroute, false, globalroutes, -1); // false = below
        }
        
        sutype::uvi_t numgrs1 = globalroutes.size();

        SUASSERT (numgrs1 >= numgrs0, "");
        
        if (numgrs1 > numgrs0) {
          SUISSUE("Not sure a small GR actually connects anything meaningful")
            << ": gr=" << globalroute->to_str()
            << "; created " << (numgrs1 - numgrs0) << " extra GR below"
            << "."
            << std::endl;
        }
      }
      
      // create extra trunks (a bit shorter)
      sutype::uvi_t numwires = alltrunkwires.size();

      for (sutype::uvi_t k=0; k < numwires; ++k) {

        break; // disabled for a while
        
        suWire * trunkwire1 = alltrunkwires[k];
        
        sutype::dcoord_t newedgel = trunkwire1->edgel() + lengthregionAF / 2;
        sutype::dcoord_t newedgeh = trunkwire1->edgeh() - lengthregionBB / 2;

        if (newedgeh <= newedgel) continue;
        if (newedgel == trunkwire1->edgel() && newedgeh == trunkwire1->edgeh()) continue;

        suWire * trunkwire2 = suWireManager::instance()->create_wire_from_edge_side (net,
                                                                                     trunkwire1->layer(),
                                                                                     newedgel,
                                                                                     newedgeh,
                                                                                     trunkwire1->sidel(),
                                                                                     trunkwire1->sideh(),
                                                                                     sutype::wt_trunk); // shorter trunk wire
        
        SUASSERT (wire_is_legal_slow_check (trunkwire2), "");
        SUASSERT (wire_is_legal_slow_check (trunkwire2), "");
        
        alltrunkwires.push_back (trunkwire2);
      }

      std::sort (alltrunkwires.begin(), alltrunkwires.end(), suStatic::compare_wires_by_sidel);
      
      suConnectedEntity * connectedentity = new suConnectedEntity (net);
      connectedentity->add_type (sutype::wt_trunk);
      connectedentity->trunkGlobalRoute (globalroute);
      connectedentity->set_interface_layer (globalroute->layer()->base());

      //const sutype::satindex_t openSatindex = suSatSolverWrapper::instance()->get_constant(0);
      const sutype::satindex_t openSatindex = option_allow_opens ? suSatSolverWrapper::instance()->get_next_sat_index() : suSatSolverWrapper::instance()->get_constant(0);
      connectedentity->openSatindex (openSatindex);
      
      suLayoutFunc * layoutfuncCE = connectedentity->layoutFunc();
      SUASSERT (layoutfuncCE, "");
      SUASSERT (layoutfuncCE->nodes().empty(), "");

      layoutfuncCE->func (sutype::logic_func_or);
      layoutfuncCE->sign (sutype::unary_sign_just);

      layoutfuncCE->add_leaf (connectedentity->openSatindex());
      suLayoutFunc * layoutfunc0 = layoutfuncCE->add_func (sutype::logic_func_and, sutype::unary_sign_just, SUFILELINE_DLF);
      
      // simplified model, I assume that
      //  1) all regions have the same set of metal template instances;
      //  2) single trunks accross all regions; no jogs
      if (1) {
        
        // create trunk wires
        suLayoutFunc * layoutfunc11 = layoutfunc0->add_func (sutype::logic_func_or, sutype::unary_sign_just, SUFILELINE_DLF);
        
        for (sutype::uvi_t w=0; w < alltrunkwires.size(); ++w) {
                    
          suWire * trunkwire = alltrunkwires[w];
          SUASSERT (trunkwire->is (sutype::wt_trunk), "");

          SUINFO(np) << "Created a trunk wire: " << trunkwire->to_str() << std::endl;
          
          connectedentity->add_interface_wire (trunkwire);
          register_a_sat_wire (trunkwire);
          layoutfunc11->add_leaf (trunkwire->satindex());
        }
        
        if (connectedentity->interfaceWires().empty()) {
          SUISSUE("Can't convert global route to trunks; all wires are unfeasible") << ": " << globalroute->to_str() << std::endl;
          delete connectedentity;
          continue;
        }
        
        // minimal total trunk total width          
        suLayoutFunc * widthfunc = create_a_counter_for_total_wire_width_ (connectedentity->interfaceWires(), minWireWidth);
        
        if (widthfunc == 0) {
          
          SUFATAL ("Could not model minimal wire width for global route")
            << ": " << globalroute->to_str()
            << std::endl;
        }
        
        if (widthfunc) {
          layoutfunc0->add_node (widthfunc);
        }
        //
        
      }
      
      add_connected_entity_ (connectedentity);
    }
    
  } // end of suSatRouter::convert_global_routing_to_trunks

  // create a tie if the distance between connected entities is less than some value
  void suSatRouter::create_ties ()
  {
    const bool np = false;

    SUINFO(1) << "Create ties." << std::endl;

    const suRegion * region = suGlobalRouter::instance()->get_region (0, 0);
    SUASSERT (region, "");

    sutype::dcoord_t window [2];
    window [sutype::go_ver] = region->bbox().h();
    window [sutype::go_hor] = region->bbox().w();

    bool option_disable_max_tie_length = suOptionManager::instance()->get_boolean_option ("disable_max_tie_length", false);
    if (option_disable_max_tie_length) {
      SUINFO(1) << "Maximal tie length is disabled due to option disable_max_tie_length." << std::endl;
      window [sutype::go_ver] = -1;
      window [sutype::go_hor] = -1;
    }
    
    for (const auto & iter1 : _connectedEntities) {
      
      sutype::id_t netid = iter1.first;
      auto & ces = iter1.second;

      const suNet * net = suCellManager::instance()->get_net_by_id (netid);
      SUASSERT (net, "");
      
      SUINFO(1) << "Create ties for net " << net->name() << " and its " << ces.size() << " connected entities:" << std::endl;

      sutype::uvi_t numPrerouteCEs = 0;
      sutype::uvi_t numTrunkCEs= 0;
      
      for (sutype::uvi_t i=0; i < ces.size(); ++i) {
        
        const suConnectedEntity * ce = ces[i];
        
        if      (ce->is (sutype::wt_preroute)) ++numPrerouteCEs;
        else if (ce->is (sutype::wt_trunk))    ++numTrunkCEs;
        else {
          SUASSERT (false, "");
        }
        
        SUINFO(1) << "  " << ce->to_str() << std::endl;
        SUASSERT (ce->get_interface_layer() != 0, ce->to_str());
        const sutype::wires_t & ceiwires = ce->interfaceWires();
        if (ceiwires.empty()) {
          SUISSUE("Connected entity has no interface wires") << ": " << ce->to_str() << std::endl;
          SUASSERT (false, "");
        }

        for (sutype::uvi_t k=0; k < ceiwires.size(); ++k) {
          suWire * wire = ceiwires[k];
          SUINFO(np) << "  " << "  " << wire->to_str() << std::endl;          
        }
      }
      
      SUINFO(1)
        << "Connected entities for net " << net->name()
        << ": preroutes=" << numPrerouteCEs
        << "; trunks=" << numTrunkCEs
        << std::endl;
      
      for (sutype::uvi_t i=0; i < ces.size(); ++i) {
        
        const suConnectedEntity * ce1 = ces[i];
        const sutype::wires_t & wires1 = ce1->interfaceWires();
        const suGlobalRoute * trunkgr1 = ce1->trunkGlobalRoute();

        for (sutype::uvi_t k=i+1; k < ces.size(); ++k) {
          
          const suConnectedEntity * ce2 = ces[k];
          const sutype::wires_t & wires2 = ce2->interfaceWires();
          const suGlobalRoute * trunkgr2 = ce2->trunkGlobalRoute();

          SUINFO(np)
            << "Try to create a tie"
            << " " << ce1->id() << "-" << ce2->id()
            << ":"
            << " between ce1=" << ce1->to_str() << " and ce2=" << ce2->to_str()
            << std::endl;

          suTie * tie = create_prerouted_tie_ (ce1, ce2);
          
          if (tie) {
            SUINFO(np) << "Created a prerouted tie: " << tie->to_str() << std::endl;
            add_tie_ (tie);
            continue;
          }
          
          // connect two preroutes
          if (ce1->is (sutype::wt_preroute) && ce2->is (sutype::wt_preroute)) {

            SUINFO(np)
              << "Connect two preroutes"
              << std::endl;

            // don't create ties between preroutes
            if (0) {
              SUINFO(np) << " Skipped: 1.0" << std::endl;
              continue;
            }
            
            const suLayer * player1 = ce1->get_interface_layer();
            //const suLayer * player2 = ce2->get_interface_layer();
            
            sutype::dcoord_t distpgd = calculate_distance_ (wires1, wires2, player1->pgd());
            if (window[player1->pgd()] >= 0 && distpgd > window[player1->pgd()]) {
              SUINFO(np) << " Skipped: 1.1" << std::endl;
              continue;
            }
            
            sutype::dcoord_t distogd = calculate_distance_ (wires1, wires2, player1->ogd());
            if ( window[player1->ogd()] >= 0 && distogd > window[player1->ogd()]) {
              SUINFO(np) << " Skipped: 1.2" << std::endl;
              continue;
            }

            if (potential_tie_is_most_likely_redundant_ (ce1, ce2, ces)) {
              SUINFO(np) << " Skipped: 1.3" << std::endl;
              continue;
            }
          }

          // create a tie if global routes have at least one common point
          if (trunkgr1 && trunkgr2) {

            bool maySkipThisTie = true;

//             if (trunkgr1->layer()->level() != trunkgr2->layer()->level()) {
//               if (!suLayerManager::instance()->layers_are_adjacent_metal_layers (trunkgr1->layer(), trunkgr2->layer())) {
//                 SUINFO(np) << " Skipped: 2.1" << std::endl;
//                 continue;
//               }
//             }

            // collinear same-layer GRs
            // trunks may be connected directly without any shunts
            if (trunkgr1->layer() == trunkgr2->layer() &&
                trunkgr1->bbox().distance_to (trunkgr2->bbox(), trunkgr1->layer()->ogd()) <= 1 &&
                trunkgr1->bbox().distance_to (trunkgr2->bbox(), trunkgr1->layer()->pgd()) <= 0) {
              maySkipThisTie = false;
            }
            
            if (maySkipThisTie && trunkgr1->bbox().has_at_least_common_point (trunkgr2->bbox())) {
              maySkipThisTie = false;
            }
            
            if (maySkipThisTie) {
              SUINFO(np) << " Skipped: 2.2" << std::endl;
              continue;
            }
          }

          if ((ce1->is (sutype::wt_preroute) && ce2->is (sutype::wt_trunk)) ||
              (ce2->is (sutype::wt_preroute) && ce1->is (sutype::wt_trunk))) {
            
            const suConnectedEntity * pce = ce1->is (sutype::wt_preroute) ? ce1 : ce2;
            const suConnectedEntity * tce = (pce == ce1) ? ce2 : ce1;

            SUASSERT (pce->is (sutype::wt_preroute), "");
            SUASSERT (tce->is (sutype::wt_trunk), "");

            const suLayer * player = pce->get_interface_layer();
            const suLayer * tlayer = tce->get_interface_layer();

            SUASSERT (player, "pce=" << pce->to_str());
            SUASSERT (tlayer, "tce=" << tce->to_str());
            
            // do not create a tie if perpendicular wires don't intersect
            if (player->pgd() != tlayer->pgd()) {
              
              sutype::dcoord_t distpgd = calculate_distance_ (wires1, wires2, player->pgd());
              
              if (window[player->pgd()] >= 0 && distpgd > window[player->pgd()]) {
                SUINFO(np) << " Skipped: 3.2" << std::endl;
                continue;
              }
              
              sutype::dcoord_t distogd = calculate_distance_ (wires1, wires2, player->ogd());
              
              if (window[player->ogd()] >= 0 && distogd > window[player->ogd()]) {
                SUINFO(np) << " Skipped: 3.3" << std::endl;
                continue;
              }
            }

            // do not create a tie if parallel wires are far away from each other
            else {
              
              sutype::dcoord_t distpgd = calculate_distance_ (wires1, wires2, player->pgd());
              if (window[player->pgd()] >= 0 && distpgd > window[player->pgd()]) {
                SUINFO(np) << " Skipped: 4.1" << std::endl;
                continue;
              }
              
              sutype::dcoord_t distogd = calculate_distance_ (wires1, wires2, player->ogd());
              if (window[player->ogd()] >= 0 && distogd > window[player->ogd()]) {
                SUINFO(np) << " Skipped: 4.2" << std::endl;
                continue;
              }
            }
          }
          
          SUASSERT (tie == 0, "");
          
          tie = new suTie (ce1, ce2);
          
          SUINFO(np)
            << "Created a tie: " << tie->to_str()
            << std::endl;
          
          add_tie_ (tie);
        }
      }
    }
    
    for (const auto & iter1 : _ties) {
      
      const sutype::id_t netid = iter1.first;
      const suNet * net = suCellManager::instance()->get_net_by_id (netid);
      const auto & ties = iter1.second;

      if (!ties.empty()) {
        SUINFO(1) << "Created " << ties.size() << " ties for net " << net->name() << std::endl;
        for (const auto & iter2 : ties) {
          suTie * tie = iter2;
          SUINFO(1) << "  " << tie->to_str() << std::endl;
        }
      }
    }

    //SUABORT;
    
  } // end of suSatRouter::create_ties

  //
  void suSatRouter::create_global_routing ()
  {
    sutype::ties_t ties;

    for (const auto & iter1 : _ties) {
      
      const auto & netties = iter1.second;
      SUASSERT (netties.size() == 1, "I don't expect more ties for pin checker mode");

      ties.insert (ties.end(), netties.begin(), netties.end());
    }
    
  } // end of suSatRouter::create_global_routing

  //
  void suSatRouter::prune_redundant_ties ()
  {
    for (int targetNumTrunkCes = 2; targetNumTrunkCes >= 1; --targetNumTrunkCes) {

      SUINFO(1)
        << "Prune redundant ties"
        << ": targetNumTrunkCes=" << targetNumTrunkCes
        << std::endl;
      
      for (const auto & iter1 : _ties) {
        
        const sutype::id_t netid = iter1.first;
        auto & netties = _ties[netid];
      
        std::map<int,sutype::ties_t> tiesPerLevelDiff;

        for (sutype::uvi_t i=0; i < netties.size(); ++i) {
        
          suTie * tie = netties[i];
        
          const suConnectedEntity * ce0 = tie->entity0();
          const suConnectedEntity * ce1 = tie->entity1();

          int numTrunkCes = 0;

          if (ce0->is (sutype::wt_trunk)) ++numTrunkCes;
          if (ce1->is (sutype::wt_trunk)) ++numTrunkCes;

          if (numTrunkCes != targetNumTrunkCes) continue;
        
          const suLayer * layer0 = ce0->get_interface_layer ();
          const suLayer * layer1 = ce1->get_interface_layer ();

          // good tie
          if (layer0->level() == layer1->level()) {
            continue;
          }
        
          // good tie
          if (layer0->level() != layer1->level()) {
            if (suLayerManager::instance()->layers_are_adjacent_metal_layers (layer0, layer1)) {
              continue;
            }
          }

          int leveldiff = abs (layer0->level() - layer1->level());
        
          tiesPerLevelDiff[-leveldiff].push_back (tie);
        }

        for (const auto & iter2 : tiesPerLevelDiff) {

          const sutype::ties_t & badties = iter2.second;

          for (const auto & iter3 : badties) {

            suTie * tie = iter3;
            const suConnectedEntity * ce0 = tie->entity0();
            const suConnectedEntity * ce1 = tie->entity1();

            bool ret = path_exists_between_two_connected_entities_ (ce0,
                                                                    ce1,
                                                                    true, // checkRoutes
                                                                    true, // useTrunksOnly
                                                                    tie); // tieToSkip
          
            if (!ret) continue;
          
            for (sutype::uvi_t i=0; i < netties.size(); ++i) {

              if (netties[i] != tie) continue;

              netties[i] = netties.back();
              netties.pop_back();
              break;
            }
          
            SUINFO(1) << "Deleted a redundant tie: " << tie->to_str() << std::endl;
            delete tie;
          }
        }
      }
    }
    
  } // end of suSatRouter::prune_redundant_ties

  //
  void suSatRouter::create_routes ()
  {
    SUINFO(1) << "Create grids." << std::endl;
    
    SUASSERT (_grids.empty(), "");
    suSatRouter::instance()->create_grids_for_generators (_grids);
    SUASSERT (!_grids.empty(), "");

    SUINFO(1) << "Create routes." << std::endl;

    for (const auto & iter1 : _ties) {
      
      const sutype::id_t netid = iter1.first;
      
      sutype::ties_t & ties = _ties[netid];
      if (ties.empty()) continue;
      
      const suNet * net = suCellManager::instance()->get_net_by_id (netid);
      SUASSERT (net->wire_data_model_is_legal(), "");
      
      SUINFO(1) << "Create routes for " << ties.size() << " ties of net " << net->name() << std::endl;
      
      for (sutype::uvi_t i=0; i < ties.size(); ++i) {
        
        suTie * tie = ties[i];
        if (!tie->routes().empty()) continue; // tie is rerouted; it already has routes
        
        create_routes_ (tie);
      }

      for (sutype::svi_t i=0; i < (sutype::svi_t)ties.size(); ++i) {

        suTie * tie = ties[i];
        if (!tie->routes().empty()) continue;

        SUINFO (1) << "Deleted a tie without routes: " << tie->to_str() << std::endl;
        
        delete tie;
        ties[i] = ties.back();
        ties.pop_back();
        --i;
      }

      std::sort (ties.begin(), ties.end(), suStatic::compare_ties_by_id);
    }

    for (const auto & iter1 : _ties) {
      
      const sutype::id_t netid = iter1.first;
      const suNet * net = suCellManager::instance()->get_net_by_id (netid);
      const auto & netties = iter1.second;

      if (!netties.empty()) {
        SUINFO(1) << "Created routes for " << netties.size() << " ties for net " << net->name() << std::endl;
        for (const auto & iter2 : netties) {
          suTie * tie = iter2;
          SUINFO(1) << "  " << tie->to_str() << std::endl;
        }
      }
    }

    std::map<const suConnectedEntity *, sutype::ties_t, suConnectedEntity::cmp_const_ptr> tiesPerConnectedEntity;

    for (const auto & iter1 : _ties) {
      
      //const sutype::id_t netid = iter1.first;
      //const suNet * net = suCellManager::instance()->get_net_by_id (netid);
      const auto & netties = iter1.second;
      
      for (const auto & iter2 : netties) {
        
        suTie * tie = iter2;
        const sutype::routes_t & routes = tie->routes();
        SUASSERT (!routes.empty(), "Unexpected to meet a tie without routes");
        
        tiesPerConnectedEntity [tie->entity0()].push_back (tie);
        tiesPerConnectedEntity [tie->entity1()].push_back (tie);
      }
    }

    for (const auto & iter1 : tiesPerConnectedEntity) {

      const suConnectedEntity * ce0 = iter1.first;
      const sutype::ties_t & ceties = iter1.second;

      SUASSERT (ce0->is (sutype::wt_preroute) || ce0->is (sutype::wt_trunk), "");
      SUASSERT (ce0->is (sutype::wt_preroute) != ce0->is (sutype::wt_trunk), "");

      if (!ce0->is (sutype::wt_preroute)) continue;

      sutype::uvi_t numTiesToPreroutes = 0;
      sutype::uvi_t numTiesToTrunks    = 0;

      for (const auto & iter2 : ceties) {

        suTie * tie = iter2;
        const suConnectedEntity * ce1 = tie->get_another_entity (ce0);

        SUASSERT (ce1 != ce0, "");
        SUASSERT (ce1->is (sutype::wt_preroute) || ce1->is (sutype::wt_trunk), "");
        SUASSERT (ce1->is (sutype::wt_preroute) != ce1->is (sutype::wt_trunk), "");

        if (ce1->is (sutype::wt_preroute)) ++ numTiesToPreroutes;
        if (ce1->is (sutype::wt_trunk))    ++ numTiesToTrunks;
      }

      //SUINFO(1) << "Connected entity " << ce0->to_str() << " has " << numTiesToPreroutes << " ties to preroutes and " << numTiesToTrunks << " ties to trunks." << std::endl;
      
      if (numTiesToTrunks == 0) {
        SUISSUE("Connected entity has no ties to trunks")
          << ": " << ce0->to_str()
          << std::endl;
      }
    }
    
  } // end of suSatRouter::create_routes
  
  //
  void suSatRouter::prune_ties ()
  {    
    SUINFO(1) << "Prune ties." << std::endl;
    
    for (const auto & iter1 : _ties) {
      
      const sutype::id_t netid = iter1.first;
      
      delete_unfeasible_net_ties_ (netid);
    }
    
  } // end of suSatRouter::prune_ties

  //
  void suSatRouter::emit_connected_entities ()
  {
    SUINFO(1) << "Emit connected entities." << std::endl;

    for (const auto & iter1 : _connectedEntities) {
      
      auto & ces = iter1.second;
      
      for (const auto & iter2 : ces) {
      
        suConnectedEntity * ce = iter2;
        SUINFO(1) << "Emit connected entity: " << ce->to_str() << std::endl;

        SUASSERT (ce->is (sutype::wt_preroute) || ce->is (sutype::wt_trunk), "");
        SUASSERT (!(ce->is (sutype::wt_preroute) && ce->is (sutype::wt_trunk)), "");

        SUASSERT (ce->layout_function_is_legal(), "");
        sutype::satindex_t satindex = ce->layoutFunc()->calculate_satindex();
        SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, ce->to_str());
        
        if (ce->is (sutype::wt_preroute)) {
          suSatSolverWrapper::instance()->emit_ALWAYS_ONE (satindex);
        }
        else if (ce->is (sutype::wt_trunk)) {
          
          if (_option_may_skip_global_routes) {
            // do nothing
          }
          else {
            suSatSolverWrapper::instance()->emit_ALWAYS_ONE (satindex);
          }
        }
        else {
          SUASSERT (false, "");
        }
      }
    }
    
//     // debug experiments
//     // emit desired number of tracks
//     if (1) {
      
//       const unsigned numTracksPerTrunk = 1;
      
//       for (const auto & iter1 : _connectedEntities) {
//         auto & ces = iter1.second;
//         for (const auto & iter2 : ces) {
//           const suConnectedEntity * ce = iter2;
//           const sutype::wires_t & wires = ce->wires();
//           sutype::clause_t & clause0 = suClauseBank::loan_clause ();
//           for (const auto & iter3 : wires) {
//             const suWire * wire = iter3;
//             if (!wire->is (sutype::wt_trunk)) continue;
//             clause0.push_back (wire->satindex());
//           }
//           if (clause0.empty()) {
//             suClauseBank::return_clause (clause0);
//             continue;
//           }
//           suSatSolverWrapper::instance()->emit_ALWAYS_ONE (suSatSolverWrapper::instance()->emit_GREATER_or_EQUAL_THEN (clause0, numTracksPerTrunk));
//           suClauseBank::return_clause (clause0);
//         }
//       }
//     }
        
  } // end of suSatRouter::emit_connected_entities

  //
  void suSatRouter::emit_conflicts ()
    const
  {
    sutype::wires_t wires;
    sutype::wires_t vias;
    
    for (sutype::uvi_t i=0; i < _satindexToPointer.size(); ++i) {
      
      suWire * wire = _satindexToPointer[i];
      if (!wire) continue;
      
      SUASSERT (wire->satindex() != sutype::UNDEFINED_SAT_INDEX, "");
      
      const suLayer * layer = wire->layer();
      SUASSERT (layer, "");

      if (layer->is (sutype::lt_via)) {
        vias.push_back (wire);
      }
      else {
        wires.push_back (wire);
      }
    }
    
    emit_conflicts_for_wires_ (wires);

    //emit_conflicts_for_vias_ (vias); // tofix; disabled for a while
    
  } // end of suSatRouter::emit_conflicts

  //
  void suSatRouter::emit_patterns ()
    const
  {
    SUINFO(1) << "Emit patterns." << std::endl;

    sutype::uvi_t totalNumEmittedPatterns = 0;
    
    for (sutype::uvi_t i=0; i < _satindexToPointer.size(); ++i) {
      
      suWire * wire = _satindexToPointer[i];
      
      if (!wire) continue;
      
      suPatternManager::instance()->create_raw_pattern_instances (wire);
    }

    SUINFO(1) << "Created " << suPatternManager::instance()->num_pattern_instances() << " raw pattern instances." << std::endl;
    
    suPatternManager::instance()->delete_incomplete_pattern_instances ();
    
    SUINFO(1) << "Created " << suPatternManager::instance()->num_pattern_instances() << " complete pattern instances." << std::endl;

    suPatternManager::instance()->delete_redundant_pattern_instances ();
    
    const std::vector<sutype::patterninstances_tc> & patternIdToInstances = suPatternManager::instance()->patternIdToInstances();

    for (const auto & iter1 : patternIdToInstances) {
      
      const sutype::patterninstances_tc & pis = iter1;
      if (pis.empty()) continue;
      
      for (const auto & iter2 : pis) {
        
        const suPatternInstance * pi = iter2;
        pi->layoutFunc()->simplify ();
      }
    }

    for (const auto & iter1 : patternIdToInstances) {
      
      const sutype::patterninstances_tc & pis = iter1;
      if (pis.empty()) continue;

      sutype::uvi_t numEmittedPatterns = 0;
      sutype::uvi_t numSkippedPatterns = 0;
      sutype::uvi_t numUnsolvablePatterns = 0;      
        
      for (const auto & iter2 : pis) {

        const suPatternInstance * pi = iter2;
        sutype::satindex_t satindex = pi->layoutFunc()->calculate_satindex();
        
        if (suSatSolverWrapper::instance()->is_constant (0, satindex)) {
          ++numSkippedPatterns;
          continue;
        }

        if (suSatSolverWrapper::instance()->is_constant (1, satindex)) {
          ++numUnsolvablePatterns;
          SUISSUE("Found unsolvable DRV") << ": pattern: " << pi->to_str() << std::endl;
          continue;
        }
        
        ++numEmittedPatterns;
        ++totalNumEmittedPatterns;
        
        suSatSolverWrapper::instance()->emit_ALWAYS_ZERO (satindex);
        //pi->print_pattern_instance (std::cout);
      }
      
      if (numEmittedPatterns) {
        const suPattern * pattern = pis.front()->pattern();
        SUINFO(1) << "Emitted " << numEmittedPatterns << " instances of pattern " << pattern->name() << " (" << pattern->comment() << ")." << std::endl;
      }
      
      if (numUnsolvablePatterns) {
        //SUASSERT (false, "Found unsolvable conflict");
      }
    }

    SUINFO(1) << "Emitted " << totalNumEmittedPatterns << " pattern instances." << std::endl;
    //SUABORT;
    
  } // end of suSatRouter::emit_patterns

  //
  void suSatRouter::emit_same_width_for_trunks ()
    const
  {
    SUINFO(1) << "Emit same width for trunks." << std::endl;

    sutype::uvi_t nummatches = 0;

    sutype::connectedentities_t trunkces;

    for (const auto & iter0 : _connectedEntities) {

      const auto & netces = iter0.second;

      for (const auto & iter1 : netces) {

        suConnectedEntity * ce = iter1;
        if (!ce->is (sutype::wt_trunk)) continue;

        const suGlobalRoute * gr = ce->trunkGlobalRoute();
        SUASSERT (gr, "");

        // this GR is isolated
        if (!gr->may_have_the_same_width_as_another_gr ()) continue;
        
        trunkces.push_back (ce);
      }
    }
    
    for (sutype::uvi_t i=0; i < trunkces.size(); ++i) {

      suConnectedEntity * ce0 = trunkces[i];
      const suGlobalRoute * gr0 = ce0->trunkGlobalRoute();

      for (sutype::uvi_t k=i+1; k < trunkces.size(); ++k) {
        
        suConnectedEntity * ce1 = trunkces[k];
        const suGlobalRoute * gr1 = ce1->trunkGlobalRoute();
        
        if (!gr1->must_have_the_same_width_as_another_gr (gr0)) continue;

        ++nummatches;

        // total width is emitted separately
        // here, it's enough to emit simple single constraints
        // mode 0: ce0 - ce1
        // mode 1: ce1 - ce0

        for (int mode = 0; mode <= 1; ++mode) {

          const sutype::wires_t & cewires0 = (mode == 0) ? ce0->interfaceWires() : ce1->interfaceWires();
          const sutype::wires_t & cewires1 = (mode == 0) ? ce1->interfaceWires() : ce0->interfaceWires();

          for (sutype::uvi_t w0 = 0; w0 < cewires0.size(); ++w0) {

            suWire * wire0 = cewires0[w0];
            SUASSERT (wire0, "");
            SUASSERT (wire0->is (sutype::wt_trunk), "");
            sutype::satindex_t satindex0 = wire0->satindex();
            SUASSERT (satindex0 != sutype::UNDEFINED_SAT_INDEX, "");
            if (suSatSolverWrapper::instance()->is_constant (0, satindex0)) continue;

            sutype::dcoord_t width0 = wire0->width();

            sutype::clause_t & clause0 = suClauseBank::loan_clause();
            clause0.push_back (-satindex0);

            sutype::clause_t & clause1 = suClauseBank::loan_clause();

            for (sutype::uvi_t w1 = 0; w1 < cewires1.size(); ++w1) {
            
              suWire * wire1 = cewires1[w1];
              SUASSERT (wire1, "");
              SUASSERT (wire1->is (sutype::wt_trunk), "");
              sutype::satindex_t satindex1 = wire1->satindex();
              SUASSERT (satindex1 != sutype::UNDEFINED_SAT_INDEX, "");
              if (suSatSolverWrapper::instance()->is_constant (0, satindex1)) continue;

              sutype::dcoord_t width1 = wire1->width();
              if (width1 != width0) continue;
              
              clause1.push_back (satindex1);
            }
            
            if (!clause1.empty()) {
              clause0.push_back (suSatSolverWrapper::instance()->emit_OR_or_return_constant (clause1));
            }

            suSatSolverWrapper::instance()->emit_OR_ALWAYS_ONE (clause0);

            suClauseBank::return_clause (clause1);
            suClauseBank::return_clause (clause0);
          }
        }
      }
    }

    SUINFO(1) << "Found " << nummatches << " matched pairs of routes." << std::endl;
    
  } // end of suSatRouter::emit_same_width_for_trunks

  //
  void suSatRouter::emit_trunk_routing ()
  {
    SUINFO(1) << "Emit trunk routing." << std::endl;

    // every trunk wire if it's used must have routes of two different ties
    for (const auto & iter1 : _connectedEntities) {

      sutype::id_t netid = iter1.first;
      if (_ties.count (netid) == 0) continue; // net has no ties

      const auto & netces = iter1.second;
      const sutype::ties_t & netties = _ties.at (netid);
      
      for (const auto & iter2 : netces) {

        // find a trunk connected entity
        const suConnectedEntity * ce = iter2;
        if (!ce->is (sutype::wt_trunk)) continue;

        sutype::ties_t ceties;

        for (const auto & iter3 : netties) {

          suTie * tie = iter3;
          if (tie->entity0() != ce && tie->entity1() != ce) continue;
          if (tie->routes().empty()) continue;
          SUASSERT (tie->satindex() != sutype::UNDEFINED_SAT_INDEX, "");

          ceties.push_back (tie);
        }
        
        const sutype::wires_t & cewires = ce->interfaceWires();
        
        // every trunk wire must be absent or it must belong to at least two routes of different ties
        //
        for (const auto & iter3 : cewires) {
          
          suWire * trunkwire = iter3;
          SUASSERT (trunkwire->is (sutype::wt_trunk), "");
          
          sutype::satindex_t trunkwiresatindex = trunkwire->satindex();
          SUASSERT (trunkwiresatindex != sutype::UNDEFINED_SAT_INDEX, "");

          // every satindex is an OR of routes of a particular tie - every such route uses this trunk wire
          sutype::clause_t & clause0 = suClauseBank::loan_clause ();
          
          for (const auto & iter4 : ceties) {
            
            suTie * tie = iter4;
            const sutype::routes_t & routes = tie->routes();

            // collect here all routes of the tie that have this trunkwire
            sutype::clause_t & clause1 = suClauseBank::loan_clause ();

            for (const auto & iter5 : routes) {

              suRoute * route = iter5;
              if (!route->has_wire (trunkwire)) continue;
              
              suLayoutFunc * layoutFunc = route->layoutFunc();
              SUASSERT (layoutFunc, "");
              sutype::satindex_t routesatindex = layoutFunc->satindex();
              SUASSERT (routesatindex != sutype::UNDEFINED_SAT_INDEX, "");

              clause1.push_back (routesatindex);
            }

            if (clause1.empty()) {
              suClauseBank::return_clause (clause1);
              continue;
            }

            if (clause1.size() == 1) {
              clause0.push_back (clause1.front());
              suClauseBank::return_clause (clause1);
              continue;
            }

            clause0.push_back (suSatSolverWrapper::instance()->emit_OR_or_return_constant (clause1));
            suClauseBank::return_clause (clause1);
          }

          // forbit a trunk wire if it has routes of less than 2 ties
          if (clause0.size() < 2) {
            SUINFO(1) << "Trunk wire has not enough ties: ce " << ce->to_str() << " has routes only of " << clause0.size() << " tie(s). Must be at least 2. Disabled: " << trunkwire->to_str() << std::endl;
            suSatSolverWrapper::instance()->emit_ALWAYS_ZERO (trunkwiresatindex);
            suClauseBank::return_clause (clause0);
            continue;
          }

          sutype::satindex_t out = suSatSolverWrapper::instance()->emit_GREATER_or_EQUAL_THEN (clause0, 2);
          suClauseBank::return_clause (clause0);

          // trunk wire must be absent or must have at least two routes applied in layout
          sutype::clause_t & clause2 = suClauseBank::loan_clause ();
          clause2.push_back (-trunkwiresatindex);
          clause2.push_back (out);
          suSatSolverWrapper::instance()->emit_OR_ALWAYS_ONE (clause2);
          
          suClauseBank::return_clause (clause2);
        }
        
        // if trunk wire w0 belongs to a route r1 of tie t1
        // all other sister trunk wires {w1, ..., wN} must be absent or have a route r2 of tie t1
        for (sutype::uvi_t i=0; i < cewires.size(); ++i) {
          
          suWire * trunkwire1 = cewires[i];

          sutype::satindex_t trunkwiresatindex1 = trunkwire1->satindex();
          SUASSERT (trunkwiresatindex1 != sutype::UNDEFINED_SAT_INDEX, "");

          for (sutype::uvi_t k=i+1; k < cewires.size(); ++k) {

            suWire * trunkwire2 = cewires[k];

            sutype::satindex_t trunkwiresatindex2 = trunkwire2->satindex();
            SUASSERT (trunkwiresatindex2 != sutype::UNDEFINED_SAT_INDEX, "");

            for (const auto & iter4 : ceties) {

              suTie * tie = iter4;
              const sutype::routes_t & routes = tie->routes();

              sutype::clause_t & clause1 = suClauseBank::loan_clause ();
              sutype::clause_t & clause2 = suClauseBank::loan_clause ();
              
              for (const auto & iter5 : routes) {
                
                suRoute * route = iter5;
                
                suLayoutFunc * layoutFunc = route->layoutFunc();
                SUASSERT (layoutFunc, "");
                sutype::satindex_t routesatindex = layoutFunc->satindex();
                SUASSERT (routesatindex != sutype::UNDEFINED_SAT_INDEX, "");
                
                if (route->has_wire (trunkwire1)) { clause1.push_back (routesatindex); }
                if (route->has_wire (trunkwire2)) { clause2.push_back (routesatindex); }
              }

              // trunkwire1 must be absent if one of routes2 is present; because trunkwire1 has no routes of this tie
              if (clause1.empty() && !clause2.empty()) {
                suSatSolverWrapper::instance()->emit_AND_ALWAYS_ZERO (trunkwiresatindex1, suSatSolverWrapper::instance()->emit_OR_or_return_constant (clause2));
              }
              // trunkwire2 must be absent if one of routes1 is present; because trunkwire1 has no routes of this tie
              else if (clause2.empty() && !clause1.empty()) {
                suSatSolverWrapper::instance()->emit_AND_ALWAYS_ZERO (trunkwiresatindex2, suSatSolverWrapper::instance()->emit_OR_or_return_constant (clause1));
              }
              
              // if two trunk wires are present - they must have routes of the same tie, i.e.:
              // trunkwire1 must be absert if one of routes2 is present and none of routes1 is present
              // trunkwire2 must be absert if one of routes1 is present and none of routes2 is present
              else if (!clause1.empty() && !clause2.empty()) {
                
                sutype::satindex_t orOfRoutes1 = suSatSolverWrapper::instance()->emit_OR_or_return_constant (clause1);
                sutype::satindex_t orOfRoutes2 = suSatSolverWrapper::instance()->emit_OR_or_return_constant (clause2);

                sutype::clause_t & clause0 = suClauseBank::loan_clause ();
                clause0.push_back (orOfRoutes1);
                clause0.push_back (trunkwiresatindex2);
                clause0.push_back (-orOfRoutes2);
                suSatSolverWrapper::instance()->emit_AND_ALWAYS_ZERO (clause0);

                clause0.clear();
                clause0.push_back (orOfRoutes2);
                clause0.push_back (trunkwiresatindex1);
                clause0.push_back (-orOfRoutes1);
                suSatSolverWrapper::instance()->emit_AND_ALWAYS_ZERO (clause0);

                suClauseBank::return_clause (clause0);
              }
              
              suClauseBank::return_clause (clause1);
              suClauseBank::return_clause (clause2);
            }
          }
        }

      }
    }
    
  } // end of suSatRouter::emit_trunk_routing

  // http://ru.wikipedia.org/wiki/%D0%9C%D0%B0%D1%82%D1%80%D0%B8%D1%86%D0%B0_%D0%B4%D0%BE%D1%81%D1%82%D0%B8%D0%B6%D0%B8%D0%BC%D0%BE%D1%81%D1%82%D0%B8
  // http://ru.wikipedia.org/wiki/%D0%90%D0%BB%D0%B3%D0%BE%D1%80%D0%B8%D1%82%D0%BC_%D0%A3%D0%BE%D1%80%D1%88%D0%B5%D0%BB%D0%BB%D0%B0
  void suSatRouter::emit_ties_and_routes ()
  {
    const bool option_allow_opens = suOptionManager::instance()->get_boolean_option ("allow_opens", false);

    SUINFO(1)
      << "Emit ties"
      << "; allow_opens=" << option_allow_opens
      << std::endl;
    
    // create satindices for ties and routes
    for (const auto & iter1 : _ties) {
      
      const sutype::id_t netid = iter1.first;
      const suNet * net = suCellManager::instance()->get_net_by_id (netid);
      
      const auto & entities = _connectedEntities [netid]; 
      const auto & netties  = _ties              [netid];
      
      SUINFO(1) << "Emit " << netties.size() << " ties for net " << net->name() << std::endl;
            
      for (const auto & iter2 : netties) {
        
        suTie * tie = iter2;
        SUASSERT (tie->satindex() == sutype::UNDEFINED_SAT_INDEX, "");
        SUASSERT (tie->openSatindex() == sutype::UNDEFINED_SAT_INDEX, "");
        
        // dummy check
        for (const auto & iter3 : entities) {
          SUASSERT (tie->net() == iter3->net(), "");
        }
        
        const sutype::routes_t & routes = tie->routes();
        
        if (routes.empty()) {
          SUISSUE("Tie has no routes") << ": tie " << tie->to_str() << std::endl;
          SUASSERT (false, "");
          continue;
        }
        
        // satindices of routes
        sutype::clause_t & clause = suClauseBank::loan_clause ();

        bool tieIsPrerouted = false;
        
        for (const auto & iter3 : routes) {
          
          suRoute * route = iter3;
          SUASSERT (route->layoutFunc(), "");
          SUASSERT (route->layoutFunc()->satindex() == sutype::UNDEFINED_SAT_INDEX, "");
          
          if (!route->layoutFunc()->is_valid()) {
            suSatRouter::print_layout_node (std::cout, route->layoutFunc(), "");
            SUASSERT (false, "Bad route of a tie: " << tie->to_str());
          }

          //SUINFO(1) << "routesatindex (B): " << route->layoutFunc()->satindex() << std::endl;
          
          sutype::satindex_t routesatindex = route->layoutFunc()->calculate_satindex();
          SUASSERT (routesatindex != sutype::UNDEFINED_SAT_INDEX, "");
          clause.push_back (routesatindex);

          //SUINFO(1) << "routesatindex (A): " << routesatindex << std::endl;
          
          if (suSatSolverWrapper::instance()->is_constant (1, routesatindex)) {
            tieIsPrerouted = true;
          }

          // debug
          if (0) {

            if (route->wires()[0]->id() == 1297 &&
                route->wires()[1]->id() == 1294) {

              suSatSolverWrapper::instance()->emit_ALWAYS_ONE (routesatindex);
              SUISSUE("Hardcoded a debug route") << std::endl;
            }
          }
          //
          
        }

        sutype::satindex_t openSatindex = suSatSolverWrapper::instance()->get_constant(0);

        // allow this tie to be open
        if (option_allow_opens && !tieIsPrerouted) {
          openSatindex = suSatSolverWrapper::instance()->get_next_sat_index();
        }
        
        clause.push_back (openSatindex);
        
        sutype::satindex_t tiesatindex = suSatSolverWrapper::instance()->emit_OR_or_return_constant (clause);
        
        tie->satindex (tiesatindex);
        tie->openSatindex (openSatindex);
        
        // route can't appear without a tie
        sutype::clause_t & clause1 = suClauseBank::loan_clause ();
        for (const auto & iter3 : clause) {
          sutype::satindex_t routesatindex = iter3;
          clause1.push_back (-routesatindex);
          clause1.push_back (tiesatindex);
          suSatSolverWrapper::instance()->emit_OR_ALWAYS_ONE (clause1);
          clause1.clear();
        }
        suClauseBank::return_clause (clause1);
        
        suClauseBank::return_clause (clause);        
      }
      
      emit_net_ties_ (entities, netties);
    }
    
    //
    std::map<sutype::satindex_t,sutype::clause_t> routesPerWire;

    // put wires per routes
    for (const auto & iter1 : _ties) {
      
      const sutype::id_t netid = iter1.first;
      const auto & netties  = _ties [netid];
      
      for (const auto & iter2 : netties) {
        
        suTie * tie = iter2;
        const sutype::routes_t & routes = tie->routes();

        for (const auto & iter3 : routes) {

          suRoute * route = iter3;
          const sutype::wires_t & wires = route->wires();
          SUASSERT (route->layoutFunc(), "");
          sutype::satindex_t routesatindex = route->layoutFunc()->satindex();
          SUASSERT (routesatindex != sutype::UNDEFINED_SAT_INDEX, "");

          for (const auto & iter4 : wires) {

            suWire * wire = iter4;
            if (!wire->is (sutype::wt_route) && !wire->is (sutype::wt_shunt)) continue;
            sutype::satindex_t wiresatindex = wire->satindex();
            
            routesPerWire[wiresatindex].push_back (wiresatindex);
          }
        }
      }
    }

    // any wt_route/wt_shunt wire can't appear without one of its routes
    for (auto & iter : routesPerWire) {
      
      sutype::satindex_t wiresatindex  = iter.first;
      sutype::clause_t & clause1       = iter.second; // possible routes that own this wire
      
      clause1.push_back (-wiresatindex); // can modify clause1; it's not used anymore
      suSatSolverWrapper::instance()->emit_OR_ALWAYS_ONE (clause1);
    }   
    
  } // end of suSatRouter::emit_ties_and_routes

  // an optional constraint; just to improve routing quality and to reduce redundant routes
  void suSatRouter::emit_tie_triangles ()
  {
    SUINFO(1) << "Emit tie triangles" << std::endl;
    
    for (const auto & iter0 : _connectedEntities) {

      const sutype::connectedentities_t & netces = iter0.second;

      for (sutype::uvi_t i0 = 0; i0 < netces.size(); ++i0) {

        suConnectedEntity * ce0 = netces [i0];

        for (sutype::uvi_t i1 = i0+1; i1 < netces.size(); ++i1) {

          suConnectedEntity * ce1 = netces [i1];
          
          suTie * tie01 = get_tie_ (ce0, ce1);
          if (!tie01) continue;

          for (sutype::uvi_t i2 = i1+1; i2 < netces.size(); ++i2) {

            suConnectedEntity * ce2 = netces [i2];

            suTie * tie02 = get_tie_ (ce0, ce2);
            if (!tie02) continue;

            suTie * tie12 = get_tie_ (ce1, ce2);
            if (!tie12) continue;

            SUASSERT (tie01 != tie02, "");
            SUASSERT (tie01 != tie12, "");
            SUASSERT (tie02 != tie12, "");
            
            if (tie01->routes().empty()) continue;
            if (tie02->routes().empty()) continue;
            if (tie12->routes().empty()) continue;
            
            sutype::satindex_t tiesatindex01 = tie01->satindex();
            sutype::satindex_t tiesatindex02 = tie02->satindex();
            sutype::satindex_t tiesatindex12 = tie12->satindex();

            if (suSatSolverWrapper::instance()->is_constant (tiesatindex01)) continue;
            if (suSatSolverWrapper::instance()->is_constant (tiesatindex02)) continue;
            if (suSatSolverWrapper::instance()->is_constant (tiesatindex12)) continue;

            sutype::clause_t & clause = suClauseBank::loan_clause ();

            clause.push_back (tiesatindex01);
            clause.push_back (tiesatindex02);
            clause.push_back (tiesatindex12);

            sutype::satindex_t out = suSatSolverWrapper::instance()->emit_LESS_or_EQUAL_THEN (clause, 2);
            suSatSolverWrapper::instance()->emit_ALWAYS_ONE (out);
            
            suClauseBank::return_clause (clause);
          }
        }
      }
    }
    
  } // end of suSatRouter::emit_tie_triangles

  //
  void suSatRouter::emit_metal_fill_rules ()
  {
    SUINFO(1) << "Emit metal fill rules" << std::endl;

    emit_min_length_ ();
    
  } // end of suSatRouter::emit_metal_fill_rules

  //
  bool suSatRouter::solve_the_problem ()
    const
  {
    SUINFO(1) << "Solve the problem." << std::endl;

    if (1) {
      bool ok = suSatSolverWrapper::instance()->simplify();
      if (!ok) {
        SUINFO(1) << "*" << std::endl;
        SUINFO(1) << "*" << std::endl;
        SUINFO(1) << "*" << std::endl;
        SUINFO(1) << "* Cannot solve the problem." << std::endl;
        SUINFO(1) << "*" << std::endl;
        SUINFO(1) << "*" << std::endl;
        SUINFO(1) << "*" << std::endl;
        return false;
      }
    }
    
    if (1) {

      SUINFO(1) << "Try to route w/o any assumption." << std::endl;

      suSatSolverWrapper::instance()->print_statistic_of_emitted_clauses ();
      
      SUINFO(1) << "Solving started..." << std::endl;
      
      const double cputime0 = suTimeManager::instance()->get_cpu_time();
      bool ok = suSatSolverWrapper::instance()->solve_the_problem ();
      suStatic::increment_elapsed_time (sutype::ts_sat_initial_solving, suTimeManager::instance()->get_cpu_time() - cputime0);
      
      SUINFO(1) << "Solving ended." << std::endl;
      
      if (!ok) {
        SUINFO(1) << "*" << std::endl;
        SUINFO(1) << "*" << std::endl;
        SUINFO(1) << "*" << std::endl;
        SUINFO(1) << "* Cannot solve the problem." << std::endl;
        SUINFO(1) << "*" << std::endl;
        SUINFO(1) << "*" << std::endl;
        SUINFO(1) << "*" << std::endl;
        return false;
      }
    }

    return true;
    
  } // end of suSatRouter::solve_the_problem

  //
  // First, I minimize trunks; it creates a reasonable skeleton of the routing; then I remove the rest of wires
  void suSatRouter::optimize_routing ()
  {
    bool option_disable_optimization = suOptionManager::instance()->get_boolean_option ("disable_optimization", false);
    
    SUINFO(1) << "### OPT 1 ### Minimize the number of open connected entities." << std::endl;
    optimize_the_number_of_open_connected_entities_ ();
    //post_routing_fix_routes (false);
    
    SUINFO(1) << "### OPT 2 ### Minimize the number of open ties." << std::endl;
    optimize_the_number_of_open_ties_ ();
    //post_routing_fix_routes (false);

    if (option_disable_optimization) {
      SUINFO(1) << "Optimizations are disabled due to option disable_optimization." << std::endl;
      return;
    }
    
    SUINFO(1) << "### OPT 3 ### Optimize the number of direct I-routes." << std::endl;
    optimize_the_number_of_direct_wires_between_preroutes_ (false);
    post_routing_fix_routes (false);
    
    SUINFO(1) << "### OPT 4 ### Maximize the number of ties between trunks." << std::endl;
    optimize_the_number_of_ties_ (2, sutype::om_maximize, suGlobalRouter::instance()->reasonable_gr_length(), -1);
    post_routing_fix_routes (false);

    SUINFO(1) << "### OPT 5 ### Maximize the number of ties between trunks and preroutes." << std::endl;
    optimize_the_number_of_ties_ (1, sutype::om_maximize, suGlobalRouter::instance()->reasonable_gr_length(), -1);
    post_routing_fix_routes (false);
    
    SUINFO(1) << "### OPT 5 ### Minimize the number of trunk tracks." << std::endl;
    optimize_the_number_of_tracks_ (sutype::wt_trunk, false, false, -1);
    post_routing_fix_routes (false);

    SUINFO(1) << "### OPT 6 ### Minimize the number preroute extensions." << std::endl;
    minimize_the_number_of_preroute_extensions_ ();
    post_routing_fix_routes (false);
       
    if (1) {
      post_routing_fix_trunks ();
      post_routing_fix_routes (false);
    }
    
    SUINFO(1) << "### OPT 7 ### Minimize the number of wire tracks." << std::endl;
    optimize_the_number_of_tracks_ (sutype::wt_undefined, false, true, 1);
    post_routing_fix_routes (false);
        
    SUINFO(1) << "### OPT 8 ### Minimize the number of ties between preroutes." << std::endl;
    optimize_the_number_of_ties_ (0, sutype::om_minimize, -1, -1); // 3rd param works only for trunks
    post_routing_fix_routes (false);
    
    if (1) {
      SUINFO(1) << "### OPT 9 ### Maximize the number of routes between trunks and preroutes." << std::endl;
      optimize_the_number_of_routes_ (1, sutype::om_maximize);
    }

    SUINFO(1) << "### OPT 10 ### Minimize the number of wire tracks." << std::endl;
    optimize_the_number_of_tracks_ (sutype::wt_undefined, false, false, -1);
    post_routing_fix_routes (false);
    
    if (1) {
      SUINFO(1) << "### OPT 11 ### Maximize the number of routes between preroutes." << std::endl;
      optimize_the_number_of_routes_ (0, sutype::om_maximize);
    }
    
    if (1) {
      SUINFO(1) << "### OPT 12 ### Maximize the number of routes between trunks." << std::endl;
      optimize_the_number_of_routes_ (2, sutype::om_maximize);
    }

    if (1) {
      SUINFO(1) << "### OPT 13 ### Optimize widths of trunk wires."<< std::endl;
      optimize_wire_widths_ (sutype::wt_trunk);
      post_routing_fix_routes (false);
    }
    
    if (1) {
      SUINFO(1) << "### OPT 14 ### Optimize widths of shunt wires."<< std::endl;
      optimize_wire_widths_ (sutype::wt_shunt);
      post_routing_fix_routes (false);
    }

    if (1) {
      SUINFO(1) << "### OPT 15 ### Optimize connections to preroutes." << std::endl;
      optimize_connections_to_preroutes_ ();
      post_routing_fix_routes (false);
    }

    if (1) {
      SUINFO(1) << "### OPT 16 ### Optimize lengths of shunt wires."<< std::endl;
      optimize_wire_lengths_ (sutype::wt_shunt);
      post_routing_fix_routes (false);
    }

//     if (1) {
//       // no value in this optimization
//       SUINFO(1) << "### OPT 17 ### Optimize lengths of route wires."<< std::endl;
//       optimize_wire_lengths_ (sutype::wt_route);
//       post_routing_fix_routes (false);
//     }
    
  } // end of suSatRouter::optimize_routing

  //
  void suSatRouter::post_routing_fix_wires (const sutype::wires_t & wires)
    const
  {
    SUINFO(1) << "Fix wires." << std::endl;
    
    if (!suSatSolverWrapper::instance()->model_is_valid()) {
      const double cputime0 = suTimeManager::instance()->get_cpu_time();
      bool ok = suSatSolverWrapper::instance()->solve_the_problem ();
      suStatic::increment_elapsed_time (sutype::ts_sat_other_time, suTimeManager::instance()->get_cpu_time() - cputime0);
      SUASSERT (ok, "");
    }

    sutype::clause_t & clause0 = suClauseBank::loan_clause();
    sutype::clause_t & clause1 = suClauseBank::loan_clause();

    for (sutype::uvi_t i=0; i < wires.size(); ++i) {
      
      suWire * wire = wires[i];
      
      if (!wire) continue;
      
      sutype::satindex_t satindex = wire->satindex();      
      if (suSatSolverWrapper::instance()->is_constant (satindex)) continue;

      sutype::bool_t value = suSatSolverWrapper::instance()->get_modeled_value (satindex);
      
      if (value == sutype::bool_true) {
        clause1.push_back (satindex);
      }
      else {
        clause0.push_back (satindex);
      }
    }

    suSatSolverWrapper::instance()->keep_model_unchanged (true);
    
    if (!clause0.empty()) suSatSolverWrapper::instance()->emit_ALWAYS_ZERO (clause0);
    if (!clause1.empty()) suSatSolverWrapper::instance()->emit_ALWAYS_ONE  (clause1);
    
    suSatSolverWrapper::instance()->keep_model_unchanged (false);
    
    SUINFO(1) << "Made " << clause0.size() << " wires permanent absent." << std::endl;
    SUINFO(1) << "Made " << clause1.size() << " wires permanent present." << std::endl;
    
    suClauseBank::return_clause (clause1);
    suClauseBank::return_clause (clause0);

//     const double cputime1 = suTimeManager::instance()->get_cpu_time();
//     ok = suSatSolverWrapper::instance()->solve_the_problem ();
//     suStatic::increment_elapsed_time (sutype::ts_sat_other_time, suTimeManager::instance()->get_cpu_time() - cputime1);
//     SUASSERT (ok, "");
    
  } // end of suSatRouter::post_routing_fix_wires

  // we can make trunks always present because we have an optimization of tracks
  void suSatRouter::post_routing_fix_trunks ()
    const
  {
    SUINFO(1) << "Fix trunks." << std::endl;

    // tmp
    if (0) {
      SUINFO(1) << "DISABLED FOR A WHILE" << std::endl;
      return;
    }
    //
    
    sutype::wires_t wires;

    for (sutype::uvi_t i=0; i < _satindexToPointer.size(); ++i) {
      
      suWire * wire = _satindexToPointer[i];
      
      if (!wire) continue;
      if (!wire->is (sutype::wt_trunk)) continue;

      wires.push_back (wire);
    }

    post_routing_fix_wires (wires);
    
  } // end of suSatRouter::post_routing_fix_trunks

  //
  void suSatRouter::post_routing_delete_useless_generator_instances (bool checkCurrentModeledValue,
                                                                     const suNet * targetnet)
  {
    //SUINFO(1) << "Delete useless generator instances." << std::endl;

    if (checkCurrentModeledValue && !suSatSolverWrapper::instance()->model_is_valid()) {
      
      const double cputime0 = suTimeManager::instance()->get_cpu_time();
      bool ok = suSatSolverWrapper::instance()->solve_the_problem ();
      suStatic::increment_elapsed_time (sutype::ts_sat_other_time, suTimeManager::instance()->get_cpu_time() - cputime0);
      SUASSERT (ok, "");
    }
    
    sutype::uvi_t numDeletedGIs = 0;

    sutype::clause_t & deletedgis = suClauseBank::loan_clause();

    for (const auto & iter0 : _generatorinstances) {
      
      const auto & key1 = iter0.first;
      const auto & container1 = iter0.second;

      if (targetnet != 0) {
        sutype::id_t netid = key1;
        const suNet * net = suCellManager::instance()->get_net_by_id (netid);
        if (net != targetnet) continue;
      }
      
      for (const auto & iter1 : container1) {
        
        const auto & key2 = iter1.first;
        const auto & container2 = iter1.second;
        
        for (const auto & iter2 : container2) {
          
          const auto & key3 = iter2.first;
          const auto & container3 = iter2.second;

          for (const auto & iter3 : container3) {

            const auto & key4 = iter3.first;
            const auto & container4 = iter3.second;

            for (const auto & iter4 : container4) {
          
              const auto & key5 = iter4.first;
              //auto & gis = iter4.second;
              auto & gis = _generatorinstances[key1][key2][key3][key4][key5];
              
              for (sutype::svi_t i=0; i < (sutype::svi_t)gis.size(); ++i) {
            
                suGeneratorInstance * gi = gis[i];
            
                sutype::satindex_t gisatindex = gi->layoutFunc()->satindex();

                bool deleteThisGI = false;

                if (gisatindex == sutype::UNDEFINED_SAT_INDEX) {
                  deleteThisGI = true;
                }

                if (!deleteThisGI &&
                    checkCurrentModeledValue &&
                    suSatSolverWrapper::instance()->get_modeled_value (gisatindex) != sutype::bool_true) {
              
                  deleteThisGI = true;
                }
            
                if (!deleteThisGI) continue;
            
                delete gi;
                gis[i] = gis.back();
                gis.pop_back();
                --i;
            
                ++numDeletedGIs;

                if (gisatindex != sutype::UNDEFINED_SAT_INDEX) {
                  SUASSERT (checkCurrentModeledValue, "");
                  deletedgis.push_back (gisatindex);
                }
            
              }
            }
          }
        }
      }
    }
    
    if (numDeletedGIs) {
      SUINFO(1) << "Deleted " << numDeletedGIs << " useless generator instances not appplied so far." << std::endl;
    }

    if (!deletedgis.empty()) {

      SUASSERT (checkCurrentModeledValue, "");
      
      suSatSolverWrapper::instance()->emit_OR_ALWAYS_ZERO (deletedgis);
      
      const double cputime0 = suTimeManager::instance()->get_cpu_time();
      bool ok = suSatSolverWrapper::instance()->solve_the_problem ();
      suStatic::increment_elapsed_time (sutype::ts_sat_other_time, suTimeManager::instance()->get_cpu_time() - cputime0);
      SUASSERT (ok, "");
    }
    
    suClauseBank::return_clause (deletedgis);
    
  } // end of suSatRouter::post_routing_delete_useless_generator_instances
  
  //
  void suSatRouter::post_routing_fix_routes (bool deleteAbsentRoutes)
  {
    const bool np = false;

    //SUINFO(1) << "Fix routes." << std::endl;

    // tmp
    if (0) {
      SUINFO(1) << "DISABLED FOR A WHILE" << std::endl;
      return;
    }
    //

    // we may have wires and gis marked as constants
    // re-calculate satindex
    for (const auto & iter1 : _ties) {
      const auto & netties  = iter1.second;
      for (const auto & iter2 : netties) {
        suTie * tie = iter2;
        const sutype::routes_t & routes = tie->routes();
        for (const auto & iter3 : routes) {
          suRoute * route = iter3;

          if (np) {
            SUINFO(1) << std::endl;
            SUINFO(1) << "Before simplify:" << std::endl;
            suSatRouter::print_layout_node (std::cout, route->layoutFunc(), "");
          }
          
          if (_option_simplify_routes) {
            route->layoutFunc()->simplify();
          }
          
          if (np) {
            SUINFO(1) << "After simplify:" << std::endl;
            suSatRouter::print_layout_node (std::cout, route->layoutFunc(), "");
          }
        }
      }
    }
    
    const double cputime0 = suTimeManager::instance()->get_cpu_time();
    bool ok = suSatSolverWrapper::instance()->solve_the_problem ();
    suStatic::increment_elapsed_time (sutype::ts_sat_other_time, suTimeManager::instance()->get_cpu_time() - cputime0);
    SUASSERT (ok, "");

    sutype::uvi_t numUnfeasible = 0;
    sutype::uvi_t numUseless = 0;
    sutype::uvi_t numAlive = 0;

    sutype::clause_t & deletedroutesatindices = suClauseBank::loan_clause ();
    sutype::clause_t & presentroutesatindices = suClauseBank::loan_clause ();

    sutype::routes_t fixedroutes;
    
    for (const auto & iter1 : _ties) {
      const auto & netties  = iter1.second;
      for (const auto & iter2 : netties) {
        suTie * tie = iter2;
        sutype::routes_t & routes = tie->routes();

        for (sutype::svi_t r=0; r < (sutype::svi_t)routes.size(); ++r) {

          suRoute * route = routes[r];
          
          sutype::satindex_t routesatindex = route->layoutFunc()->satindex();

          if (suSatSolverWrapper::instance()->is_constant (0, routesatindex)) {

            deletedroutesatindices.push_back (routesatindex);
            delete route;
            routes[r] = routes.back();
            routes.pop_back();
            --r;
            ++numUnfeasible;
            continue;
          }

          if (!deleteAbsentRoutes) {
            ++numAlive;
            continue;
          }
          
          // pre-antenna action; fix present routes
          if (suSatSolverWrapper::instance()->get_modeled_value (routesatindex) == sutype::bool_true) {
            route->release_useless_wires ();
            ++numAlive;
            presentroutesatindices.push_back (routesatindex);
            fixedroutes.push_back (route);
            continue;
          }
          
          // pre-antenna action; delete absent routes
          deletedroutesatindices.push_back (routesatindex);
          delete route;
          routes[r] = routes.back();
          routes.pop_back();
          --r;
          ++numUseless;
        }
      }
    }

    if (numUnfeasible > 0) {
      SUINFO(1) << "Deleted " << numUnfeasible << " unfeasible routes." << std::endl;
    }
    
    if (numUseless > 0) {
      SUINFO(1) << "Deleted " << numUseless << " useless routes not appplied so far." << std::endl;
    }

    //SUINFO(1) << "Remained " << numAlive << " routes." << std::endl;

    if (!deletedroutesatindices.empty()) {
      SUINFO(1) << deletedroutesatindices.size() << " absent routes deleted." << std::endl;
      suSatSolverWrapper::instance()->keep_model_unchanged (true);
      suSatSolverWrapper::instance()->emit_OR_ALWAYS_ZERO (deletedroutesatindices);
      suSatSolverWrapper::instance()->keep_model_unchanged (false);
    }

    if (!presentroutesatindices.empty()) {
      
      SUINFO(1) << presentroutesatindices.size() << " present routes fixed." << std::endl;
      suSatSolverWrapper::instance()->keep_model_unchanged (true);
      suSatSolverWrapper::instance()->emit_AND_ALWAYS_ONE (presentroutesatindices);
      suSatSolverWrapper::instance()->keep_model_unchanged (false);

      // very good for antenna removal
      detect_mandatory_satindices_for_fixed_routes_ (fixedroutes);
    }
    
    suClauseBank::return_clause (presentroutesatindices);
    suClauseBank::return_clause (deletedroutesatindices);
    
  } // end of suSatRouter::post_routing_fix_routes

  void suSatRouter::eliminate_antennas ()
  {
    eliminate_antennas (sutype::wt_fill);
    eliminate_antennas (sutype::wt_undefined);
    
  } // end of suSatRouter::eliminate_antennas
  
  //
  void suSatRouter::eliminate_antennas (sutype::wire_type_t inputtargetwiretype)
  {
    const double cputime0 = suTimeManager::instance()->get_cpu_time();
        
    std::vector<sutype::wire_type_t> targetwiretypes;

    if (inputtargetwiretype != sutype::wt_undefined) {
      targetwiretypes.push_back (inputtargetwiretype);
    }
    else {
      targetwiretypes.push_back (sutype::wt_trunk); // no antenna satindices expected at this stage actually after suSatRouter::post_routing_fix_trunks
      targetwiretypes.push_back (sutype::wt_shunt);
      targetwiretypes.push_back (sutype::wt_route);
      targetwiretypes.push_back (sutype::wt_enclosure); // most of enclosures are out of any actual wires
      targetwiretypes.push_back (sutype::wt_fill);
    }

    SUASSERT (!targetwiretypes.empty(), "");
    sutype::bitid_t commontargetwiretype = 0;
    for (const auto & iter0 : targetwiretypes) {
      sutype::wire_type_t wt = iter0;
      commontargetwiretype |= wt;
    }

    SUINFO(1)
      << "Eliminate antennas"
      << "; target wire type: " << suStatic::wire_type_2_str (commontargetwiretype)
      << std::endl;
    
    sutype::clause_t & satindicesToCheckLazily = suClauseBank::loan_clause ();
    sutype::clause_t & satindicesToCheck       = suClauseBank::loan_clause ();
    sutype::clause_t & assumptions             = suClauseBank::loan_clause ();

    std::set<const suNet *, suNet::cmp_const_ptr> netsInUse;
    std::set<const suLayer *, suLayer::cmp_const_ptr_by_level> layersInUse;

    std::map<const suNet *, std::map <const suLayer *, sutype::wires_t, suLayer::cmp_const_ptr>, suNet::cmp_const_ptr> fixedWiresPerLayerPerNet;

    // wires    
    for (sutype::uvi_t i=0; i < _satindexToPointer.size(); ++i) {
      
      suWire * wire = _satindexToPointer[i];
      if (!wire) continue;

      sutype::satindex_t satindex = wire->satindex();    
      const suNet * net = wire->net();
      const suLayer * layer = wire->layer();

      if (suSatSolverWrapper::instance()->is_constant (1, satindex)) {
        fixedWiresPerLayerPerNet[net][layer].push_back (wire);
        continue;
      }
      
      if (suSatSolverWrapper::instance()->is_constant (0, satindex)) {
        continue;
      }

      if (!(wire->type() & commontargetwiretype)) continue;
      
      netsInUse.insert (net);
      layersInUse.insert (layer->base());
      
      sutype::bool_t value = suSatSolverWrapper::instance()->get_modeled_value (satindex, sutype::bool_false);
      
      if (value == sutype::bool_true) {
        satindicesToCheck.push_back (satindex);
        satindicesToCheckLazily.push_back (satindex); // try to eliminate lazily (i.e. without SAT) all possible wires of all layers and of all nets
      }
      else {
        assumptions.push_back (-satindex); // absent wires must remain absent
      }
    }
    
    const sutype::uvi_t numwires = satindicesToCheck.size();
    SUINFO(1) << "Found " << numwires << " objects to check." << std::endl;
    
    if (!assumptions.empty()) {
      
      SUINFO(0) << "Made " << assumptions.size() << " satindices permanent absent." << std::endl;
      suSatSolverWrapper::instance()->emit_ALWAYS_ONE (assumptions);
      assumptions.clear ();
      
      const double cputime1 = suTimeManager::instance()->get_cpu_time();
      bool ok = suSatSolverWrapper::instance()->solve_the_problem ();
      suStatic::increment_elapsed_time (sutype::ts_sat_other_time, suTimeManager::instance()->get_cpu_time() - cputime1);
      SUASSERT (ok, "");
    }

    std::map<sutype::satindex_t,sutype::routes_t> satindexToRoutes;
    
    SUASSERT (suSatSolverWrapper::instance()->model_is_valid(), "");
    
    for (const auto & iter1 : _ties) {
      const auto & netties = iter1.second;
      for (const auto & iter2 : netties) {
        suTie * tie = iter2;
        const sutype::routes_t & tieroutes = tie->routes();
        for (const auto & iter3 : tieroutes) {
          suRoute * route = iter3;
          sutype::satindex_t routesatindex = route->layoutFunc()->satindex();
          SUASSERT (routesatindex != sutype::UNDEFINED_SAT_INDEX, "");
          SUASSERT (suSatSolverWrapper::instance()->get_modeled_value (routesatindex) == sutype::bool_true, ""); // can be only true after post_routing_fix_routes
          SUASSERT (suSatSolverWrapper::instance()->is_constant (1, routesatindex), "");
          const sutype::wires_t & routewires = route->wires();
          for (const auto & iter4 : routewires) {
            suWire * wire = iter4;
            sutype::satindex_t wiresatindex = wire->satindex();
            SUASSERT (wiresatindex != sutype::UNDEFINED_SAT_INDEX, "");
            SUASSERT (wiresatindex > 0, "");            
            satindexToRoutes[wiresatindex].push_back (route);
          }
        }
      }
    }
    
    // eliminate constants
    update_affected_routes_ (satindexToRoutes);

    // sort nets by criticality
    // (not implemented yet)
    
    for (sutype::uvi_t t=0; t < targetwiretypes.size(); ++t) {

      sutype::wire_type_t targetwiretype = targetwiretypes[t];
      
      // eliminate antennas layer by layer
      for (const auto & iter0 : layersInUse) {

        const suLayer * baselayer = iter0;
        SUASSERT (baselayer->is_base(), "");

        // in current flow, antenna vias can't survide
        // First I eliminate all optional enclosures
        // Then, I call post_routing_delete_useless_generator_instances
        // It releases all useless wires including via cuts
        if (baselayer->is (sutype::lt_via)) continue;
        
        // eliminate antennas net by net
        for (const auto & iter1 : netsInUse) {

          SUASSERT (assumptions.empty(), "");

          const suNet * net = iter1;

          sutype::clause_t & satindicesToCheck2 = suClauseBank::loan_clause ();

          sutype::clause_t & mandatorySatindices = suClauseBank::loan_clause ();
          SUASSERT (suSatSolverWrapper::instance()->model_is_valid(), "");

          for (sutype::svi_t k=0; k < (sutype::svi_t)satindicesToCheck.size(); ++k) {

            sutype::satindex_t satindex = satindicesToCheck[k];
            if (suSatSolverWrapper::instance()->is_constant (satindex)) continue; // may become constant 0/1 because of extra checks (e.g. using satindicesToCheckLazily)

            const suWire * wire = get_wire_by_satindex (satindex);
            if (!wire) continue; // may be unreleased
        
            if (wire->layer()->base() != baselayer) continue;
            if (wire->net() != net) continue;

            sutype::wire_type_t wiretype = wire->get_wire_type();
            SUASSERT (wiretype != sutype::wt_undefined, "");

            if (targetwiretype != sutype::wt_undefined && !wire->is (targetwiretype)) continue;
            
            //
            if (satindexToRoutes.count (satindex) > 0) {
              
              bool satindexIsDefinitelyMandatory = false;
              const sutype::routes_t & routes = satindexToRoutes.at (satindex);
              for (const auto & iter2 : routes) {
                suRoute * route = iter2;
                sutype::satindex_t modeledsatindex = route->layoutFunc()->model_satindex (satindex, suSatSolverWrapper::instance()->get_constant(0));
                
                if (modeledsatindex != sutype::UNDEFINED_SAT_INDEX && suSatSolverWrapper::instance()->is_constant (0, modeledsatindex)) {
                  satindexIsDefinitelyMandatory = true;
                  break;
                }
              }
              
              if (satindexIsDefinitelyMandatory) {
                SUASSERT (suSatSolverWrapper::instance()->get_modeled_value (satindex) == sutype::bool_true, "");
                mandatorySatindices.push_back (satindex);
                satindicesToCheck[k] = satindicesToCheck.back();
                satindicesToCheck.pop_back();
                --k;
                continue;
              }
            }
            //

            // it's unexpected to see any antennas at this stage
            if (targetwiretype == sutype::wt_trunk || targetwiretype == sutype::wt_undefined) {
              
              SUISSUE("Unexpected antenna")
                << ": " << wire->to_str()
                << std::endl;
            }

            satindicesToCheck2.push_back (satindex);

            satindicesToCheck[k] = satindicesToCheck.back();
            satindicesToCheck.pop_back();
            --k;
          }

          if (!mandatorySatindices.empty()) {
            
            SUINFO(1)
              << "Detected " << mandatorySatindices.size() << " definitely mandatory wires"
              << std::endl;
            
            suSatSolverWrapper::instance()->keep_model_unchanged (true);
            suSatSolverWrapper::instance()->emit_AND_ALWAYS_ONE (mandatorySatindices);
            suSatSolverWrapper::instance()->keep_model_unchanged (false);
            
            update_affected_routes_ (satindexToRoutes);
          }
          suClauseBank::return_clause (mandatorySatindices);
          
          if (satindicesToCheck2.empty()) {
            suClauseBank::return_clause (satindicesToCheck2);
            continue;
          }
                      
          SUINFO(1)
            << "Eliminate antennas of layer: " << baselayer->name()
            << "; net: " << net->name()
            << "; wiretype: " << suStatic::wire_type_2_str (targetwiretype)
            << "; #indices0=" << satindicesToCheck2.size()
            << "; #indices1=" << satindicesToCheckLazily.size()
            << std::endl;

          remove_constanst_wires_ (satindicesToCheckLazily);
          
          sutype::uvi_t numsatindices = satindicesToCheck2.size();
          sutype::uvi_t nummandatory  = 0;

          update_affected_routes_ (satindexToRoutes);

          //SUASSERT (suSatSolverWrapper::instance()->model_is_valid(), "");

          //const double cputime2 = suTimeManager::instance()->get_cpu_time();
          eliminate_antennas_by_bisection_ (satindicesToCheck2, satindicesToCheckLazily, assumptions, nummandatory, fixedWiresPerLayerPerNet, satindexToRoutes);
          //const double cputime3 = suTimeManager::instance()->get_cpu_time();
          
          SUASSERT (satindicesToCheck2.empty(), "");
          SUASSERT (assumptions.empty(), "");
          SUASSERT (nummandatory <= numsatindices, "");
          
          sutype::uvi_t numoptional = (numsatindices - nummandatory);
          
          SUINFO(1) << "  " << nummandatory << " mandatory wires" << std::endl;
          SUINFO(1) << "  " << numoptional << " removed antennas" << std::endl;
          //SUINFO(1) << "  " << satindicesToCheckLazily.size() << " remained wires to check" << std::endl;
          //SUINFO(1) << "  Elapsed: " << (cputime3 - cputime2) << " sec." << std::endl;
          
          suClauseBank::return_clause (satindicesToCheck2);

          //SUASSERT (suSatSolverWrapper::instance()->model_is_valid(), "");
          if (!suSatSolverWrapper::instance()->model_is_valid()) {
            const double cputime1 = suTimeManager::instance()->get_cpu_time();
            bool ok = suSatSolverWrapper::instance()->solve_the_problem ();
            suStatic::increment_elapsed_time (sutype::ts_sat_other_time, suTimeManager::instance()->get_cpu_time() - cputime1);
            SUASSERT (ok, "");
          }
          
          if (numoptional > 0 && targetwiretype == sutype::wt_enclosure) {
            post_routing_delete_useless_generator_instances (true, net);
          }
          
        } // for(nets)
      } // for(layers)
    } // for(targetwiretype)
    
    SUASSERT (assumptions.empty(), "");
        
    // final call to create a legal satisfiable legal model
    if (suSatSolverWrapper::instance()->model_is_valid()) {
      const double cputime1 = suTimeManager::instance()->get_cpu_time();
      bool ok = suSatSolverWrapper::instance()->solve_the_problem (assumptions);
      suStatic::increment_elapsed_time (sutype::ts_sat_other_time, suTimeManager::instance()->get_cpu_time() - cputime1);
      SUASSERT (ok, "");
    }
    else {
      SUASSERT (false, "");
    }

    remove_constanst_wires_ (satindicesToCheckLazily);

    // vias may remain
    for (const auto & iter : satindicesToCheckLazily) {
      sutype::satindex_t satindex = iter;
      SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");
      suWire * wire = get_wire_by_satindex (satindex);
      SUASSERT (wire, "");
      //SUISSUE("Remained antenna wire") << ": " << wire->to_str() << std::endl;
    }
    
    suClauseBank::return_clause (assumptions);
    suClauseBank::return_clause (satindicesToCheck);
    suClauseBank::return_clause (satindicesToCheckLazily);

    suStatic::increment_elapsed_time (sutype::ts_antenna_total_time, suTimeManager::instance()->get_cpu_time() - cputime0);
    
  } // end of suSatRouter::eliminate_antennas

  //
  void suSatRouter::report_disconnected_entities ()
    const
  {
    SUINFO(1) << "Report layout opens." << std::endl;
    
    const sutype::nets_t & nets = suCellManager::instance()->topCellMaster()->nets();
    
    for (sutype::uvi_t i=0; i < nets.size(); ++i) {
      
      const suNet * net = nets[i];

      sutype::id_t netid = net->id();
      if (_connectedEntities.count (netid) == 0) continue; // do not report opens for net which were not routed
      
      std::vector<sutype::wires_t> ces = calculate_connected_entities_ (net);
      
      sutype::uvi_t numConnectedEntities = ces.size();
      
      SUASSERT (numConnectedEntities > 0, "");
      
      if (numConnectedEntities == 1) {
        SUINFO(1) << "Net " << net->name() << " has no opens." << std::endl;
        continue;
      }
      
      sutype::uvi_t numopens = (numConnectedEntities - 1);
      
      SUISSUE("Net has opens") << ": Net " << net->name() << " has " << numopens << " open" << ((numopens > 1) ? "s" : "") << std::endl;
    }
    
  } // end of suSatRouter::report_disconnected_entities

  //
  void suSatRouter::apply_solution ()
  {    
    sutype::uvi_t numwires = 0;
    sutype::uvi_t numgeneratorinstances = 0;

    std::set<suNet*, suNet::cmp_ptr> setOfModifiedNets;
    
    for (sutype::uvi_t i=0; i < _satindexToPointer.size(); ++i) {
      
      suWire * wire = _satindexToPointer[i];
      if (!wire) continue;

      sutype::satindex_t satindex = i;
      SUASSERT (satindex > 0, "");

      sutype::bool_t value = suSatSolverWrapper::instance()->get_modeled_value (satindex, sutype::bool_false);
      if (value != sutype::bool_true) continue;
      
      suNet * net = (suNet*)wire->net();
      
      //if (net->has_wire (wire)) continue; // may be stored as a preroute
      if (net->wire_is_redundant (wire)) continue;
      
      net->add_wire (suWireManager::instance()->create_wire_from_wire (net, wire)); // final wire
      setOfModifiedNets.insert (net);
      ++numwires;
    }

    for (const auto & iter0 : _generatorinstances) {
      const auto & container1 = iter0.second;
      for (const auto & iter1 : container1) {
        const auto & container2 = iter1.second;
        for (const auto & iter2 : container2) {
          const auto & container3 = iter2.second;
          for (const auto & iter3 : container3) {
            const auto & container4 = iter3.second;
            for (const auto & iter4 : container4) {
              const auto & container5 = iter4.second;
              for (const auto & iter5 : container5) {
          
                suGeneratorInstance * generatorinstance = iter5;
                SUASSERT (generatorinstance, "");
                suNet * net = (suNet*)generatorinstance->net();

                suLayoutFunc * layoutFunc = generatorinstance->layoutFunc();
                sutype::satindex_t satindex = layoutFunc->satindex();
                if (satindex == sutype::UNDEFINED_SAT_INDEX) continue;

                sutype::bool_t value = suSatSolverWrapper::instance()->get_modeled_value (satindex, sutype::bool_false);
                if (value != sutype::bool_true) continue;

                if (net->has_equal_generator_instance (generatorinstance)) continue; // may be stored as a preroute
                net->add_generator_instance (generatorinstance->create_clone (sutype::wt_route));
                setOfModifiedNets.insert (net);
                ++numgeneratorinstances;
              }
            }
          }
        }
      }
    }

    
    SUINFO(1) << "Found " << numwires << " registered SAT wires." << std::endl;
    SUINFO(1) << "Found " << numgeneratorinstances << " registered SAT generator instances." << std::endl;

    sutype::nets_t modifiednets;
    modifiednets.insert (modifiednets.end(), setOfModifiedNets.begin(), setOfModifiedNets.end());
    std::sort (modifiednets.begin(), modifiednets.end(), suStatic::compare_nets_by_id);
   
    // merge wires
    for (const auto & iter : modifiednets) {
      suNet * net = iter;
      net->merge_wires ();
      net->remove_useless_wires ();
    }
    
  } // end of suSatRouter::apply_solution

  //
  sutype::object_t suSatRouter::get_object_type_by_satindex (sutype::satindex_t satindex)
    const
  {
    suWire * wire = get_wire_by_satindex (satindex);
    if (wire) return sutype::obj_wire;

    suGeneratorInstance * gi = get_generator_instance_by_satindex (satindex);
    if (gi) return sutype::obj_generator_instance;

    return sutype::obj_none;
    
  } // end of suSatRouter::get_object_type_by_satindex

  //
  suWire * suSatRouter::get_wire_by_satindex (sutype::satindex_t satindex)
    const
  {
    SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");

    sutype::satindex_t abssatindex = abs (satindex);
    
    if (abssatindex >= (sutype::satindex_t)_satindexToPointer.size()) return 0;
    
    return _satindexToPointer[abssatindex];
    
  } // end of suSatRouter::get_wire_by_satindex

  //
  suGeneratorInstance * suSatRouter::get_generator_instance_by_satindex (sutype::satindex_t satindex)
    const
  {
    SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");

    sutype::satindex_t abssatindex = abs (satindex);

    for (const auto & iter0 : _generatorinstances) {
      const auto & container1 = iter0.second;
      for (const auto & iter1 : container1) {
        const auto & container2 = iter1.second;
        for (const auto & iter2 : container2) {
          const auto & container3 = iter2.second;
          for (const auto & iter3 : container3) {
            const auto & container4 = iter3.second;
            for (const auto & iter4 : container4) {
              const auto & container5 = iter4.second;
              for (const auto & iter5 : container5) {
        
                suGeneratorInstance * gi = iter5;
                suLayoutFunc * layoutFunc = gi->layoutFunc();
                SUASSERT (layoutFunc, "");
                if (abs (layoutFunc->satindex()) == abssatindex) return gi;
              }
            }
          }
        }
      }
    }
    
    return 0;
    
  } // end of suSatRouter::get_generator_instance_by_satindex

//
  void suSatRouter::print_applied_ties ()
    const
  {    
    SUINFO(1) << "Report applied ties:" << std::endl;

    for (const auto & iter1 : _ties) {
      
      const auto & ties = iter1.second;
            
      for (const auto & iter2 : ties) {
        
        suTie * tie = iter2;
        
        sutype::satindex_t tiesatindex = tie->satindex();
        if (suSatSolverWrapper::instance()->get_modeled_value (tiesatindex) != sutype::bool_true) continue;

        sutype::satindex_t openSatindex = tie->openSatindex();
        bool tieIsOpen = (suSatSolverWrapper::instance()->get_modeled_value (openSatindex) == sutype::bool_true);
        
        const sutype::routes_t & routes = tie->routes();
        sutype::uvi_t numappliedroutes = 0;
        
        for (const auto & iter3 : routes) {

          suRoute * route = iter3;

          suLayoutFunc * layoutFunc = route->layoutFunc();
          SUASSERT (layoutFunc, "");

          sutype::satindex_t routesatindex = route->layoutFunc()->satindex();
          SUASSERT (routesatindex != sutype::UNDEFINED_SAT_INDEX, "");

          sutype::bool_t value0 = suSatSolverWrapper::instance()->get_modeled_value (routesatindex);
          sutype::bool_t value1 = layoutFunc->get_modeled_value ();

          SUASSERT (value0 == value1, "");
          
          if (value0 != sutype::bool_true) continue;
          
          ++numappliedroutes;
        }
        
        SUINFO(1)
          << "Applied a tie"
          << " (" << numappliedroutes << " routes of " << routes.size() << ")"
          << "" << (tieIsOpen ? " (open)" : "")
          << ": " << tie->to_str() << std::endl;

        if (!tieIsOpen && numappliedroutes == 0) {
          SUASSERT (false, "Applied non-open tie has no applied routes: " << tie->to_str());
        }
        
      }
    }
    
  } // end of suSatRouter::print_applied_ties
    
  //
  void suSatRouter::print_applied_routes ()
    const
  {    
    SUINFO(1) << "Report applied ties and routes." << std::endl;

    for (const auto & iter1 : _ties) {
      
      const auto & ties = iter1.second;
            
      for (const auto & iter2 : ties) {
        
        suTie * tie = iter2;
        sutype::satindex_t tiesatindex = tie->satindex();
        
        if (suSatSolverWrapper::instance()->get_modeled_value (tiesatindex) == sutype::bool_true) {
          //SUINFO(1) << "Applied a tie: " << tie->to_str() << std::endl;
        }
        
        const sutype::routes_t & routes = tie->routes();
        
        for (const auto & iter3 : routes) {

          suRoute * route = iter3;

          suLayoutFunc * layoutFunc = route->layoutFunc();
          SUASSERT (layoutFunc, "");

          sutype::bool_t value = layoutFunc->get_modeled_value ();
          
          if (value != sutype::bool_true) continue;
          
          SUINFO(1) << "Applied a route of tie: " << tie->to_str() << std::endl;
          suSatRouter::print_layout_node (std::cout, layoutFunc, "");
          SUINFO(1) << std::endl;
        }
      }
    }
    
  } // end of suSatRouter::print_applied_routes
  
  //
  void suSatRouter::create_grids_for_generators (sutype::grids_t & grids)
    const
  {
    SUASSERT (grids.empty(), "");

    const sutype::metaltemplateinstances_t & mtis = suMetalTemplateManager::instance()->allMetalTemplateInstances();
    SUINFO(1) << "Found " << mtis.size() << " metal template instances." << std::endl;

    // I don't have a grid for vias
    // metal template instances define a grid of vias
    // I need to define for every via generator all possible locations and periods
    // Let's use a simplification that vias can be only on one grid

    std::set<std::string> warnings;

    for (sutype::uvi_t i=0; i < mtis.size(); ++i) {

      const suMetalTemplateInstance * mti0 = mtis[i];
      const suMetalTemplate * mt0 = mti0->metalTemplate();
      const suLayer * baselayer0 = mt0->baseLayer();
      const sutype::dcoords_t & widths0 = mt0->get_widths(mti0->isInverted());
      const sutype::layers_tc & layers0 = mt0->get_layers(mti0->isInverted());
      const sutype::dcoord_t period0 = mt0->pitch();
      const sutype::dcoords_t centerlines0 = mti0->get_center_lines (0, 0);
      const sutype::grid_orientation_t gd0 = baselayer0->pgd();
      
      for (sutype::uvi_t k=i+1; k < mtis.size(); ++k) {

        const suMetalTemplateInstance * mti1 = mtis[k];
        const suMetalTemplate * mt1 = mti1->metalTemplate();
        const suLayer * baselayer1 = mt1->baseLayer();
        const sutype::dcoords_t & widths1 = mt1->get_widths(mti1->isInverted());
        const sutype::layers_tc & layers1 = mt1->get_layers(mti1->isInverted());
        const sutype::dcoord_t period1 = mt1->pitch();
        const sutype::dcoords_t centerlines1 = mti1->get_center_lines (0, 0);
        const sutype::grid_orientation_t gd1 = baselayer1->pgd();
        
        if (gd0 == gd1) continue;

        const suLayer * cutlayer = suLayerManager::instance()->get_base_via_layer (baselayer0, baselayer1);
        if (cutlayer == 0) continue;

        //SUINFO(1) << "Found a base cut layer " << cutlayer->name() << " between baselayer0=" << baselayer0->name() << " and baselayer1=" << baselayer1->name() << std::endl;
        
        const sutype::generators_tc & generators = suGeneratorManager::instance()->get_generators (cutlayer);
        //SUINFO(1) << "Found " << generators.size() << " generators for layer " << cutlayer->name() << std::endl;
        if (generators.empty()) continue;

        const suMetalTemplateInstance * vermti = (baselayer0->pgd() == sutype::go_ver) ? mti0 : mti1;
        const suMetalTemplateInstance * hormti = (vermti == mti0) ? mti1 : mti0;

        // create a grid
        const sutype::dcoord_t xperiod = (baselayer0->pgd() == sutype::go_ver) ? period0 : period1;
        const sutype::dcoord_t yperiod = (baselayer0->pgd() == sutype::go_ver) ? period1 : period0;
        const sutype::dcoords_t & xdcoords = (baselayer0->pgd() == sutype::go_ver) ? centerlines0 : centerlines1;
        const sutype::dcoords_t & ydcoords = (baselayer0->pgd() == sutype::go_ver) ? centerlines1 : centerlines0;
        suGrid * grid = new suGrid (vermti, hormti, xdcoords, ydcoords, xperiod, yperiod);
        grids.push_back (grid);
        //
        
        for (sutype::uvi_t w0 = 0; w0 < widths0.size(); ++w0) {

          sutype::dcoord_t width0 = widths0[w0];
          const suLayer *  layer0 = layers0[w0];

          for (sutype::uvi_t w1 = 0; w1 < widths1.size(); ++w1) {

            sutype::dcoord_t width1 = widths1[w1];
            const suLayer *  layer1 = layers1[w1];

            sutype::uvi_t numgenerators = 0;

            for (const auto & iter0 : generators) {

              const suGenerator * generator = iter0;

              if (!generator->matches (layer0, layer1, width0, width1)) continue;

              sutype::dcoord_t centerline0 = mti0->get_center_line (w0);
              sutype::dcoord_t centerline1 = mti1->get_center_line (w1);
              
              sutype::dcoord_t xdcoord = (gd0 == sutype::go_ver) ? centerline0 : centerline1;
              sutype::dcoord_t ydcoord = (gd0 == sutype::go_ver) ? centerline1 : centerline0;
              
              grid->add_generator (generator, xdcoord, ydcoord);
              ++numgenerators;
            }

            // report this issue
            if (numgenerators == 0) {
              
              std::ostringstream oss;
              
              oss
                << mt0->name()
                << "/"
                << mt1->name()
                << ": No feasible generators " << cutlayer->to_str() << " between wires "
                << "(" << layer0->to_str() << " w=" << width0 << " mt=" << mt0->name() << ")"
                << " and "
                << "(" << layer1->to_str() << " w=" << width1 << " mt=" << mt1->name() << ")";
              
              warnings.insert (oss.str());
            }
            
          }
        }

        grid->precompute_data ();
      }
    }
    
    if (!warnings.empty()) {
      SUISSUE("Found grid issues") << ": " << warnings.size() << std::endl;
      for (const auto & iter : warnings) {
        const std::string & msg = iter;
        SUISSUE("Grid has an issue") << ": " << msg << std::endl;
      }
    }

    SUINFO(1) << "Created grids: " << grids.size() << "." << std::endl;

//     for (const auto & iter : grids) {
      
//       suGrid * grid = iter;
//       //SUINFO(1) << "" << std::endl;
//       //grid->print ();
//     }
    
  } // end of create_grids_for_generators

  //
  bool suSatRouter::wire_is_legal_slow_check (const suWire * wire1)
    const
  {
    bool verbose = false;
    
    return wire_is_legal_slow_check (wire1, verbose);
    
  } // end of suSatRouter::wire_is_legal_slow_check

  //
  bool suSatRouter::wire_is_legal_slow_check (const suWire * wire1,
                                              bool verbose)
    const
  {
    suNet * conflictingnet = 0;

    return wire_is_legal_slow_check (wire1, verbose, conflictingnet);
    
  } // end of suSatRouter::wire_is_legal_slow_check
  
  // a slow procedure checks any electrical shorts with preroutes and other conflicts
  bool suSatRouter::wire_is_legal_slow_check (const suWire * wire1,
                                              bool verbose,
                                              suNet * & conflictingnet)
    const
  {
    conflictingnet = 0;

    // check only wires for a while
    if (!wire1->layer()->is (sutype::lt_wire)) return true;

    //if (wire_is_out_of_boundaries_ (wire1)) return false;
    
    const sutype::nets_t & nets = suCellManager::instance()->topCellMaster()->nets();
    
    for (const auto & iter : nets) {
      
      suNet * net2 = iter;

      // 1: avoidUselessChecks = true
      // 0: avoidUselessChecks = false
      bool conflict1 = net2->wire_is_in_conflict (wire1, verbose, true);
      bool conflict0 = conflict1;

      // debug
      if (0) {
        conflict0 = net2->wire_is_in_conflict (wire1, verbose, false);
      }
      //

      SUASSERT (conflict1 == conflict0, "Data looks corrupted: net=" << net2->to_str() << "; wire1: " << wire1->to_str());
      
      if (conflict1) {
        conflictingnet = net2;
        return false;
      }
    }
    
    return true;
    
  } // end of suSatRouter::wire_is_legal_slow_check
  
  // ------------------------------------------------------------
  // -
  // --- Private static methods
  // -
  // ------------------------------------------------------------

  // very good for antenna removal
  void suSatRouter::detect_mandatory_satindices_for_fixed_routes_ (const sutype::routes_t & routes)
    const
  {
    const bool np = !false;

    SUASSERT (suSatSolverWrapper::instance()->model_is_valid(), "");

    sutype::uvi_t numwires = 0;
    sutype::uvi_t numgis = 0;
    sutype::uvi_t numgiwires = 0;
    sutype::uvi_t numsatindices = 0;

    // sanity check
    for (const auto & iter0 : routes) {
      suRoute * route = iter0;
      sutype::satindex_t routesatindex = route->layoutFunc()->satindex();
      SUASSERT (routesatindex != sutype::UNDEFINED_SAT_INDEX, "");
      SUASSERT (suSatSolverWrapper::instance()->is_constant (1, routesatindex), "");
    }
    
    while (true) {

      bool repeat = false;

      for (const auto & iter0 : routes) {

        suRoute * route = iter0;
        sutype::satindex_t routesatindex = route->layoutFunc()->satindex();
        SUASSERT (routesatindex != sutype::UNDEFINED_SAT_INDEX, "");

        // simplify may update routesatindex; if routesatindex is not a constant
        // it will become a constant in this turn
        //SUASSERT (suSatSolverWrapper::instance()->is_constant (1, routesatindex), ""); 
        
        if (_option_simplify_routes) {
          route->layoutFunc()->simplify();
        }
        route->release_useless_wires();
      
        sutype::clause_t & routesatindices = suClauseBank::loan_clause();
      
        // satindices are not sorted; satindices are not unique
        route->layoutFunc()->collect_satindices (routesatindices);
        suStatic::sort_and_leave_unique (routesatindices);
        
        for (sutype::uvi_t i=0; i < routesatindices.size(); ++i) {
          
          sutype::satindex_t satindex = routesatindices[i];
          SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");

          if (suSatSolverWrapper::instance()->is_constant (satindex)) continue; // nothing to do
          if (suSatSolverWrapper::instance()->get_modeled_value (satindex) != sutype::bool_true) continue; // can't be mandatory
        
          sutype::satindex_t modeledsatindex = route->layoutFunc()->model_satindex (satindex, suSatSolverWrapper::instance()->get_constant(0));
        
          if (modeledsatindex == sutype::UNDEFINED_SAT_INDEX) continue; // can't say definitely about this satindex
          if (!suSatSolverWrapper::instance()->is_constant (0, modeledsatindex)) continue; // can't say definitely about this satindex
        
          suWire * wire = get_wire_by_satindex (satindex);
          suGeneratorInstance * gi = get_generator_instance_by_satindex (satindex);

          bool foundNewConstant = false;

          if (wire) {
            
            SUINFO(np) << "Found mandatory wire: " << wire->to_str() << std::endl;

            suSatSolverWrapper::instance()->keep_model_unchanged (true);
            suSatSolverWrapper::instance()->emit_ALWAYS_ONE (satindex);
            suSatSolverWrapper::instance()->keep_model_unchanged (false);
            ++numwires;
            foundNewConstant = true;
            repeat = true;
          }

          else if (gi) {

            SUINFO(np) << "Found mandatory gi: " << gi->to_str() << std::endl;
          
            suSatSolverWrapper::instance()->keep_model_unchanged (true);
            suSatSolverWrapper::instance()->emit_ALWAYS_ONE (satindex);
            suSatSolverWrapper::instance()->keep_model_unchanged (false);
            ++numgis;
            foundNewConstant = true;
            repeat = true;

            const sutype::wires_t giwires = gi->wires();
          
            for (const auto & iter1 : giwires) {
            
              suWire * giwire = iter1;
              sutype::satindex_t satindex1 = giwire->satindex();
              SUASSERT (satindex1 != sutype::UNDEFINED_SAT_INDEX, "");
              if (suSatSolverWrapper::instance()->is_constant (satindex1)) continue; // nothing to do
              SUASSERT (suSatSolverWrapper::instance()->get_modeled_value (satindex1) == sutype::bool_true, "");
              SUINFO(np) << "Found mandatory gi wire: " << giwire->to_str() << std::endl;
              suSatSolverWrapper::instance()->keep_model_unchanged (true);
              suSatSolverWrapper::instance()->emit_ALWAYS_ONE (satindex1);
              suSatSolverWrapper::instance()->keep_model_unchanged (false);
              ++numgiwires;
              foundNewConstant = true;
              repeat = true;
            }
          }

          else {

            SUINFO(np) << "Found mandatory satindex: " << satindex << std::endl;
            
            suSatSolverWrapper::instance()->keep_model_unchanged (true);
            suSatSolverWrapper::instance()->emit_ALWAYS_ONE (satindex);
            suSatSolverWrapper::instance()->keep_model_unchanged (false);
            ++numsatindices;
            foundNewConstant = true;
            repeat = true;
          }

          // tofix; disabled; too heavy
          if (foundNewConstant) {
            if (_option_simplify_routes) {
              route->layoutFunc()->simplify();
            }
            route->release_useless_wires();
          }
        
        } // for(i/routesatindices)
      
        suClauseBank::return_clause (routesatindices);
       
      } // for(iter0/routes)

      if (!repeat) break;
    }

    // sanity check
    for (const auto & iter0 : routes) {
      suRoute * route = iter0;
      sutype::satindex_t routesatindex = route->layoutFunc()->satindex();
      SUASSERT (routesatindex != sutype::UNDEFINED_SAT_INDEX, "");
      
      if (!suSatSolverWrapper::instance()->is_constant (1, routesatindex)) {
        print_layout_node (std::cout, route->layoutFunc(), "");
        SUASSERT (false, "");
      }
    }
    
    SUINFO(1)
      << "Checked " << routes.size() << " fixed routes; found mandatory"
      << ": " << numwires << " wires"
      << "; " << numgis << " gis"
      << "; " << numgiwires << " gi wires"
      << "; " << numsatindices << " non-object satindices"
      << std::endl;
    
  } // end of suSatRouter::detect_mandatory_satindices_for_fixed_routes_

  //
  void suSatRouter::remove_constanst_wires_ (sutype::clause_t & satindices)
    const
  {
    sutype::uvi_t counter = 0;
            
    for (sutype::uvi_t i=0; i < satindices.size(); ++i) {
              
      sutype::satindex_t satindex = satindices[i];
      if (suSatSolverWrapper::instance()->is_constant (satindex)) continue;
      const suWire * wire = get_wire_by_satindex (satindex);
      if (!wire) continue; // may be unreleased
              
      satindices [counter] = satindex;
      ++counter;
    }
            
    if (counter != satindices.size()) {
      satindices.resize (counter);
    }
     
  } // end of suSatRouter::remove_constanst_wires_

  //
  std::vector<sutype::wires_t> suSatRouter::calculate_connected_entities_ (const suNet * net)
    const
  {
    // take layout objects
    const sutype::wires_t & netwires = net->wires();
    const sutype::generatorinstances_t & netgeneratorinstances = net->generatorinstances ();

    // create a copy
    sutype::wires_t allwires = netwires;

    for (const auto & iter1 : netgeneratorinstances) {
      const suGeneratorInstance * gi = iter1;
      const sutype::wires_t & giwires = gi->wires();
      for (const auto & iter2 : giwires) {
        suWire * giwire = iter2;
        allwires.push_back (giwire);
      }
    }

    SUASSERT (!allwires.empty(), "");

    std::vector<sutype::wires_t> ces;
           
    calculate_connected_entities_ (allwires, ces);

    return ces;
    
  } // end of suSatRouter::calculate_connected_entities_

  //
  void suSatRouter::calculate_connected_entities_ (sutype::wires_t & allwires,
                                                   std::vector<sutype::wires_t> & ces)
    const
  {
    while (1) {

      if (allwires.empty()) break;
      
      sutype::wires_t currentce;
      currentce.push_back (allwires.back());
      allwires.pop_back();

      while (true) {
    
        bool foundwire = false;

        for (sutype::uvi_t i=0; i < currentce.size(); ++i) {

          suWire * wire1 = currentce[i];

          for (sutype::svi_t k=0; k < (sutype::svi_t)allwires.size(); ++k) {
            
            suWire * wire2 = allwires[k];

            if (!wire1->layer()->is_electrically_connected_with (wire2->layer())) continue;
            if (!wire1->rect().has_at_least_common_point (wire2->rect())) continue;

            currentce.push_back (wire2);
          
            allwires[k] = allwires.back();
            allwires.pop_back();
            --k;

            foundwire = true;
          }
        }

        if (!foundwire) break;
      }

      SUASSERT (!currentce.empty(), "");
      ces.push_back (currentce);
    } 

  } // end of uSatRouter::calculate_connected_entities_
  
  // static
  void suSatRouter::print_layout_func_ (std::ostream & oss,
                                        const suLayoutFunc * func,
                                        const std::string & offset)
  {
    const sutype::layoutnodes_t & nodes = func->nodes();

    oss << (offset.length()/2) << "\t" << "|";
    
    oss << offset << "Node type=";

    if        (func->func() == sutype::logic_func_and && func->sign() == sutype::unary_sign_just) { oss << "AND";
    } else if (func->func() == sutype::logic_func_and && func->sign() == sutype::unary_sign_not)  { oss << "NAND";
    } else if (func->func() == sutype::logic_func_or  && func->sign() == sutype::unary_sign_just) { oss << "OR";
    } else if (func->func() == sutype::logic_func_or  && func->sign() == sutype::unary_sign_not)  { oss << "NOR";
    } else {
      SUASSERT (false, "");
    }

    sutype::satindex_t satindex = func->satindex();

    oss << " satindex=" << satindex;

    if (suSatSolverWrapper::instance()->is_constant (0, satindex)) { oss << " CONSTANT_0"; }
    if (suSatSolverWrapper::instance()->is_constant (1, satindex)) { oss << " CONSTANT_1"; }
    
    oss << " #nodes=" << nodes.size();

    oss << " {" << std::endl;

    for (int mode = 1; mode <= 2; ++mode) {

      for (const auto & iter : nodes) {

        const suLayoutNode * node = iter;
        if (mode == 1 && !node->is_leaf()) continue;
        if (mode == 2 && !node->is_func()) continue;

        suSatRouter::print_layout_node (oss, node, offset + "  ");
      }
    }

    oss << (offset.length()/2) << "\t" << "|";
    oss << offset << "}" << std::endl;
    
  } // end of suSatRouter::print_layout_func_

  // static
  void suSatRouter::print_layout_leaf_ (std::ostream & oss,
                                        const suLayoutLeaf * leaf,
                                        const std::string & offset)
  {
    sutype::satindex_t satindex = leaf->satindex();

    oss << (offset.length()/2) << "\t" << "|";

    oss << offset << "Node type=leaf pr=";
    if (satindex < 0)
      oss << "mba";
    else
      oss << "mbp";

    oss << " satindex=" << satindex;

    if (suSatSolverWrapper::instance()->is_constant (0, satindex)) {
      oss << " CONSTANT_0";
      //return;
    }

    if (suSatSolverWrapper::instance()->is_constant (1, satindex)) {
      oss << " CONSTANT_1";
      //return;
    }
    
    suWire * wire = suSatRouter::instance()->get_wire_by_satindex (satindex);
    if (wire == 0) {
      suGeneratorInstance * gi = suSatRouter::instance()->get_generator_instance_by_satindex (satindex);
      if (gi) {
        oss << " gi=" << gi->to_str();
      }
      else {
        oss << " obj=<unknown>";
      }
    }
    else {
      oss << " wire=" << wire->to_str();
    }

//     oss << " layer=" << wire->layer()->name();
//     oss << " rect=" << wire->rect().to_str(":");

//     oss << " w=" << wire->rect().w();
//     oss << " h=" << wire->rect().h();

//     if (wire->net()) {
//       oss << " net=" << wire->net()->name();
//     }
//     else {
//       oss << " net=<null>";
//     }
    
    oss << std::endl;
    
  } // end of suSatRouter::print_layout_leaf_

  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------

  //
  bool suSatRouter::wire_is_out_of_boundaries_ (const suWire * wire)
    const
  {
    const suRectangle & topCellMasterBbox = suCellManager::instance()->topCellMaster()->bbox();

    const suLayer * layer = wire->layer();
    if (!layer->is (sutype::lt_wire)) return false;

    SUASSERT (layer->has_pgd(), "");
    sutype::grid_orientation_t pgd = layer->pgd();

    sutype::dcoord_t topCellMasterBboxSidel = topCellMasterBbox.sidel (pgd);
    sutype::dcoord_t topCellMasterBboxSideh = topCellMasterBbox.sideh (pgd);
    sutype::dcoord_t topCellMasterBboxEdgel = topCellMasterBbox.edgel (pgd);
    sutype::dcoord_t topCellMasterBboxEdgeh = topCellMasterBbox.edgeh (pgd);

    if (wire->is (sutype::wt_shunt)) {

      if (wire->sidel() <= topCellMasterBboxSidel) return true;
      if (wire->sideh() >= topCellMasterBboxSideh) return true;
    }

    if (suRuleManager::instance()->rule_is_defined (sutype::rt_minete, layer)) {

      sutype::dcoord_t minete = suRuleManager::instance()->get_rule_value (sutype::rt_minete, layer);

      //bool ruleWorksForThisWire = (wire->is (sutype::wt_shunt) || wire->is (sutype::wt_route));
      bool ruleWorksForThisWire = (!wire->is (sutype::wt_preroute));

      if (ruleWorksForThisWire) {
              
        if (wire->edgeh() > topCellMasterBboxEdgeh - minete/2) return true;
        if (wire->edgel() < topCellMasterBboxEdgel + minete/2) return true;
      }
    }
    
    return false;
    
  } // end of suSatRouter::wire_is_out_of_boundaries_

  // return true if two preroutes have another preroute in between
  bool suSatRouter::potential_tie_is_most_likely_redundant_ (const suConnectedEntity * ce1,
                                                             const suConnectedEntity * ce2,
                                                             const sutype::connectedentities_t & ces)
    const
  {
    const bool np = (ce1->id() == 149 && ce2->id() == 151);
    
    SUASSERT (ce1->is (sutype::wt_preroute), "");
    SUASSERT (ce2->is (sutype::wt_preroute), "");
    SUASSERT (ce1 != ce2, "");
    
    const sutype::wires_t & wires1 = ce1->interfaceWires();
    const sutype::wires_t & wires2 = ce2->interfaceWires();

    suRectangle tmprect;

    for (const auto & iter1 : wires1) {

      suWire * wire1 = iter1;
      const suLayer * baselayer1 = wire1->layer()->base();

      for (const auto & iter2 : wires2) {

        suWire * wire2 = iter2;
        const suLayer * baselayer2 = wire2->layer()->base();

        SUASSERT (wire1 != wire2, "");

        if (baselayer1 != baselayer2) continue;

        sutype::dcoord_t x1 = std::max (wire1->rect().xl(), wire2->rect().xl());
        sutype::dcoord_t x2 = std::min (wire1->rect().xh(), wire2->rect().xh());
        sutype::dcoord_t y1 = std::max (wire1->rect().yl(), wire2->rect().yl());
        sutype::dcoord_t y2 = std::min (wire1->rect().yh(), wire2->rect().yh());

        sutype::dcoord_t xl = std::min (x1, x2);
        sutype::dcoord_t xh = std::max (x1, x2);
        sutype::dcoord_t yl = std::min (y1, y2);
        sutype::dcoord_t yh = std::max (y1, y2);

        tmprect.xl (xl);
        tmprect.xh (xh);
        tmprect.yl (yl);
        tmprect.yh (yh);
        
        SUINFO(np)
          << "tmprect: " << tmprect.to_str()
          << "; wire1=" << wire1->rect().to_str()
          << "; wire2=" << wire2->rect().to_str()
          << std::endl;
        
        for (const auto & iter3 : ces) {

          const suConnectedEntity * ce3 = iter3;
          if (ce3 == ce1) continue;
          if (ce3 == ce2) continue;
          if (!ce3->is (sutype::wt_preroute)) continue;

          const sutype::wires_t & wires3 = ce3->interfaceWires();

          for (const auto & iter4 : wires3) {

            suWire * wire3 = iter4;
            const suLayer * baselayer3 = wire3->layer()->base();

            if (baselayer3 != baselayer1) continue;

            if (wire3->rect().has_a_common_rect (tmprect)) return true;
          }
        }
      }
    }

    return false;
   
  } // end of suSatRouter::potential_tie_is_most_likely_redundant_

  //
  suTie * suSatRouter::get_tie_ (suConnectedEntity * ce0,
                                 suConnectedEntity * ce1)
    const
  {
    SUASSERT (ce0, "");
    SUASSERT (ce1, "");
    SUASSERT (ce0 != ce1, "");
    SUASSERT (ce0->net(), "");
    SUASSERT (ce1->net(), "");
    SUASSERT (ce0->net() == ce1->net(), "");
    
    sutype::id_t netid = ce0->net()->id();
    
    if (_ties.count (netid) == 0) return 0;
    
    const sutype::ties_t & netties = _ties.at (netid);
    
    for (const auto & iter : netties) {
      
      suTie * tie = iter;
      
      if (tie->entity0() == ce0 && tie->entity1() == ce1) return tie;
      if (tie->entity0() == ce1 && tie->entity1() == ce0) return tie;
    }
    
    return 0;
    
  } // end of suSatRouter::get_tie_

  void suSatRouter::emit_min_length_ ()
  {
    SUINFO(1) << "Emit min length" << std::endl;
    
    const sutype::layers_tc & idToLayer = suLayerManager::instance()->idToLayer();
    //const sutype::nets_tc & netsToRoute = suCellManager::instance()->netsToRoute();

    const sutype::dcoord_t step = 10; // 1nm

    const sutype::uvi_t initNumWires = _satindexToPointer.size();
    sutype::uvi_t numcreatedwires = 0;
    
    for (sutype::uvi_t i=0; i < idToLayer.size(); ++i) {

      const suLayer * layer0 = idToLayer[i];
      if (!layer0->is_base()) continue;
      if (!layer0->is (sutype::lt_metal)) continue;
      if (!suRuleManager::instance()->rule_is_defined (sutype::rt_minlength, layer0)) continue;
      if (!layer0->has_pgd()) continue;

      const sutype::dcoord_t minlength = suRuleManager::instance()->get_rule_value (sutype::rt_minlength, layer0);
     
      for (sutype::uvi_t k=0; k < initNumWires; ++k) {
        
        suWire * wire1 = _satindexToPointer[k];
        if (!wire1) continue;
        if (wire1->is (sutype::wt_fill)) continue;
        
        if (!wire1->is (sutype::wt_shunt) && !wire1->is (sutype::wt_preroute) && !wire1->is (sutype::wt_trunk)) continue;
        
        const suLayer * layer1 = wire1->layer();
        SUASSERT (layer1, "");
        if (layer1->base() != layer0->base()) continue;
        
        sutype::dcoord_t length1 = wire1->length();
        if (length1 >= minlength) continue;

        sutype::satindex_t satindex1 = wire1->satindex();
        SUASSERT (satindex1 != sutype::UNDEFINED_SAT_INDEX, "");
        if (suSatSolverWrapper::instance()->is_constant (0, satindex1)) continue; // unfeasible wire; nothing to extend

        // mti
        const suMetalTemplateInstance * mti = suMetalTemplateManager::instance()->get_best_mti_for_wire (wire1);
        if (!mti) continue;
        if (mti->metalTemplate()->stops().empty()) continue;
        
        const suNet * net1 = wire1->net();

        sutype::dcoord_t sidel1 = wire1->sidel();
        sutype::dcoord_t sideh1 = wire1->sideh();
        sutype::dcoord_t edgel1 = wire1->edgel();
        sutype::dcoord_t edgeh1 = wire1->edgeh();
        
        sutype::dcoord_t edgel2 = edgeh1 - minlength;
        sutype::dcoord_t edgeh2 = edgel1 + minlength;
        
        SUASSERT (edgel2 < edgeh2, "");

        sutype::wires_t wires;

        // mode 0: down
        // mode 1: up
        for (int mode = 1; mode <= 2; ++mode) {

          const bool lower = (mode == 1);
          
          sutype::dcoord_t startedge = (mode == 1) ? edgel1 : edgeh1;
          sutype::dcoord_t targtedge = (mode == 1) ? edgel2 : edgeh2;
          sutype::dcoord_t increment = (mode == 1) ? -step  : step;

          SUASSERT (startedge != targtedge, "");
          SUASSERT (increment != 0, "");

          sutype::dcoord_t e0 = startedge;

          while (1) {

            if (increment < 0 && e0 <= targtedge) break;
            if (increment > 0 && e0 >= targtedge) break;

            sutype::dcoord_t e1 = e0 + increment;
            sutype::dcoord_t e2 = mti->get_line_end_on_the_grid (e1, lower);

            if (lower) { SUASSERT (e2 <= e1, "");
            } else     { SUASSERT (e2 >= e1, ""); }

            // override to match line-end grid
            e1 = e2;
            
            suWire * wire2 = suWireManager::instance()->create_wire_from_edge_side (net1,
                                                                                    layer1,
                                                                                    ((increment > 0) ? e0 : e1),
                                                                                    ((increment > 0) ? e1 : e0),
                                                                                    sidel1,
                                                                                    sideh1,
                                                                                    sutype::wt_fill); // min length
            SUASSERT (wire2, "");
            SUASSERT (wire2->is (sutype::wt_fill), "");
            
            if (1) {
              suWireManager::instance()->keep_wire_as_illegal_if_it_is_really_illegal (wire2);
            }
            else {
              if (wire2 && wire2->satindex() == sutype::UNDEFINED_SAT_INDEX && !suSatRouter::instance()->wire_is_legal_slow_check (wire2)) {
                SUASSERT (suWireManager::instance()->get_wire_usage (wire2) == 1, "");
                suWireManager::instance()->release_wire (wire2);
                wire2 = 0;
              }
            }

            // re-use precomputed info
            if (wire2 && suWireManager::instance()->wire_is_illegal (wire2)) {
              SUASSERT (wire2->satindex() == sutype::UNDEFINED_SAT_INDEX, "");
              if (suWireManager::instance()->get_wire_usage (wire2) > 1) {
                SUASSERT (suWireManager::instance()->get_wire_usage (wire2) == 2, "");
                suWireManager::instance()->release_wire (wire2);
              }
              SUASSERT (suWireManager::instance()->get_wire_usage (wire2) == 1, "");
              SUASSERT (suWireManager::instance()->wire_is_illegal (wire2), "");
              wire2 = 0;
            }
            
            if (!wire2 || suWireManager::instance()->wire_is_illegal (wire2)) {
              break;
            }
            
            wires.push_back (wire2);

            e0 = e1;
            
          } // while(1)
        } // for(mode)
        
        // can't fix minlength for this wire
        if (wires.empty()) {
          suSatSolverWrapper::instance()->emit_ALWAYS_ZERO (satindex1);
          continue;
        }

        // sort
        std::sort (wires.begin(), wires.end(), suStatic::compare_wires_by_edgel);
        
        sutype::dcoord_t edgel4 = wires.front()->edgel();
        sutype::dcoord_t edgeh4 = wires.back() ->edgeh();

        // can't help this wire
        if (edgeh4 - edgel4 < minlength) {
          
          for (const auto & iter0 : wires) {
            suWire * wire2 = iter0;
            suWireManager::instance()->release_wire (wire2);
          }
          
          wires.clear();
          
          suSatSolverWrapper::instance()->emit_ALWAYS_ZERO (satindex1);
          continue;
        }
        
        // register wires
        for (const auto & iter0 : wires) {
          
          suWire * wire2 = iter0;
          sutype::satindex_t satindex2 = wire2->satindex();
          if (satindex2 != sutype::UNDEFINED_SAT_INDEX) continue;

          if (net1->wire_is_redundant (wire2)) continue;
          
          ++numcreatedwires;
          register_a_sat_wire (wire2);
        }

        bool doNotNeedToModelMinLength = false;

        // mode 0: do not emit
        // mode 1: emit
        for (int mode = 1; mode <= 2; ++mode) {

          if (mode == 2 && doNotNeedToModelMinLength) break;

          sutype::clause_t & clause1 = suClauseBank::loan_clause ();
          clause1.push_back (-satindex1);

          for (sutype::uvi_t j=0; j < wires.size(); ++j) {

            suWire * wire3 = wires[j];
            sutype::dcoord_t edgel3 = wire3->edgel();
          
            sutype::clause_t & clause0 = suClauseBank::loan_clause ();
            sutype::satindex_t satindex3 = wire3->satindex(); // undefined wires have satindex=sutype::UNDEFINED_SAT_INDEX
            if (satindex3 != sutype::UNDEFINED_SAT_INDEX)
              clause0.push_back (satindex3);
          
            sutype::dcoord_t targetedge = edgel3 + minlength;
            bool reachedMinLength = (wire3->edgeh() >= targetedge);
          
            for (sutype::uvi_t s=j+1; s < wires.size(); ++s) {
            
              if (reachedMinLength) break;
            
              suWire * wire4 = wires[s];
              sutype::satindex_t satindex4 = wire4->satindex();
              if (satindex4 == sutype::UNDEFINED_SAT_INDEX) continue; // skip redundant wire

              clause0.push_back (wire4->satindex());

              if (wire4->edgeh() >= targetedge) {
                reachedMinLength = true;
                break;
              }
            }
          
            if (!reachedMinLength) {
              suClauseBank::return_clause (clause0);
              continue;
            }

            if (clause0.empty()) {
              suClauseBank::return_clause (clause0);
              doNotNeedToModelMinLength = true;
              SUASSERT (mode == 1, "");
              break;
            }
          
            // emit
            if (mode == 2) {
              sutype::dcoord_t out = suSatSolverWrapper::instance()->emit_AND_or_return_constant (clause0);
              clause1.push_back (out);
            }
            
            suClauseBank::return_clause (clause0);
          }
          
          // emit
          if (mode == 2) {
            SUASSERT (doNotNeedToModelMinLength == false, "");
            suSatSolverWrapper::instance()->emit_OR_ALWAYS_ONE (clause1);
          }

          suClauseBank::return_clause (clause1);
          
        } // for(mode)
        
        // release redundant wires
        for (const auto & iter0 : wires) {
          
          suWire * wire2 = iter0;
          sutype::satindex_t satindex2 = wire2->satindex();
          if (satindex2 != sutype::UNDEFINED_SAT_INDEX) continue;
          suWireManager::instance()->release_wire (wire2);
        }
        
      } // for(k/_satindexToPointer)
    } // for(i/layers)

    SUINFO(1)
      << "Created " << numcreatedwires << " fill wires." << std::endl;
    
  } // end of suSatRouter::emit_min_length_

  //
  void suSatRouter::emit_conflicts_for_vias_ (const sutype::wires_t & inputwires)
    const
  {
    SUINFO(1)
      << "Emit conflicts for " << inputwires.size() << " vias"
      << std::endl;

    if (inputwires.empty()) return;

    const sutype::layers_tc & idToLayer = suLayerManager::instance()->idToLayer();

    std::vector<sutype::wires_t> wiresPerLayer [2];
    wiresPerLayer[0].resize (idToLayer.size());
    wiresPerLayer[1].resize (idToLayer.size());

    sutype::uvi_t numwires = 0;    
    sutype::uvi_t numskipped = 0;
    
    for (sutype::uvi_t i=0; i < inputwires.size(); ++i) {
      
      suWire * wire = inputwires[i];
      SUASSERT (wire, "");
      SUASSERT (wire->satindex() != sutype::UNDEFINED_SAT_INDEX, "");
      
      const suLayer * layer = wire->layer();
      SUASSERT (layer, "");
      SUASSERT (layer->pers_id() >= 0 && layer->pers_id() < (sutype::id_t)wiresPerLayer[0].size(), layer->to_str() << "; #layers=" << wiresPerLayer[0].size());

      if (!layer->is (sutype::lt_via)) {
        ++numskipped;
        continue;
      }
      
      ++numwires;
      
      // I don't check pgd/ogd for vias
      if (1 || !layer->has_pgd() || layer->pgd() == sutype::go_ver) wiresPerLayer [sutype::go_ver][layer->pers_id()].push_back (wire);
      if (1 || !layer->has_pgd() || layer->pgd() == sutype::go_hor) wiresPerLayer [sutype::go_hor][layer->pers_id()].push_back (wire);
    }
    
    SUINFO(1) << "Found " << numwires << " registered sat vias." << std::endl;
    SUINFO(1) << "Skipped " << numskipped << " wires." << std::endl;

    long int numconflicts = 0;

    // same-base-layer conflicts
    // 
    for (sutype::uvi_t i=0; i < idToLayer.size(); ++i) {

      const suLayer * layer0 = idToLayer[i];
      const sutype::wires_t & wires0 = wiresPerLayer[0][i];
      if (wires0.empty()) continue;

      for (sutype::uvi_t k=i+1; k < idToLayer.size(); ++k) {

        const suLayer * layer1 = idToLayer[k];
        const sutype::wires_t & wires1 = wiresPerLayer[0][k];
        if (wires1.empty()) continue;

        if (layer0->base() != layer1->base()) continue;

        for (const auto & iter0 : wires0) {

          const suWire * wire0 = iter0;

          for (const auto & iter1 : wires1) {

            const suWire * wire1 = iter1;

            numconflicts += emit_conflict_ (wire0, wire1);
          }
        }
      }
    }
    
    SUINFO(1) << "Emitted " << numconflicts << " pair conflicts between vias." << std::endl;
    
  } // end of suSatRouter::emit_conflicts_for_vias_
  
  //
  void suSatRouter::emit_conflicts_for_wires_ (const sutype::wires_t & inputwires)
    const
  {
    SUINFO(1)
      << "Emit conflicts for " << inputwires.size() << " wires"
      << std::endl;

    if (inputwires.empty()) return;
    
    const sutype::layers_tc & idToLayer = suLayerManager::instance()->idToLayer();

    std::vector<sutype::wires_t> wiresPerLayer [2];
    wiresPerLayer[0].resize (idToLayer.size());
    wiresPerLayer[1].resize (idToLayer.size());

    sutype::uvi_t numwires = 0;    
    sutype::uvi_t numskipped = 0;
    
    for (sutype::uvi_t i=0; i < inputwires.size(); ++i) {
      
      suWire * wire = inputwires[i];
      SUASSERT (wire, "");
      SUASSERT (wire->satindex() != sutype::UNDEFINED_SAT_INDEX, "");
      
      const suLayer * layer = wire->layer();
      SUASSERT (layer, "");
      SUASSERT (layer->pers_id() >= 0 && layer->pers_id() < (sutype::id_t)wiresPerLayer[0].size(), layer->to_str() << "; #layers=" << wiresPerLayer[0].size());

      if (layer->is (sutype::lt_via)) {
        ++numskipped;
        continue;
      }
      
      ++numwires;
      
      if (!layer->has_pgd() || layer->pgd() == sutype::go_ver) wiresPerLayer [sutype::go_ver][layer->pers_id()].push_back (wire);
      if (!layer->has_pgd() || layer->pgd() == sutype::go_hor) wiresPerLayer [sutype::go_hor][layer->pers_id()].push_back (wire);
    }
    
    SUINFO(1) << "Found " << numwires << " registered sat wires." << std::endl;
    SUINFO(1) << "Skipped " << numskipped << " vias." << std::endl;
    
    // sort wires and check sorting
    // mode 0: (sutype::grid_orientation_t)mode
    // mode 1: (sutype::grid_orientation_t)mode
    for (int mode = 0; mode < 2; ++mode) {
      
      sutype::grid_orientation_t gd = (sutype::grid_orientation_t)mode;
      SUASSERT (gd == sutype::go_ver || gd == sutype::go_hor, "");

      for (sutype::uvi_t k=0; k < wiresPerLayer[gd].size(); ++k) {

        const suLayer * layer = idToLayer[k];
        SUASSERT (layer->pers_id() == (sutype::id_t)k, "");
        
        sutype::wires_t & wires = wiresPerLayer[gd][k];

        suStatic::sort_wires (wires, gd);
        
        for (sutype::uvi_t i=0; i+1 < wires.size(); ++i) {

          if (!layer->has_pgd()) break;

          const suWire * wire1 = wires[i];
          const suWire * wire2 = wires[i+1];
          
          SUASSERT (wire2->sidel() >= wire1->sidel(), "");

          if (wire2->sidel() == wire1->sidel()) {
            SUASSERT (wire2->edgel() >= wire1->edgel(), "");
          }
          
        }
      }
    }

    long int numconflicts = 0;

    // same-layer same-color conflicts between wires
    //
    for (sutype::uvi_t j=0; j < idToLayer.size(); ++j) {

      const suLayer * layer1 = idToLayer[j];
      if (layer1->is (sutype::lt_via)) continue; // don't worry about vias because they always have enclosure wires
      
      const sutype::id_t layerid1 = layer1->pers_id();
      SUASSERT (idToLayer[layerid1] == layer1, "");
      
      // mode 0: gd = (sutype::grid_orientation_t)mode
      // mode 1: gd = (sutype::grid_orientation_t)mode
      for (int mode = 0; mode < 2; ++mode) {
        
        sutype::grid_orientation_t gd = (sutype::grid_orientation_t)mode;
        SUASSERT (gd == sutype::go_ver || gd == sutype::go_hor, "");

        const sutype::wires_t & wires = wiresPerLayer[gd][layerid1];
        
        for (sutype::uvi_t i=0; i < wires.size(); ++i) {

          const suWire * wire1 = wires[i];

          for (sutype::uvi_t k=i+1; k < wires.size(); ++k) {

            const suWire * wire2 = wires[k];
            SUASSERT (wire2->sidel() >= wire1->sidel(), "wire1=" << wire1->to_str() << "; wire2=" << wire2->to_str());
            
            if (wire2->sidel() > wire1->sideh()) break;
            //if (wire2->sidel() == wire1->sidel() && wire2->edgel() > wire1->edgeh()) break; // incorrect check! commented

            numconflicts += emit_conflict_ (wire1, wire2);
          }
        }
      }
    }

    SUINFO(1) << "Emitted " << numconflicts << " pair conflicts between wires." << std::endl;
    
  } // end of suSatRouter::emit_conflicts_for_wires_

  //
  suTie * suSatRouter::create_prerouted_tie_ (const suConnectedEntity * ce1,
                                              const suConnectedEntity * ce2)
  {
    const bool np = false;

    const sutype::wires_t & wires1 = ce1->interfaceWires();
    const sutype::wires_t & wires2 = ce2->interfaceWires();

    if (np) {

      SUINFO(1) << "wires1:" << std::endl;
      for (const auto & iter : wires1) {
        suWire * wire = iter;
        if (!wire->is (sutype::wt_preroute)) continue;
        SUINFO(1) << "  " << wire->to_str() << std::endl;
      }

      SUINFO(1) << "wires2:" << std::endl;
      for (const auto & iter : wires2) {
        suWire * wire = iter;
        if (!wire->is (sutype::wt_preroute)) continue;
        SUINFO(1) << "  " << wire->to_str() << std::endl;
      }
    }

    for (const auto & iter1 : wires1) {

      suWire * wire1 = iter1;
      if (!wire1->is (sutype::wt_preroute)) continue;

      for (const auto & iter2 : wires2) {

        suWire * wire2 = iter2;
        if (!wire2->is (sutype::wt_preroute)) continue;

        suRoute * route = create_route_between_two_wires_ (wire1,
                                                           wire2,
                                                           true);
        
        if (!route) continue;
        
        suTie * tie = new suTie (ce1, ce2);
        
        tie->add_route (route);

        SUINFO(np)
          << "Created a prerouted tie=" << tie->to_str()
          << std::endl;
        
        return tie;
      }
    }

    return 0;
    
  } // end of suSatRouter::create_prerouted_tie_

  // a slow procedure
  // a wire is obsolete if it's covered completely by a preroute
  // tested some ideas; but didn't find any benefit
  bool suSatRouter::wire_is_obsolete_ (const suWire * wire1)
    const
  {
    if (wire1->is (sutype::wt_preroute)) return false;
    
    const suNet * net = wire1->net();
    const sutype::wires_t & netwires = net->wires();
    
    for (const auto & iter : netwires) {

      suWire * wire2 = iter;

      if (!wire2->is (sutype::wt_preroute)) continue;
      if (wire2->layer() != wire1->layer()) continue;

      if (wire2->rect().covers_compeletely (wire1->rect())) {
        //SUISSUE("Wire is obsolete") << ": " << wire1->to_str() << std::endl;
        return true;
      }
    }
    
    return false;
    
  } // end of suSatRouter::wire_is_obsolete_

  //
  suGeneratorInstance * suSatRouter::create_generator_instance_ (const suGenerator * generator,
                                                                 const suNet * net,
                                                                 sutype::dcoord_t dx,
                                                                 sutype::dcoord_t dy,
                                                                 sutype::wire_type_t cutwiretype,
                                                                 bool verbose)
  {
    const bool np = false;

    SUASSERT (generator, "");
    SUASSERT (net, "");
    SUASSERT (cutwiretype == sutype::wt_preroute || cutwiretype == sutype::wt_route, "");
    SUASSERT (generator->get_cut_layer(), "");
    SUASSERT (generator->get_cut_layer()->is (sutype::lt_via), "");
    
    const sutype::id_t netid = net->id();
    const sutype::id_t layerid = generator->get_cut_layer()->pers_id();

    //sutype::wire_type_t cutwiretype_as_map_key = cutwiretype;
    sutype::wire_type_t cutwiretype_as_map_key = sutype::wt_undefined;
    
    if (_generatorinstances.count (netid) > 0) { // check netid
      
      const auto & container1 = _generatorinstances.at (netid);

      if (container1.count (layerid) > 0) { // check layerid

        const auto & container2 = container1.at (layerid);

        if (container2.count (cutwiretype_as_map_key) > 0) { // check cutwiretype
          
          const auto & container3 = container2.at (cutwiretype_as_map_key);

          if (container3.count (dx) > 0) { // check dx

            const auto & container4 = container3.at (dx);

            if (container4.count (dy) > 0) { // check dy
              
              const auto & container5 = container4.at (dy);
              
              for (const auto & iter5 : container5) {
                
                suGeneratorInstance * gi = iter5;
                
                if (gi->generator() != generator) continue;
                
                SUINFO(np) << "Reused gi: " << gi->to_str() << std::endl;
                return gi;
              }
            }
          }
        }
      }
    }
    
    suGeneratorInstance * generatorinstance = new suGeneratorInstance (generator, net, dx, dy, cutwiretype);
    
    _generatorinstances[netid][layerid][cutwiretype_as_map_key][dx][dy].push_back (generatorinstance);
    
    const sutype::wires_t & giwires = generatorinstance->wires();
    
    // create extra wires to match metal template instance
    const sutype::uvi_t numwires = giwires.size();
    
    for (sutype::uvi_t i=0; i < numwires; ++i) {
      
      suWire * wire = giwires[i];
      if (wire->layer()->is (sutype::lt_via)) continue;

      suWire * newwire = suMetalTemplateManager::instance()->create_wire_to_match_metal_template_instance (wire, wire->get_wire_type());
      if (newwire == wire) continue;
      
      // can't create a generator instance here; it's out of any legal mti -- we can't create a legal wire
      if (newwire == 0 || !wire_is_legal_slow_check (newwire, verbose)) {

        if (verbose) {
          
          SUISSUE("Could not find legal metal template instance to land an enclosure (written an error bad_encl)")
            << ": wire " << wire->to_str() << " of a via " << generatorinstance->to_str()
            << std::endl;
          
          suErrorManager::instance()->add_error (std::string("bad_encl_") + wire->layer()->name() + "_" + generatorinstance->generator()->get_cut_layer()->name(),
                                                 wire->rect().xl(),
                                                 wire->rect().yl(),
                                                 wire->rect().xh(),
                                                 wire->rect().yh());
        }

        if (_option_assert_illegal_input_wires) { 
          SUASSERT (false, "");
          generatorinstance->set_illegal ();
        }
        
        if (newwire) {
          suWireManager::instance()->release_wire (newwire);
          newwire = 0;
        }
        
        continue;
      }
      
      generatorinstance->add_wire (newwire);
    }

    // create this gi once even it's illegal; it's done not to create and delete it many times
    for (const auto & iter : giwires) {
      
      suWire * wire = iter;
      
      if (!wire_is_legal_slow_check (wire, false)) {
        
        generatorinstance->set_illegal ();
        break;
      }
    }

    // emit
    if (generatorinstance->legal()) {
      emit_generator_instance_ (generatorinstance);
    }
    
    if (np) {
      
      SUINFO(np) << "Created gi: " << generatorinstance->to_str() << std::endl;
      
      for (const auto & iter : giwires) {
        suWire * wire = iter;
        SUINFO(np) << "  Wire " << wire->to_str() << " belongs to gi: " << generatorinstance->to_str() << std::endl;
      }
    }
    
    return generatorinstance;
    
  } // end of suSatRouter::create_generator_instance_
  
  //
  void suSatRouter::emit_generator_instance_ (suGeneratorInstance * generatorinstance)
  {    
    const sutype::wires_t & giwires = generatorinstance->wires();
    
    suLayoutFunc * layoutfunc0 = generatorinstance->layoutFunc();
    SUASSERT (layoutfunc0, "");
    SUASSERT (layoutfunc0->nodes().empty(), "");
    SUASSERT (layoutfunc0->satindex() == sutype::UNDEFINED_SAT_INDEX, "");
    
    layoutfunc0->func (sutype::logic_func_and);
    layoutfunc0->sign (sutype::unary_sign_just);
    
    // I can't request that a via wire can't appear without enclosure wires
    // because an identical via wire may belong to different generator instances with different enclosure wires
    for (const auto & iter : giwires) {
      
      suWire * generatorwire = iter;
      register_a_sat_wire (generatorwire);
      sutype::satindex_t satindex = generatorwire->satindex();
      SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");
      
      layoutfunc0->add_leaf (satindex);
    }
    
    layoutfunc0->calculate_satindex ();

    // check
    if (0) {
      
      sutype::satindex_t gisatindex = layoutfunc0->satindex();
      
      for (const auto & iter : giwires) {
        suWire * generatorwire = iter;
        sutype::satindex_t satindex = generatorwire->satindex();
        if (satindex != gisatindex) continue;
        SUINFO(1) << "Generator instance and wire have the same satindex: " << generatorwire->to_str() << std::endl;
      }
    }
    
  } // end of suSatRouter::emit_generator_instance_

  //
  void suSatRouter::emit_conflicts_ (const suWire * wire1)
    const
  {
    for (sutype::uvi_t i=0; i < _satindexToPointer.size(); ++i) {
      
      suWire * wire2 = _satindexToPointer[i];
      if (!wire2) continue;
      
      SUASSERT (wire2->satindex() != sutype::UNDEFINED_SAT_INDEX, "");
      
      const suLayer * layer2 = wire2->layer();
      SUASSERT (layer2, "");

      if (layer2->is (sutype::lt_via)) continue;

      emit_conflict_ (wire1, wire2);
    }
    
  } // end of suSatRouter::emit_conflicts_
  
  // 
  long int suSatRouter::emit_conflict_ (const suWire * wire1,
                                        const suWire * wire2)
    const
  {
    // check is correct if we have a single common template for this layer
    if (0) {
      if (wire1->layer() == wire2->layer() && wire1->rect().has_at_least_common_point (wire2->rect()) && !wire1->is (sutype::wt_preroute) && !wire2->is (sutype::wt_preroute)) {
        SUASSERT (wire1->sidel() == wire2->sidel(), "wire1=" << wire1->to_str() << "; wire2=" << wire2->to_str());
        SUASSERT (wire1->sideh() == wire2->sideh(), "wire1=" << wire1->to_str() << "; wire2=" << wire2->to_str());
      }
    }
    //
    
    const sutype::satindex_t satindex1 = wire1->satindex();
    const sutype::satindex_t satindex2 = wire2->satindex();

    if (satindex1 == sutype::UNDEFINED_SAT_INDEX) return 0;
    if (satindex2 == sutype::UNDEFINED_SAT_INDEX) return 0;
        
    bool conflict = suSatRouter::wires_are_in_conflict (wire1, wire2);
    if (!conflict) return 0;
    
    if (wire1->is (sutype::wt_preroute) || wire2->is (sutype::wt_preroute)) {
      
      suErrorManager::instance()->add_error (std::string("conflict_preroutes"),
                                             wire1->rect());
      
      suErrorManager::instance()->add_error (std::string("conflict_preroutes"),
                                             wire2->rect());
      
      SUASSERT (false, "Found a conflict for a preroute wire1=" << wire1->to_str() << "; wire2=" << wire2->to_str());
    }
    
    suSatSolverWrapper::instance()->emit_AND_ALWAYS_ZERO (satindex1, satindex2);
    
    return 1;
    
  } // end of suSatRouter::emit_conflict_
  
  //
  bool suSatRouter::wire_covers_edge_ (sutype::satindex_t wiresatindex,
                                       sutype::dcoord_t edge,
                                       const suLayer * layer,
                                       sutype::clause_t & clause)
    const
  {
    suWire * wire = get_wire_by_satindex (wiresatindex);

    // it may be a generatorinstance
    if (!wire) return false;
    
    if (wire->layer()->base() != layer->base()) return false;
      
    if (!wire->is (sutype::wt_route)) return false;
      
    if (edge >= wire->edgel() && edge <= wire->edgeh()) {  
      clause.push_back (wiresatindex);
    }
    
    return false;
    
  } // end of suSatRouter::wire_covers_edge_
  
  //
  void suSatRouter::unregister_a_sat_wire (suWire * wire)
  {
    SUASSERT (wire, "");

    sutype::satindex_t satindex = wire->satindex();
    
    if (satindex == sutype::UNDEFINED_SAT_INDEX) return;
    
    sutype::satindex_t abssatindex = abs (satindex);
    
    SUASSERT (abssatindex > 0, "");
    SUASSERT (abssatindex < (sutype::satindex_t)_satindexToPointer.size(), "");
    SUASSERT (_satindexToPointer [abssatindex], "");
    SUASSERT (wire == (suWire*)_satindexToPointer[abssatindex], "satindex=" << satindex);
    
    if (suSatSolverWrapper::instance()->use_clause_manager()) {
      SUASSERT (!suSatSolverWrapper::instance()->satindex_is_used_in_clauses (satindex), "");
      SUASSERT (!suSatSolverWrapper::instance()->satindex_is_used_in_clauses (-satindex), "");      
    }
    
    _satindexToPointer [abssatindex] = 0;
    
    wire->satindex (sutype::UNDEFINED_SAT_INDEX);

    suSatSolverWrapper::instance()->return_sat_index (satindex);
    
  } // end of suSatRouter::unregister_a_sat_wire
  
  //
  void suSatRouter::register_a_sat_wire (suWire * wire)
  {
    SUASSERT (wire, "");
    
    if (wire->satindex() != sutype::UNDEFINED_SAT_INDEX) return;

    SUASSERT (wire_is_legal_slow_check (wire), wire->to_str());
    
    sutype::satindex_t satindex = suSatSolverWrapper::instance()->get_next_sat_index ();
    
    SUASSERT (satindex > 0, "");
    SUASSERT (!suSatSolverWrapper::instance()->is_constant (satindex), "");
    
    wire->satindex (satindex);
        
    if ((sutype::satindex_t)_satindexToPointer.size() < satindex+1) {
      _satindexToPointer.resize (satindex+1, 0);
    }

    SUASSERT (_satindexToPointer [satindex] == 0, "");
    
    _satindexToPointer [satindex] = wire;

    // set extra features
    if (wire_is_obsolete_ (wire)) {
      suWireManager::instance()->mark_wire_as_obsolete (wire);
    }

    // debug
    if (0) {
      if (wire->satindex() == 505) {
        suSatSolverWrapper::instance()->emit_ALWAYS_ONE (wire->satindex());
      }
    }
    //
    
  } // end of suSatRouter::register_a_sat_wire
  
  //
  void suSatRouter::add_connected_entity_ (suConnectedEntity * ce)
  {    
    //SUINFO(1) << "Stored a connected entity: " << ce->to_str() << std::endl;

    _connectedEntities [ce->net()->id()].push_back (ce);

    if (0) {
      const sutype::wires_t & wires = ce->wires();
      for (const auto & iter : wires) {
        const suWire * wire = iter;
        SUINFO(1) << "CE wire: " << wire->to_str() << std::endl;
      }
    }
    
  } // end of suSatRouter::add_connected_entity_

  //
  void suSatRouter::add_tie_ (suTie * tie)
  {
    _ties [tie->net()->id()].push_back (tie);

    //SUASSERT (tie->id() != 1446, tie->to_str());
    
  } // end of suSatRouter::add_tie_

  // all ties belong to one net
  void suSatRouter::delete_unfeasible_net_ties_ (sutype::id_t netid)
  {
    const bool np = false;

    sutype::ties_t & ties = _ties[netid];
    sutype::connectedentities_t & entities = _connectedEntities[netid];

    const suNet * net = suCellManager::instance()->get_net_by_id (netid);
    SUASSERT (net, "");

    SUINFO(1) << "Delete unfeasible routes, ties, and entities for net " << net->name() << std::endl;
    
    if (ties.empty()) {
      SUINFO(1) << "Net " << net->name() << " has no ties." << std::endl;
      return;
    }
    
    sutype::uvi_t numdeletedwires  = 0;
    sutype::uvi_t numdeletedroutes = 0;
    sutype::uvi_t numdeletedties   = 0;
    sutype::uvi_t numdeletedces    = 0;
    
    std::set<suTie*, suTie::cmp_ptr> tiesToDelete;

    //bool maySkipThisStage0 = false;
    //bool maySkipThisStage1 = false;
    //bool maySkipThisStage2 = false;
    bool maySkipThisStage3 = false;
    bool maySkipThisStage4 = false;
    
    while (true) {
      
      bool repeat = false;

      std::map<const suConnectedEntity *, sutype::ties_t, suConnectedEntity::cmp_const_ptr> tiesPerEntity;
      
      // Stage 0: delete unfeasible constant_0 routes; delete ties without routes
      SUINFO(np) << "Stage 0: delete unfeasible constant_0 routes; delete ties without routes" << std::endl;

      for (sutype::svi_t i=0; i < (sutype::svi_t)ties.size(); ++i) {

        suTie * tie = ties[i];
        SUASSERT (tie->net() == net, "");

        if (tiesToDelete.count (tie) > 0) {
          ties[i] = ties.back();
          ties.pop_back();
          --i;
          continue;
        }

        sutype::routes_t & routes = tie->routes();
        
        // delete unfeasible constant_0 routes
        for (sutype::svi_t r=0; r < (sutype::svi_t)routes.size(); ++r) {

          suRoute * route = routes[r];
          SUASSERT (route, "");
          SUASSERT (route->layoutFunc(), "");
          SUASSERT (route->wires().size() >= 2, "");

          //route->release_useless_wires (); // commented; too heavy; zero ROI
          
          sutype::satindex_t routesatindex = route->layoutFunc()->satindex();
          
          if (!suSatSolverWrapper::instance()->is_constant (0, routesatindex)) continue;

          SUINFO(np)
            << "DUNT: "
            << "Found an unfeasible route of a tie " << tie->to_str()
            << std::endl;
          
          ++numdeletedroutes;
          
          delete route;
          routes[r] = routes.back();
          routes.pop_back();
          --r;
        }
        
        if (routes.empty()) {

          SUINFO(np)
            << "DUNT: "
            << "Found a tie without routes: " << tie->to_str()
            << std::endl;
          
          tiesToDelete.insert (tie);
          ties[i] = ties.back();
          ties.pop_back();
          --i;
          continue;
        }
        
        tiesPerEntity [tie->entity0()].push_back (tie);
        tiesPerEntity [tie->entity1()].push_back (tie);
      }

      // Stage 1: check number of ties per entity; delete a tie if this is the only tie that connects this trunk entity and any other entity
      SUINFO(np) << "Stage 1: check number of ties per entity; delete a tie if this is the only tie that connects this trunk entity and any other entity" << std::endl;
      
      for (const auto & iter : tiesPerEntity) {
        
        const suConnectedEntity * entity0 = iter.first;
        SUASSERT (entity0, "");

        sutype::ties_t & entityties = tiesPerEntity [entity0];
        SUASSERT (!entityties.empty(), "");
        
        for (sutype::svi_t i=0; i < (sutype::svi_t)entityties.size(); ++i) {

          suTie * tie = entityties[i];
        
          if (tiesToDelete.count (tie) > 0) {
            entityties[i] = entityties.back();
            entityties.pop_back();
            --i;
            continue;
          }
        }

        std::sort (entityties.begin(), entityties.end(), suStatic::compare_ties_by_id);
        
        if (entity0->is (sutype::wt_preroute) && entityties.empty()) {
          
          SUISSUE("Connected entity has not enough ties") << ": " << entity0->to_str() << ": ties " << entityties.size() << std::endl;
          SUASSERT (false, "");
        }

        if (entity0->is (sutype::wt_trunk) && entityties.size() < 2) {

          SUISSUE("Connected entity has not enough ties") << ": " << entity0->to_str() << ": ties " << entityties.size() << std::endl;

          if (!_option_may_skip_global_routes) {
            SUASSERT (false, "");
          }
          
          for (sutype::uvi_t i=0; i < entityties.size(); ++i) {
            suTie * tie = entityties[i];
            numdeletedroutes += tie->routes().size();
            tie->delete_routes ();
            tiesToDelete.insert (tie);
            repeat = true;
          }

          entityties.clear ();
        }
      }

      if (repeat) continue;

      // Stage 2: check trunk routes; if a trunk wire has only one tie - delete all routes to this wire
      SUINFO(np) << "Stage 2: check trunk routes; if a trunk wire has only one tie - delete all routes to this wire" << std::endl;

      for (const auto & iter1 : tiesPerEntity) {
        
        const suConnectedEntity * entity0 = iter1.first;
        SUASSERT (entity0, "");

        if (!entity0->is (sutype::wt_trunk)) continue;
        
        const sutype::ties_t & entityties = tiesPerEntity [entity0];
        SUASSERT (!entityties.empty(), "");
        
        const sutype::wires_t & wires = entity0->interfaceWires();
        SUASSERT (!wires.empty(), "");
        
        for (const auto & iter2 : wires) {

          suWire * wire = iter2;
          SUASSERT (wire->is (sutype::wt_trunk), "");
          
          sutype::ties_t tiesPerWire;

          for (const auto & iter3 : entityties) {

            suTie * tie = iter3;
            const sutype::routes_t & routes = tie->routes();

            for (const auto & iter4 : routes) {

              suRoute * route = iter4;
              if (route->has_wire (wire)) {
                tiesPerWire.push_back (tie);
                break;
              }
            }

            if (tiesPerWire.size() >= 2) break;
          }

          if (tiesPerWire.size() >= 2) continue;
          
          // delete unfeasible routes
          for (sutype::uvi_t t=0; t < tiesPerWire.size(); ++t) {
            
            suTie * tie = tiesPerWire[t];
            sutype::routes_t & routes = tie->routes();
            
            sutype::uvi_t counter = 0;
            
            for (sutype::uvi_t i=0; i < routes.size(); ++i) {
              
              suRoute * route = routes[i];
              const sutype::wires_t & routewires = route->wires();
              SUASSERT (routewires.size() >= 2, "");
              
              if (wire == routewires[0] || wire == routewires[1]) {
                SUASSERT (route->layoutFunc(), "");
                SUASSERT (route->layoutFunc()->satindex() == sutype::UNDEFINED_SAT_INDEX, "");
                ++numdeletedroutes;
                delete route; // here, we can't release temporary wires created for this route because I store only two terminal wires
                repeat = true;
                continue;
              }
              
              routes [counter] = route;
              ++counter;
            }
            
            SUASSERT (counter < routes.size(), "");
            
            SUINFO(np)
              << "DUNT: "
              << "Wire has no enough ties: wire " << wire->to_str() << "; ties " << tiesPerWire.size() << "; deleted " << (routes.size() - counter) << " unfeasible routes."
              << std::endl;
            
            routes.resize (counter);
          }
        }
      }
      
      if (repeat) continue;

      // Stage 3: reduce a route
      SUINFO(np) << "Stage 3: reduce a route" << std::endl;

      for (const auto & iter : tiesPerEntity) {

        if (maySkipThisStage3) break;

        const suConnectedEntity * entity0 = iter.first;
        SUASSERT (entity0, "");
        if (!entity0->is (sutype::wt_preroute)) continue;
        
        const sutype::ties_t & entityties0 = tiesPerEntity [entity0];
        SUASSERT (!entityties0.empty(), "");

        for (sutype::uvi_t t0 = 0; t0 < entityties0.size(); ++t0) {

          suTie * tie0 = entityties0[t0];
          const suConnectedEntity * entity1 = tie0->get_another_entity (entity0);
          if (!entity1->is (sutype::wt_preroute)) continue;

          
        }
      }

      // Stage 4: reduce a route between a preroute A and a trunk B if this preroute A has another route with a lower trunk C
      SUINFO(np) << "Stage 4: reduce a route between a preroute A and a trunk B if this preroute A has another route with a lower trunk C" << std::endl;

      for (const auto & iter : tiesPerEntity) {

        if (maySkipThisStage4) break;
        
        const suConnectedEntity * prerouteentityA = iter.first; // entity "A"
        SUASSERT (prerouteentityA, "");
        //if (!prerouteentityA->is (sutype::wt_preroute)) continue;

        const sutype::wires_t & interfaceWiresA = prerouteentityA->interfaceWires();
        
        const sutype::ties_t & entitytiesA = tiesPerEntity [prerouteentityA];
        SUASSERT (!entitytiesA.empty(), "");
                
        for (sutype::uvi_t t0 = 0; t0 < entitytiesA.size(); ++t0) {
          
          suTie * trunktieAB = entitytiesA[t0];
          const suConnectedEntity * trunkentityB = trunktieAB->get_another_entity (prerouteentityA); // entity "B"
          SUASSERT (trunkentityB, "");
          if (!trunkentityB->is (sutype::wt_trunk)) continue;

          const sutype::wires_t & interfaceWiresB = trunkentityB->interfaceWires();
          const sutype::routes_t & routesAB = trunktieAB->routes();
          
          for (sutype::uvi_t t1 = 0; t1 < entitytiesA.size(); ++t1) {
            
            if (t1 == t0) continue;
            
            suTie * trunktieAC = entitytiesA[t1];
            SUASSERT (trunktieAC != trunktieAB, "");
            const suConnectedEntity * trunkentityC = trunktieAC->get_another_entity (prerouteentityA); // entity "C"
            SUASSERT (trunkentityC, "");
            if (!trunkentityC->is (sutype::wt_trunk)) continue;
            
            const sutype::wires_t & interfaceWiresC = trunkentityC->interfaceWires();
            sutype::routes_t & routesAC = trunktieAC->routes();
            
            // 2nd check:
            //   a routeAC of trunktieAC is partially redundant if it has a non-trunk wire "c" that touches a trunk wire "b" of a trunktieAB
            //   in this case, I release this wire "c" from this routeAC

            for (sutype::uvi_t iwa=0; iwa < interfaceWiresA.size(); ++iwa) {

              suWire * interfaceWireA = interfaceWiresA[iwa];
              //SUASSERT (trunkentitywireB->is (sutype::wt_preroute), "");

              for (sutype::uvi_t iwb=0; iwb < interfaceWiresB.size(); ++iwb) {

                suWire * interfaceWireB = interfaceWiresB [iwb];
                SUASSERT (interfaceWireB->is (sutype::wt_trunk), "");

                for (sutype::uvi_t iwc=0; iwc < interfaceWiresC.size(); ++iwc) {

                  suWire * interfaceWireC = interfaceWiresC [iwc];
                  SUASSERT (interfaceWireC->is (sutype::wt_trunk), "");
                  
                  if ((interfaceWireB->layer()->level() > interfaceWireA->layer()->level() && interfaceWireC->layer()->level() > interfaceWireB->layer()->level()) ||
                      (interfaceWireB->layer()->level() < interfaceWireA->layer()->level() && interfaceWireC->layer()->level() < interfaceWireB->layer()->level())) {
                  }
                  else {
                    continue;
                  }

                  bool foundRouteAB = false; // can trim wires of routeAC if there're no feasible routesAB

                  // routes of trunktieAB
                  for (sutype::uvi_t r0=0; r0 < routesAB.size(); ++r0) {
                    
                    suRoute * routeAB = routesAB[r0];
                    const sutype::wires_t & wiresAB = routeAB->wires();
                    SUASSERT (wiresAB.size() >= 2, "");
                    
                    // routeAB and routeAC must connect the same wireA and (trunkentityB and trunkentityC correspondendly)
                    if ((wiresAB[0] == interfaceWireA && wiresAB[1] == interfaceWireB) ||
                        (wiresAB[1] == interfaceWireA && wiresAB[0] == interfaceWireB)) {

                      foundRouteAB = true;
                      break;
                    }
                  }

                  if (!foundRouteAB) continue;
                    
                  // trim wires of routeAC
                  for (sutype::svi_t r1=0; r1 < (sutype::svi_t)routesAC.size(); ++r1) {
                      
                    suRoute * routeAC = routesAC[r1];
                    const sutype::wires_t & wiresAC = routeAC->wires(); // wires will be trimmed
                    SUASSERT (wiresAC.size() >= 2, "");
                      
                    if ((wiresAC[0] == interfaceWireA && wiresAC[1] == interfaceWireC) ||
                        (wiresAC[1] == interfaceWireA && wiresAC[0] == interfaceWireC)) {
                    }
                    else {
                      continue;
                    }
                      
                    bool routemodified1 = false;
                    sutype::uvi_t numremovedwires = 0;
                    
                    // itermediate wires of routeAC
                    for (sutype::svi_t w1=2; w1 < (sutype::svi_t)wiresAC.size(); ++w1) {
                      
                      suWire * wire1 = wiresAC[w1]; // intermediate route wire
                      SUASSERT (wire1->is (sutype::wt_route) || wire1->is (sutype::wt_shunt), wire1->to_str());
                      
                      if (wire1->layer() != interfaceWireB->layer()) continue;
                      if (!wire1->rect().has_at_least_common_point (interfaceWireB->rect())) continue;
                    
                      SUINFO(np) 
                        << "DUNT: "
                        << "Wire " << wire1->to_str()
                        << " is redundant for tie " << trunktieAC->to_str()
                        << " because of wire " << interfaceWireB->to_str()
                        << " of tie " << trunktieAB->to_str()
                        << std::endl;
                        
                      sutype::uvi_t dbgnumwiresBef = wiresAC.size();                  
                      bool ret = routeAC->remove_wire_and_update_layout_function (wire1, w1);
                        
                      //wiresAC[w1] = wiresAC.back();
                      //wiresAC.pop_back();
                      //--w1;

                      //bool ret = routeAC->layoutFunc()->replace_satindex (wire1->satindex(), suSatSolverWrapper::instance()->get_constant(0));
                      //suWireManager::instance()->release_wire (wire1);
                        
                      sutype::uvi_t dbgnumwiresAft = wiresAC.size();
                      SUASSERT (dbgnumwiresAft + 1 == dbgnumwiresBef, "");
                      --w1;
                    
                      ++numremovedwires;
                      ++numdeletedwires;
                  
                      if (ret) {
                        routemodified1 = true;
                      }

                      break; // removed wire1; nothing to do more here
                                            
                    } // for(w1/wiresAC)

                      
                    if (!routemodified1) continue;
                      
                    routeAC->layoutFunc()->simplify();
                    repeat = true;
                      
                    // report only
                    bool routedeleted = false;
                    bool morewiresreleased = false;
                      
                    sutype::satindex_t routesatindex = routeAC->layoutFunc()->satindex();
                      
                    if (suSatSolverWrapper::instance()->is_constant (0, routesatindex)) {
                      ++numdeletedroutes;
                      delete routeAC;
                      routesAC[r1] = routesAC.back();
                      routesAC.pop_back();
                      --r1;
                      routedeleted = true;
                    }
                    else {
                      //morewiresreleased = routeAC->release_useless_wires (); // it's heavy and I saw no effect on test cases
                    }
                    
                    SUINFO(np) 
                      << "DUNT: "
                      << "Removed " << numremovedwires << " wires for a route"
                      << "; routedeleted=" << routedeleted
                      << "; routemodified=" << routemodified1
                      << "; morewiresreleased=" << morewiresreleased
                      << std::endl;
              
                  } // for(r1/routesAC)
                  
                } // for(iwb/interfaceWiresC)
              } // for(iwb/interfaceWiresB)
            } // for(iwa/interfaceWiresA)
          } // for(t1/entitytiesA)
        } // for(t0/entitytiesA)
      } // for(iter/tiesPerEntity)

      maySkipThisStage4 = true;
      
      if (repeat) continue;

      break;
    }
    
    // delete ties
    
    sutype::uvi_t counter = 0;
    
    for (sutype::uvi_t i=0; i < ties.size(); ++i) {

      suTie * tie = ties[i];
      
      if (tiesToDelete.count (tie) > 0) {
        SUASSERT (tie->routes().empty(), "");
        continue;
      }

      SUASSERT (!tie->routes().empty(), "");

      ties [counter] = tie;
      ++counter;
    }

    //SUASSERT (tiesToDelete.size() + counter == ties.size(), "tiesToDelete=" << tiesToDelete.size() << "; counter=" << counter << "; ties=" << ties.size());
    
    if (ties.size() != counter) {
      ties.resize (counter);
    }
    SUASSERT (ties.size() == counter, "");
    
    for (const auto & iter : tiesToDelete) {
      suTie * tie = iter;
      SUASSERT (tie->routes().empty(), "");
      SUINFO(1) << "Deleted a prunned tie: " << tie->to_str() << std::endl;
      ++numdeletedties;
      delete tie;
    }
    
    std::sort (ties.begin(), ties.end(), suStatic::compare_ties_by_id);

    // delete unfeasible entityties
    if (1) {
      
      std::map<const suConnectedEntity *, sutype::ties_t, suConnectedEntity::cmp_const_ptr> tiesPerEntity;

      for (const auto & iter : ties) {

        suTie * tie = iter;
        tiesPerEntity [tie->entity0()].push_back (tie);
        tiesPerEntity [tie->entity1()].push_back (tie);
      }

      sutype::uvi_t counter = 0;
      
      // delete entities
      for (sutype::uvi_t i=0; i < entities.size(); ++i) {
        
        suConnectedEntity * entity = entities[i];

        if (entity->is (sutype::wt_trunk)) {

          // entity has no ties at all
          if (tiesPerEntity.count (entity) == 0) {

            SUINFO(1) << "Deleted an unfeasible entity: " << entity->to_str() << std::endl;
            ++numdeletedces;
            delete entity;
            continue;
          }
          
          SUASSERT (tiesPerEntity.at(entity).size() >= 2, "");
        }
        
        entities [counter] = entity;
        ++counter;
      }
      
      if (counter != entities.size()) {
        entities.resize (counter);
      }
    }

    SUINFO(1)
      << "Deleted"
      << ": " << numdeletedwires << " wires"
      << "; " << numdeletedroutes << " routes"
      << "; " << numdeletedties << " ties"
      << "; " << numdeletedces << " ces"
      << "."
      << std::endl;
    
  } // end of suSatRouter::delete_unfeasible_net_ties_
  
  //
  sutype::dcoord_t suSatRouter::calculate_distance_ (const sutype::wires_t & wires1,
                                                     const sutype::wires_t & wires2,
                                                     sutype::grid_orientation_t gd)
    const
  {
    SUASSERT (!wires1.empty(), "");
    SUASSERT (!wires2.empty(), "");
    
    sutype::dcoord_t dist = -1;

    for (const auto & iter1 : wires1) {

      const suWire * wire1 = iter1;

      for (const auto & iter2 : wires2) {

        const suWire * wire2 = iter2;

        sutype::dcoord_t d = wire1->rect().distance_to (wire2->rect(), gd);
        SUASSERT (d >= 0, "");
        dist = (dist < 0) ? d : std::min (d, dist);
      }
    }

    return dist;
    
  } // end of suSatRouter::calculate_distance_

  //
  void suSatRouter::create_routes_ (suTie * tie)
  {
    const bool np = false;

    SUINFO(1) << "Create routes for tie: " << tie->to_str() << std::endl;
    
    SUASSERT (tie->entity0(), "");
    SUASSERT (tie->entity1(), "");
    SUASSERT (tie->routes().empty(), "");

    const sutype::wires_t & wires0 = tie->entity0()->interfaceWires();
    const sutype::wires_t & wires1 = tie->entity1()->interfaceWires();

    if (np) {

      SUINFO(1) << "wires0:" << std::endl;
      for (const auto & iter : wires0) {
        suWire * wire = iter;
        if (!wire->is (sutype::wt_preroute)) continue;
        SUINFO(1) << "  " << wire->to_str() << std::endl;
      }

      SUINFO(1) << "wires1:" << std::endl;
      for (const auto & iter : wires1) {
        suWire * wire = iter;
        if (!wire->is (sutype::wt_preroute)) continue;
        SUINFO(1) << "  " << wire->to_str() << std::endl;
      }
    }

    SUASSERT (!wires0.empty(), "");
    SUASSERT (!wires1.empty(), "");

    const suRegion * region = suGlobalRouter::instance()->get_region (0, 0);
    SUASSERT (region, "");

    sutype::dcoord_t maxdist = 2 * (region->bbox().h() + region->bbox().w());
    
    std::vector<sutype::dcoords_t> dists;
    
    for (sutype::uvi_t i=0; i < wires0.size(); ++i) {
      
      suWire * wire0 = wires0[i];

      for (sutype::uvi_t k=0; k < wires1.size(); ++k) {

        suWire * wire1 = wires1[k];

        sutype::dcoord_t distver = wire0->rect().distance_to (wire1->rect(), sutype::go_ver);
        sutype::dcoord_t disthor = wire0->rect().distance_to (wire1->rect(), sutype::go_hor);
        
        sutype::dcoords_t a;
        a.push_back (distver + disthor); // value to compare
        a.push_back (i);
        a.push_back (k);
        
        dists.push_back (a);
      }
    }

    std::sort (dists.begin(), dists.end(), suStatic::compare_dcoords);

    bool createdAtLeastOneRoute = false;

    for (const auto & iter1 : dists) {
      
      SUASSERT (iter1.size() == 3, "");

      sutype::uvi_t j = 0;
      sutype::dcoord_t dist = iter1[j]; ++j;
      sutype::dcoord_t i    = iter1[j]; ++j;
      sutype::dcoord_t k    = iter1[j]; ++j;
      
      SUASSERT (i >= 0 && i < (sutype::dcoord_t)wires0.size(), "");
      SUASSERT (k >= 0 && k < (sutype::dcoord_t)wires1.size(), "");
      
      suWire * wire0 = wires0[i];
      suWire * wire1 = wires1[k];
      
      if (createdAtLeastOneRoute && dist > maxdist) continue;
      
      suRoute * route = create_route_between_two_wires_ (wire0,
                                                         wire1,
                                                         false);
      if (!route) continue;

      createdAtLeastOneRoute = true;
      
      tie->add_route (route);
    }
    
    if (tie->routes().empty()) {
      SUISSUE("Found no routes for a tie") << ": tie=" << tie->to_str() << std::endl;
    }
    
  } // end of create_routes_

  //
  void suSatRouter::calculate_net_groups_ (const sutype::tokens_t & cetokens)
  {
    SUINFO(1) << "Calculate net groups." << std::endl;

    const bool useOnlyUpperLayerInterfaceWires = false;
    
    SUASSERT (_wireToGroupIndex.empty(), "");
    
    const bool option_allow_opens = suOptionManager::instance()->get_boolean_option ("allow_opens", false);    
    const sutype::nets_tc & netsToRoute = suCellManager::instance()->netsToRoute();
    
    std::map<const suNet *, std::vector<sutype::wires_t>, suNet::cmp_const_ptr > cesPerNet;
    
    // first stage
    for (sutype::uvi_t i=0; i < cetokens.size(); ++i) {
      
      const suToken * token = cetokens[i];
      SUASSERT (token->is_defined ("terms"), "Connected Entity has no terminals");
      
      sutype::wires_t cewires = suWireManager::instance()->get_wires_by_permanent_gid (token->get_list_of_integers ("terms"), "Connected entity");
      if (cewires.empty()) {
        SUISSUE("Connected entity has no wires. Skipped.") << std::endl;
        continue;
      }

      const suNet * cenet = 0;

      for (const auto & iter : cewires) {
        
        suWire * wire = iter;
        SUASSERT (wire->is (sutype::wt_preroute), "");

        const suNet * wirenet = wire->net();
        
        if (cenet == 0) cenet = wirenet;
        SUASSERT (cenet, "");
        SUASSERT (cenet == wirenet, "Wires of a connected entity belong to different nets: " << cenet->name() << " and " << wirenet->name());
      }
      
      SUASSERT (cenet, "");
      SUASSERT (std::find (netsToRoute.begin(), netsToRoute.end(), cenet) != netsToRoute.end(), "cetokens should be trimmed already");

      cesPerNet[cenet].push_back (cewires);
    }
    
    // second stage
    for (const auto & iter1 : cesPerNet) {
      
      const suNet * net = iter1.first;
      SUASSERT (net, "");
      SUASSERT (_wireToGroupIndex.count (net->id()) == 0, "");
      
      // calculate actual connected entities
      std::vector<sutype::wires_t> groupsOfWires = calculate_connected_entities_ (net);
      
      std::vector<sutype::uvi_t> groupIndex;
      std::map<suWire *, sutype::uvi_t> & wireToGroupIndex = _wireToGroupIndex[net->id()];
      
      for (sutype::uvi_t i=0; i < groupsOfWires.size(); ++i) {
        
        const sutype::wires_t & cewires = groupsOfWires[i];
        groupIndex.push_back (i);
        SUASSERT (i+1 == groupIndex.size(), "");
        SUASSERT (groupIndex[i] == i, "");
        sutype::uvi_t groupindex = i;

        for (const auto & iter2 : cewires) {
          suWire * wire = iter2;
          wireToGroupIndex[wire] = groupindex;
        }
      }

      const std::vector<sutype::wires_t> & netcewires = iter1.second;

      //SUINFO(1) << "Net " << net->name() << " has " << netcewires.size() << " externally defined connected entities." << std::endl;
      
      std::set<suWire *, suWire::cmp_ptr> allinterfacewires;

      for (const auto & iter2 : netcewires) {

        const sutype::wires_t & cewires = iter2;
      
        sutype::svi_t commonGroupIndex = -1;
        
        for (sutype::uvi_t k=0; k < cewires.size(); ++k) {
          
          suWire * wire = cewires[k];

          SUASSERT (wire->is (sutype::wt_preroute), "");        
          SUASSERT (wireToGroupIndex.count (wire) == 1, wireToGroupIndex.count(wire) << "; wire=" << wire->to_str());

          SUASSERT (allinterfacewires.count (wire) == 0, "");
          allinterfacewires.insert (wire);
          
          sutype::svi_t wireGroupIndex = wireToGroupIndex[wire];
          
          if (commonGroupIndex < 0) {
            commonGroupIndex = wireGroupIndex;
          }

          if (wireGroupIndex == commonGroupIndex) continue;

          if (wireGroupIndex < commonGroupIndex) {
            sutype::svi_t tmpindex = commonGroupIndex;
            commonGroupIndex = wireGroupIndex;
            wireGroupIndex = tmpindex;
          }

          // merge groups
          sutype::wires_t & wires0 = groupsOfWires[commonGroupIndex];
          sutype::wires_t & wires1 = groupsOfWires[wireGroupIndex];

          SUASSERT (!wires0.empty(), "");
          SUASSERT (!wires1.empty(), "");
          
          wires0.insert (wires0.end(), wires1.begin(), wires1.end());

          for (const auto & iter2 : wires1) {
            suWire * wire2 = iter2;
            wireToGroupIndex[wire2] = commonGroupIndex;
          }
          wires1.clear ();

          for (const auto & iter2 : wires0) {
            suWire * wire2 = iter2;
            SUASSERT ((sutype::svi_t)wireToGroupIndex[wire2] == commonGroupIndex, "");
          }
        }
      }
      
      sutype::uvi_t numActualCEs = 0;
      
      for (sutype::uvi_t i=0; i < groupsOfWires.size(); ++i) {
        const sutype::wires_t & cewires = groupsOfWires[i];
        if (cewires.empty()) continue;
        ++numActualCEs;
      }

      //SUINFO(1) << "Net " << net->name() << " actually has " << numActualCEs << " connected entities." << std::endl;

      std::map<sutype::uvi_t,sutype::wires_t> interfaceWiresPerCE;

      for (const auto & iter2 : allinterfacewires) {

        suWire * wire = iter2;
        
        SUASSERT (wire->is (sutype::wt_preroute), "");
        SUASSERT (wireToGroupIndex.count (wire) > 0, "");

        sutype::uvi_t groupIndex = wireToGroupIndex.at (wire);
        
        interfaceWiresPerCE [groupIndex].push_back (wire);
      }

      SUASSERT (_connectedEntities[net->id()].empty(), "");
      
      for (const auto & iter2 : interfaceWiresPerCE) {
        
        const sutype::wires_t & cewires = iter2.second;
        SUASSERT (!cewires.empty(), "");

        suConnectedEntity * connectedentity = new suConnectedEntity (net);
        
        connectedentity->add_type (sutype::wt_preroute);
                
        const sutype::satindex_t openSatindex = option_allow_opens ? suSatSolverWrapper::instance()->get_next_sat_index() : suSatSolverWrapper::instance()->get_constant(0);
        connectedentity->openSatindex (openSatindex);
      
        suLayoutFunc * layoutfuncCE = connectedentity->layoutFunc();
        SUASSERT (layoutfuncCE, "");
        SUASSERT (layoutfuncCE->nodes().empty(), "");
        layoutfuncCE->func (sutype::logic_func_or);
        layoutfuncCE->sign (sutype::unary_sign_just);
        
        // to allow CE to be open (i.e. unfeasible by any routes)
        layoutfuncCE->add_leaf (connectedentity->openSatindex());
        suLayoutFunc * layoutfunc0 = layoutfuncCE->add_func (sutype::logic_func_and, sutype::unary_sign_just, SUFILELINE_DLF);

        const suLayer * ceinterfacelayer = 0;
        
        for (const auto & iter3 : cewires) {
          
          suWire * wire = iter3;
          const suLayer * layer = wire->layer();
          SUASSERT (layer->is (sutype::lt_wire), "");
          SUASSERT (layer->is (sutype::lt_metal), "");

          if (ceinterfacelayer == 0 || layer->level() > ceinterfacelayer->level()) {
            ceinterfacelayer = layer;
            if (useOnlyUpperLayerInterfaceWires) {
              connectedentity->clear_interface_wires ();
            }
          }
          
          SUASSERT (wire->satindex() != sutype::UNDEFINED_SAT_INDEX, wire->to_str());
          connectedentity->add_wire (wire);
          
          SUASSERT (ceinterfacelayer, "");
          if (!useOnlyUpperLayerInterfaceWires || wire->layer()->level() == ceinterfacelayer->level()) {
            connectedentity->add_interface_wire (wire);
          }
          
          layoutfunc0->add_leaf (wire->satindex());
        }

        SUASSERT (ceinterfacelayer, "");
        
        connectedentity->set_interface_layer (ceinterfacelayer);
        
        add_connected_entity_ (connectedentity);
      }
      
      SUINFO(1)
        << "Connected entities for net " << net->name()
        << "; externally defined: " << netcewires.size()
        << "; actual number: " << numActualCEs
        << "; created " << _connectedEntities[net->id()].size()
        << std::endl;
      
    } // for(iter1/cesPerNet)
    
  } // end of suSatRouter::calculate_net_groups_
  
  // \return created route
  suRoute * suSatRouter::create_route_between_two_wires_ (suWire * wire1,
                                                          suWire * wire2,
                                                          bool usePreroutesOnly)
  {
    SUASSERT (wire1, "");
    SUASSERT (wire2, "");
    SUASSERT (wire1->satindex() != sutype::UNDEFINED_SAT_INDEX, "");
    SUASSERT (wire2->satindex() != sutype::UNDEFINED_SAT_INDEX, "");
    SUASSERT (wire1->net() == wire2->net(), "");
    SUASSERT (wire1->net(), "");

    const sutype::svi_t wireusage1 = suWireManager::instance()->get_wire_usage (wire1);
    const sutype::svi_t wireusage2 = suWireManager::instance()->get_wire_usage (wire2);
    
    SUASSERT (wireusage1 > 0, "");
    SUASSERT (wireusage2 > 0, "");

    const suNet * net = wire1->net();
    SUASSERT (net, "");
    SUASSERT (_wireToGroupIndex.count (net->id()) > 0, net->name() << "; wire1=" << wire1->to_str() << "; there are no groups for this net");
    const auto & wireToGroupIndex = _wireToGroupIndex.at (net->id());
    SUASSERT (!wireToGroupIndex.empty(), "");
    
    // this suRouteGenerator acts as a manager of data
    suRouteGenerator rg (net, wire1, wire2);
    
    suRoute * route = 0;

    // a route connects two wires using several possible options
    if (usePreroutesOnly) route = rg.connect_wires_by_preroutes (wireToGroupIndex);
    else                  route = rg.create_route               (wireToGroupIndex);
    
    if (!route) {
      
      SUASSERT (wireusage1 == suWireManager::instance()->get_wire_usage (wire1), "");
      SUASSERT (wireusage2 == suWireManager::instance()->get_wire_usage (wire2), "");
      
      if (!usePreroutesOnly) {
        
//         SUISSUE("Found no routes between two wires")
//           << ": " << wire1->id() << "-" << wire2->id()
//           << "; wire1=" << wire1->to_str()
//           << "; wire2=" << wire2->to_str()
//           << std::endl;
        
        if (0) {
          
          sutype::dcoord_t errxl = std::min (wire1->rect().xl(), wire2->rect().xl());
          sutype::dcoord_t erryl = std::min (wire1->rect().yl(), wire2->rect().yl());
          sutype::dcoord_t errxh = std::max (wire1->rect().xh(), wire2->rect().xh());
          sutype::dcoord_t erryh = std::max (wire1->rect().yh(), wire2->rect().yh());
          
          suErrorManager::instance()->add_error (std::string ("no_routes") + "_" + suStatic::dcoord_to_str(wire1->id()) + "_" + suStatic::dcoord_to_str(wire2->id()),
                                                 errxl,
                                                 erryl,
                                                 errxh,
                                                 erryh);
        }
      }
      
      return 0;
    }
    
    SUASSERT (route->layoutFunc()->is_valid(), "");
        
    if ((wireusage1 + 1) != suWireManager::instance()->get_wire_usage (wire1)) {
      suSatRouter::print_layout_node (std::cout, route->layoutFunc(), "");
      SUASSERT (false, "old=" << wireusage1 << "; new=" << suWireManager::instance()->get_wire_usage (wire1));
    }
        
    if ((wireusage2 + 1) != suWireManager::instance()->get_wire_usage (wire2)) {
      suSatRouter::print_layout_node (std::cout, route->layoutFunc(), "");
      SUASSERT (false, "old=" << wireusage2 << "; new=" << suWireManager::instance()->get_wire_usage (wire2));
    }

    // debug
    if (0) {
      
      if (rg.id () == 46) {
      
        SUASSERT (route, "");
        SUASSERT (route->layoutFunc(), "");
        sutype::satindex_t routesatindex = route->layoutFunc()->satindex();
        SUASSERT (routesatindex == sutype::UNDEFINED_SAT_INDEX, "");
        routesatindex = route->layoutFunc()->calculate_satindex();
        SUASSERT (routesatindex != sutype::UNDEFINED_SAT_INDEX, "");
        suSatSolverWrapper::instance()->emit_ALWAYS_ONE (routesatindex);

        bool ok = suSatSolverWrapper::instance()->solve_the_problem ();
        SUASSERT (ok, "");
        eliminate_antennas ();
        suSatRouter::instance()->apply_solution ();
        SUASSERT (false, "");
      }

      else {

        delete route;
        return 0;
      }
    }
    //

    return route;
    
  } // end of suSatRouter::create_route_between_two_wires_
    
  //
  void suSatRouter::emit_net_ties_ (const sutype::connectedentities_t & inputConnectedEntities,
                                    const sutype::ties_t & ties)
    const
  {
    const bool option_allow_opens = suOptionManager::instance()->get_boolean_option ("allow_opens", false);
    
    SUASSERT (!inputConnectedEntities.empty(), "");
    
    sutype::connectedentities_t  connectedEntities;
    std::vector <sutype::ties_t> connectedEntityTies;
    
    connectedEntities  .resize (inputConnectedEntities.size(), 0);
    connectedEntityTies.resize (inputConnectedEntities.size());
    
    for (sutype::uvi_t i=0; i < inputConnectedEntities.size(); ++i) {
      
      suConnectedEntity * entity = inputConnectedEntities[i];
      connectedEntities[i] = entity;
      
      for (const auto & iter : ties) {
        suTie* tie = iter;
        if (tie->entity0() == entity || tie->entity1() == entity)
          connectedEntityTies[i].push_back (tie);
      }
    }

    sutype::uvi_t counter = 0;

    // skip unfeasible entities
    for (sutype::uvi_t i=0; i < connectedEntities.size(); ++i) {

      suConnectedEntity * ce = connectedEntities[i];
      SUASSERT (ce, "");

      const sutype::ties_t & ties = connectedEntityTies[i];

      if (ce->is (sutype::wt_preroute)) {

        if (ties.empty()) {

          if (option_allow_opens) {
            SUISSUE("Skipped an unfeasible connected entity") << ": " << ce->to_str() << std::endl;
            continue;
          }
          else {
            SUASSERT (false, "Found unfeasible entity: " << ce->to_str());
          }
        }
      }
      
      else if (ce->is (sutype::wt_trunk)) {
        
        if (ties.size() < 2) {
          
          if (_option_may_skip_global_routes) {
            SUISSUE("Skipped a useless connected entity") << ": " << ce->to_str() << std::endl;
            continue;
          }
          else if (option_allow_opens) {
            SUISSUE("Skipped an unfeasible connected entity") << ": " << ce->to_str() << std::endl;
            continue;
          }
          else {
            SUASSERT (false, "Found unfeasible entity: " << ce->to_str());
          }
        }
      }
      
      else {
        SUASSERT (false, "");
      }

      if (i != counter) {
        connectedEntities   [counter] = ce;
        connectedEntityTies [counter] = ties;
      }
      ++counter;
    }

    if (counter != connectedEntities.size()) {
      connectedEntities  .resize (counter);
      connectedEntityTies.resize (counter);
    }

    //SUINFO(1) << "Found " << connectedEntities.size() << " connected entities." << std::endl;
    
    // all connected entities must be connected
    sutype::clauses_t initmatrix; // connectivity matrix
    const int N = connectedEntities.size();

    sutype::clause_t tmpclause (N, 0);
    initmatrix.resize (N, tmpclause);
    
    for (sutype::uvi_t i=0; i < connectedEntities.size(); ++i) {
      
      const sutype::ties_t & ties1 = connectedEntityTies[i];
      const suConnectedEntity * ce = connectedEntities  [i];
      sutype::satindex_t cesatindex = ce->layoutFunc()->satindex();

      sutype::satindex_t orOfTies = emit_or_ties_ (ties1);
      suSatSolverWrapper::instance()->emit_AND_ALWAYS_ZERO (orOfTies, -cesatindex);
      
      if (ce->is (sutype::wt_preroute)) {
          
        SUASSERT (ties1.size() >= 1, "");
        
        initmatrix[i][i] = orOfTies;
           
      } // end of "ce is preroute"
        
      else if (ce->is (sutype::wt_trunk)) {

        SUASSERT (ties1.size() >= 2, "");
           
        sutype::clause_t & tieclause = get_sat_indices_ (ties1);
        sutype::satindex_t out = suSatSolverWrapper::instance()->emit_GREATER_or_EQUAL_THEN (tieclause, 2);
        suClauseBank::return_clause (tieclause);
            
        // ce must be absent or must have at least two ties
        if (_option_may_skip_global_routes) {
          sutype::clause_t & clause = suClauseBank::loan_clause ();
          clause.push_back (-cesatindex);
          clause.push_back (out);
          initmatrix[i][i] = suSatSolverWrapper::instance()->emit_OR_or_return_constant (clause);
          suClauseBank::return_clause (clause);
        }

        // ce must have at least two ties
        else {
          initmatrix[i][i] = out;
        }
            
      } // end of "ce is trunk"
        
      else {
        SUASSERT (false, "");
      }
        
      //suSatSolverWrapper::instance()->emit_ALWAYS_ONE (initmatrix[i][i]);
      
      for (unsigned k=i+1; k < connectedEntities.size(); ++k) {

        const sutype::ties_t & ties2 = connectedEntityTies[k];
        
        sutype::ties_t commonties = get_common_ties_ (ties1, ties2);
        
        initmatrix[i][k] = initmatrix[k][i] = commonties.empty() ? suSatSolverWrapper::instance()->get_constant(0) : emit_or_ties_ (commonties);
      }
    }

    for (int k=0; k < N; ++k) {
      for (int i=0; i < N; ++i) {
        SUASSERT (initmatrix[i][k] != sutype::UNDEFINED_SAT_INDEX, "i=" << i << "; k=" << k);
      }
    }
        
    // http://ru.wikipedia.org/wiki/%D0%90%D0%BB%D0%B3%D0%BE%D1%80%D0%B8%D1%82%D0%BC_%D0%A3%D0%BE%D1%80%D1%88%D0%B5%D0%BB%D0%BB%D0%B0
    if (1) {
      
      sutype::clauses_t & summatrix = initmatrix;
      
      for (int k=0; k < N; ++k) {
        for (int i=0; i < N; ++i) {
          for (int j=0; j < N; ++j) {

            sutype::clause_t & andclause = suClauseBank::loan_clause ();
            andclause.push_back (summatrix[i][k]);
            
            if (summatrix[k][j] != summatrix[i][k])
              andclause.push_back (summatrix[k][j]);

            sutype::satindex_t out = suSatSolverWrapper::instance()->emit_AND_or_return_constant (andclause);
            suClauseBank::return_clause (andclause);
            
            sutype::clause_t & orclause = suClauseBank::loan_clause ();
            orclause.push_back (summatrix[i][j]);
            
            if (out != summatrix[i][j])
              orclause.push_back (out);
            
            sutype::satindex_t out2 = suSatSolverWrapper::instance()->emit_OR_or_return_constant (orclause);
            suClauseBank::return_clause (orclause);

            summatrix[i][j] = out2;
          }
        }
      }

      for (int i=0; i < N; ++i) {

        suConnectedEntity * ce1 = connectedEntities[i];
        bool maySkipCE1 = (_option_may_skip_global_routes && ce1->is (sutype::wt_trunk));

        for (int k=0; k < N; ++k) {

          suConnectedEntity * ce2 = connectedEntities[k];
          bool maySkipCE2 = (_option_may_skip_global_routes && ce2->is (sutype::wt_trunk));

          if (maySkipCE1 || maySkipCE2) continue;

          sutype::satindex_t satindex = summatrix[i][k];

          if (suSatSolverWrapper::instance()->is_constant (0, satindex)) {
            
            SUISSUE("No path found between two CEs") << ": ce1=" << ce1->to_str() << " and ce2=" << ce2->to_str() << std::endl;
            
            if (option_allow_opens) {
              continue;
            }
            else {
              SUASSERT (false, "");
            }
          }
          
          suSatSolverWrapper::instance()->emit_ALWAYS_ONE (satindex);
        }
      }
    }
    
  } // end of suSatRouter::emit_net_ties_

  //
  sutype::clause_t & suSatRouter::get_sat_indices_ (const sutype::ties_t & ties)
    const
  {
    SUASSERT (!ties.empty(), "");

    sutype::clause_t & clause = suClauseBank::loan_clause ();

    for (const auto & iter : ties) {

      const suTie * tie = iter;
      clause.push_back (tie->satindex());
    }

    return clause;
    
  } // end of suSatRouter::get_sat_indices_

  //
  sutype::satindex_t suSatRouter::emit_or_ties_ (const sutype::ties_t & ties)
    const
  {
    SUASSERT (!ties.empty(), "");

    sutype::clause_t & clause = get_sat_indices_ (ties);

    sutype::satindex_t satindex = suSatSolverWrapper::instance()->emit_OR_or_return_constant (clause);
    
    suClauseBank::return_clause (clause);
    
    return satindex;
    
  } // end of suSatRouter:::emit_or_ties_
  
  //
  sutype::ties_t suSatRouter::get_common_ties_ (const sutype::ties_t & ties1,
                                                const sutype::ties_t & ties2)
    const
  {
    sutype::ties_t commonties;

    for (const auto & iter : ties1) {

      suTie * tie = iter;
      if (std::find (ties2.begin(), ties2.end(), tie) != ties2.end())
        commonties.push_back (tie);
    }

    return commonties;
    
  } // end of suSatRouter::get_common_ties_

  // it didn't help in my trial runs
  void suSatRouter::prepare_antennas_ (const sutype::clause_t & satindicesToCheck,
                                       sutype::clause_t & satindicesToCheckMostLikelyOptional,
                                       sutype::clause_t & satindicesToCheckMostLikelyMandatory,
                                       std::map<const suNet *, std::map <const suLayer *, sutype::wires_t, suLayer::cmp_const_ptr>, suNet::cmp_const_ptr> & fixedWiresPerLayerPerNet)
    const
  {
    for (const auto & iter1 : satindicesToCheck) {

      sutype::satindex_t satindex = iter1;
      
      suWire * wire = get_wire_by_satindex (satindex);
      if (!wire || suSatSolverWrapper::instance()->is_constant (satindex)) continue; // nothing to do

      const suNet * net = wire->net();
      const suLayer * layer = wire->layer();

      if (fixedWiresPerLayerPerNet.count(net) == 0 ||
          fixedWiresPerLayerPerNet.at(net).count(layer) == 0) {

        satindicesToCheckMostLikelyOptional.push_back (satindex);
        continue;
      }

      const sutype::wires_t & fixedwires = fixedWiresPerLayerPerNet.at(net).at(layer);

      bool wireServed = false;

      for (const auto & iter2 : fixedwires) {
        
        suWire * fixedwire = iter2;
        
        SUASSERT (fixedwire, "");
        SUASSERT (suSatSolverWrapper::instance()->is_constant (1, fixedwire->satindex()), "");
        
        if (
            (fixedwire->is (sutype::wt_trunk) || fixedwire->is (sutype::wt_preroute)) &&
            wire->is (sutype::wt_route) &&
            wire->rect().has_at_least_common_point (fixedwire->rect())
            ) {
          
          satindicesToCheckMostLikelyMandatory.push_back (satindex);
          wireServed = true;
          break;
        }

        if (
            wire->is (sutype::wt_enclosure) &&
            wire->rect().has_at_least_common_point (fixedwire->rect())
            ) {
          
          satindicesToCheckMostLikelyMandatory.push_back (satindex);
          wireServed = true;
          break;
        }
      }
      
      if (wireServed) continue;
      
      satindicesToCheckMostLikelyOptional.push_back (satindex);
    }
    
    SUASSERT (satindicesToCheckMostLikelyOptional.size() + satindicesToCheckMostLikelyMandatory.size() == satindicesToCheck.size(), "");
    
  } // end of suSatRouter::prepare_antennas_

  // satindices in satindicesToCheckLazily may duplicate satindices from satindicesToCheck
  // it's OK. It means that some optional satindices (i.e. removed antennas) can be emitted as constant-0 twice; not big problem here
  void suSatRouter::eliminate_antennas_by_bisection_ (sutype::clause_t & satindicesToCheck,
                                                      sutype::clause_t & satindicesToCheckLazily,
                                                      sutype::clause_t & assumptions,
                                                      sutype::uvi_t & nummandatory,
                                                      std::map<const suNet *, std::map <const suLayer *, sutype::wires_t, suLayer::cmp_const_ptr>, suNet::cmp_const_ptr> & fixedWiresPerLayerPerNet,
                                                      std::map<sutype::satindex_t,sutype::routes_t> & satindexToRoutes)
  {
    const bool np = false;

    sutype::uvi_t counter = 0;

    for (sutype::uvi_t i=0; i < satindicesToCheck.size(); ++i) {
      sutype::satindex_t satindex = satindicesToCheck[i];
      if (suSatSolverWrapper::instance()->is_constant (satindex)) continue;
      suWire * wire = get_wire_by_satindex (satindex);
      if (!wire) continue; // a wire may become null if I deleted some useless routes during antenna removal
      satindicesToCheck[counter] = satindex;
      ++counter;
    }

    if (satindicesToCheck.size() != counter) {
      satindicesToCheck.resize (counter);
    }
    
    if (satindicesToCheck.empty()) return;
    
    SUASSERT (assumptions.empty(), "");
    for (sutype::uvi_t i=0; i < satindicesToCheck.size(); ++i) {
      sutype::satindex_t satindex = satindicesToCheck[i];
      assumptions.push_back (-satindex);
    }
    SUASSERT (!assumptions.empty(), "");
    
    const double cputime0 = suTimeManager::instance()->get_cpu_time();
    bool ok = suSatSolverWrapper::instance()->solve_the_problem (assumptions);
    suStatic::increment_elapsed_time (sutype::ts_sat_antenna, suTimeManager::instance()->get_cpu_time() - cputime0);
    SUASSERT (!assumptions.empty(), "");
    
    // all tested satindices are optional
    if (ok) {

      SUASSERT (suSatSolverWrapper::instance()->model_is_valid(), "");

      satindicesToCheck.clear();
      SUASSERT (!assumptions.empty(), "");
      
      for (sutype::svi_t i=0; i < (sutype::svi_t)satindicesToCheckLazily.size(); ++i) {
          
        sutype::satindex_t satindex = satindicesToCheckLazily[i];
        suWire * wire = get_wire_by_satindex (satindex);
          
        if (!wire || suSatSolverWrapper::instance()->is_constant (satindex)) {
          satindicesToCheckLazily[i] = satindicesToCheckLazily.back();
          satindicesToCheckLazily.pop_back();
          --i;
          continue;
        }
          
        sutype::bool_t value = suSatSolverWrapper::instance()->get_modeled_value (satindex, sutype::bool_false);
        if (value != sutype::bool_false) continue;

        // this's an optional satindex
        // this code may be slow; It's OK for debug for now
        if (std::find (assumptions.begin(), assumptions.end(), -satindex) == assumptions.end()) {
          assumptions.push_back (-satindex);
        }
        
        satindicesToCheckLazily[i] = satindicesToCheckLazily.back();
        satindicesToCheckLazily.pop_back();
        --i;
      }
      
      // sort just to check that I didn't miss something in implementation
      // all satindices are expected to be different
      std::sort (assumptions.begin(), assumptions.end());
      
      for (sutype::uvi_t a=0; a < assumptions.size(); ++a) {
        sutype::satindex_t satindex = assumptions[a];
        SUASSERT (a == 0 || satindex > assumptions[a-1], "");
        SUINFO(np) << "ANTENNA: Found an optional satindex: " << get_wire_by_satindex (satindex)->to_str() << std::endl;
      }
      
      SUASSERT (!assumptions.empty(), "");
      SUASSERT (suSatSolverWrapper::instance()->model_is_valid(), "");
      suSatSolverWrapper::instance()->keep_model_unchanged (true);
      suSatSolverWrapper::instance()->emit_ALWAYS_ONE (assumptions);
      suSatSolverWrapper::instance()->keep_model_unchanged (false);
      
      update_affected_routes_ (satindexToRoutes, assumptions);
      SUASSERT (suSatSolverWrapper::instance()->model_is_valid(), "");
      
      assumptions.clear();
        
      if (!suSatSolverWrapper::instance()->model_is_valid()) {
        SUASSERT (false, "");
        const double cputime1 = suTimeManager::instance()->get_cpu_time();
        ok = suSatSolverWrapper::instance()->solve_the_problem (assumptions);
        suStatic::increment_elapsed_time (sutype::ts_sat_antenna, suTimeManager::instance()->get_cpu_time() - cputime1);
        SUASSERT (ok, "");
      }
      
      SUASSERT (suSatSolverWrapper::instance()->model_is_valid(), "");
      SUASSERT (satindicesToCheck.empty(), "");
      SUASSERT (assumptions.empty(), "");
      return;
    }

    SUASSERT (!suSatSolverWrapper::instance()->model_is_valid(), "");

    // restore all original assumptions
    SUASSERT (satindicesToCheck.size() == assumptions.size(), "");
    assumptions.clear();
    
    // found a mandatory index
    // make it always-true in assumptions
    // problem is unsatisfiable; model is invalid
    if (satindicesToCheck.size() == 1) {

      nummandatory += satindicesToCheck.size();

      sutype::satindex_t satindex = satindicesToCheck.front();
      suWire * wire = get_wire_by_satindex (satindex);
      SUASSERT (wire, "");
      
      fixedWiresPerLayerPerNet[wire->net()][wire->layer()].push_back (wire);
      
      suSatSolverWrapper::instance()->emit_ALWAYS_ONE (satindex);
      update_affected_routes_ (satindexToRoutes, satindex);
      
      SUINFO(np) << "ANTENNA: Found a mandatory satindex: " << wire->to_str() << std::endl;
      
      satindicesToCheck.clear();
      
      SUASSERT (!suSatSolverWrapper::instance()->model_is_valid(), "");
      SUASSERT (satindicesToCheck.empty(), "");
      SUASSERT (assumptions.empty(), "");
      return;
    }

    sutype::clause_t & satindicesToCheck1 = suClauseBank::loan_clause ();
    
    sutype::uvi_t num = satindicesToCheck.size()/2;
    
    // split the list
    for (sutype::uvi_t i=0; i < num; ++i) {
      sutype::satindex_t satindex = satindicesToCheck[i];
      suWire * wire = get_wire_by_satindex (satindex);
      if (!wire || suSatSolverWrapper::instance()->is_constant (satindex)) continue; // this satindex was already marked as a constant (maybe via satindicesToCheckLazily)
      satindicesToCheck1.push_back (satindex);
    }
    
    // check the first part
    eliminate_antennas_by_bisection_ (satindicesToCheck1, satindicesToCheckLazily, assumptions, nummandatory, fixedWiresPerLayerPerNet, satindexToRoutes);
    SUASSERT (satindicesToCheck1.empty(), "");
    SUASSERT (assumptions.empty(), "");

    // to detect known states for free - I need to solve
    if (!suSatSolverWrapper::instance()->model_is_valid()) {
      const double cputime1 = suTimeManager::instance()->get_cpu_time();
      ok = suSatSolverWrapper::instance()->solve_the_problem (assumptions);
      suStatic::increment_elapsed_time (sutype::ts_sat_other_time, suTimeManager::instance()->get_cpu_time() - cputime1);
      SUASSERT (ok, "");
    }
    
    // get the second part
    satindicesToCheck1.clear ();
    for (sutype::uvi_t i=num; i < satindicesToCheck.size(); ++i) {
      
      sutype::satindex_t satindex = satindicesToCheck[i];
      suWire * wire = get_wire_by_satindex (satindex);
      
      if (!wire || suSatSolverWrapper::instance()->is_constant (satindex)) continue; // this satindex was already marked as a constant (maybe via satindicesToCheckLazily)
      
      sutype::bool_t value = suSatSolverWrapper::instance()->get_modeled_value (satindex, sutype::bool_false);
      
      if (value == sutype::bool_false) {
        assumptions.push_back (-satindex);
      }
      else {
        satindicesToCheck1.push_back (satindex);
      }
    }

    // sort just to check that I didn't miss something in implementation
    // all satindices are expected to be different
    std::sort (assumptions.begin(), assumptions.end());    
    for (sutype::uvi_t a=0; a < assumptions.size(); ++a) {
      sutype::satindex_t satindex = assumptions[a];
      SUASSERT (a == 0 || satindex > assumptions[a-1], "");
      SUINFO(np) << "ANTENNA: Found an optional satindex: " << get_wire_by_satindex (satindex)->to_str() << std::endl;
    }
    
    if (!assumptions.empty()) {
      suSatSolverWrapper::instance()->keep_model_unchanged (true);
      suSatSolverWrapper::instance()->emit_ALWAYS_ONE (assumptions);
      suSatSolverWrapper::instance()->keep_model_unchanged (false);
      update_affected_routes_ (satindexToRoutes, assumptions);
    }
        
    assumptions.clear();
    
    // check the second part
    eliminate_antennas_by_bisection_ (satindicesToCheck1, satindicesToCheckLazily, assumptions, nummandatory, fixedWiresPerLayerPerNet, satindexToRoutes);
    SUASSERT (satindicesToCheck1.empty(), "");
    
    suClauseBank::return_clause (satindicesToCheck1);
    
    satindicesToCheck.clear();

    SUASSERT (satindicesToCheck.empty(), "");
    SUASSERT (assumptions.empty(), "");
    return;
    
  } // end of suSatRouter::eliminate_antennas_by_bisection_

  // I didn't notice any benefits; 
  void suSatRouter::release_useless_wires_and_delete_useless_routes_ (const sutype::clause_t & wiresatindices)
  {
    for (const auto & iter : wiresatindices) {
      
      sutype::satindex_t satindex = iter;
      sutype::satindex_t abssatindex = abs (satindex);

      SUASSERT (suSatSolverWrapper::instance()->is_constant (0, abssatindex), "");
      
      suWire * wire = get_wire_by_satindex (satindex);
      
      if (!wire) continue;

      SUASSERT (wire->satindex() == abssatindex, "");
      
      release_useless_wire_and_delete_useless_routes_ (wire);
    }
    
  } // end of suSatRouter::release_useless_wires_and_delete_useless_routes_

  // maybe slow because here I iterate through all ties and routes
  void suSatRouter::release_useless_wire_and_delete_useless_routes_ (suWire * wire)
  {
    const bool np = false;

    SUASSERT (wire, "");

    const suNet * net = wire->net();

    SUASSERT (_ties.count (net->id()) > 0, "");

    const sutype::ties_t & ties = _ties.at (net->id());

    for (const auto & iter1 : ties) {

      suTie * tie = iter1;
     
      sutype::routes_t & routes = tie->routes();

      for (sutype::svi_t r=0; r < (sutype::svi_t)routes.size(); ++r) {

        suRoute * route = routes[r];
        
        if (!route->has_wire (wire)) continue;
        
        if (_option_simplify_routes) {
          route->layoutFunc()->simplify();
        }
        
        sutype::satindex_t routesatindex =  route->layoutFunc()->satindex();

        if (suSatSolverWrapper::instance()->is_constant (0, routesatindex)) {

          SUINFO(np) << "ANTENNA: Deleted a useless route" << std::endl;

          delete route;

          routes[r] = routes.back();
          routes.pop_back();
          --r;

          SUASSERT (false, "Unexpected because after suSatRouter::post_routing_fix_routes all routes are in use; I don't expect any useless routes");
          
          continue;
        }

        route->release_useless_wires ();
      }
    }
    
  } // end of suSatRouter::release_useless_wire_and_delete_useless_routes_
  
  //
  suLayoutFunc * suSatRouter::create_a_counter_for_total_wire_width_ (const sutype::wires_t & wires,
                                                                      sutype::dcoord_t minWireWidth)
    const
  {
    SUASSERT (!wires.empty(), "No available wires to build a counter for total wire width");

    //
    bool needcounter = false;
    sutype::dcoord_t totalwirewidth = 0;

    sutype::clause_t & availablewidths = suClauseBank::loan_clause(); // to report only
    
    for (sutype::uvi_t w=0; w < wires.size(); ++w) {
      
      const suWire * wire = wires[w];
      SUASSERT (wire->width() > 0, "");

      if (wire->width() < minWireWidth) {
        needcounter = true;
      }

      sutype::dcoord_t width = wire->width();

      totalwirewidth += width;
      
      availablewidths.push_back (width);
    }

    if (!needcounter) {

      suLayoutFunc * func = suLayoutNode::create_func (sutype::logic_func_or, sutype::unary_sign_just, SUFILELINE_DLF);
      
      for (sutype::uvi_t w=0; w < wires.size(); ++w) {
        const suWire * wire = wires[w];
        func->add_leaf (wire->satindex());
      }

      suClauseBank::return_clause (availablewidths);
      return func;
    }

    SUASSERT (minWireWidth > 0, "Unexpected target wire width: " << minWireWidth);

    if (totalwirewidth < minWireWidth) {
      
      SUFATAL ("Could not build a counter for the wire width")
        << "; target width: " << minWireWidth
        << "; available total width: " << totalwirewidth
        << "; available widths: " << suStatic::to_str (availablewidths)
        << std::endl;

      SUFATAL ("Relaxed target width")
        << " from " << minWireWidth
        << " to " << totalwirewidth
        << std::endl;

      minWireWidth = totalwirewidth;
    }

    suClauseBank::return_clause (availablewidths);

    // check for a primitive case when we need all available wires
    bool needallwires = true;
    
    for (sutype::uvi_t w=0; w < wires.size(); ++w) {
      
      const suWire * wire = wires[w];
      if (totalwirewidth - wire->width() >= minWireWidth) {
        needallwires = false;
        break;
      }
    }

    if (needallwires) {
      
      suLayoutFunc * func = suLayoutNode::create_func (sutype::logic_func_and, sutype::unary_sign_just, SUFILELINE_DLF);
      
      for (sutype::uvi_t w=0; w < wires.size(); ++w) {
        const suWire * wire = wires[w];
        func->add_leaf (wire->satindex());
      }
      
      return func;
    }
    
    std::map<sutype::dcoord_t,sutype::wires_t> wiresPerWidthMap;

    for (sutype::uvi_t w=0; w < wires.size(); ++w) {
      suWire * wire = wires[w];
      wiresPerWidthMap[wire->width()].push_back (wire);
    }

    std::vector<sutype::wires_t> wiresPerWidth;
    for (const auto & iter : wiresPerWidthMap) {
      const sutype::wires_t & wiresHaveTheSameWidth = iter.second;
      wiresPerWidth.push_back (wiresHaveTheSameWidth);
    }
    
    SUASSERT (!wiresPerWidth.empty(), "");

    // one more corner case: all wires have the same width
    if (wiresPerWidth.size() == 1) {
      
      SUASSERT (wiresPerWidth.front().size() == wires.size(), "");
      sutype::dcoord_t width = wiresPerWidth.front().front()->width();
      int minnumwires = (minWireWidth / width) + ((minWireWidth % width) ? 1 : 0);
      sutype::layoutnodes_t nodes;
      for (sutype::uvi_t w=0; w < wires.size(); ++w) {
        suWire * wire = wires[w];
        nodes.push_back (suLayoutNode::create_leaf (wire->satindex()));
      }
      
      suLayoutFunc * func0 = suLayoutNode::create_counter (sutype::bc_greater_or_equal, sutype::unary_sign_just, nodes, minnumwires);
      return func0;
    }
        
    sutype::clause_t & currentwave = suClauseBank::loan_clause ();
    sutype::clauses_t combinationsOfWires; // sutype::clauses_t isused here just as a vector of integer vectors
    
    enumerate_possible_sets_of_wires_ (wiresPerWidth, 0, minWireWidth, currentwave, combinationsOfWires);

    SUASSERT (currentwave.empty(), "");
    suClauseBank::return_clause (currentwave);
    SUASSERT (!combinationsOfWires.empty(), "");
    
    suLayoutFunc * func0 = suLayoutNode::create_func (sutype::logic_func_or, sutype::unary_sign_just, SUFILELINE_DLF);

    for (const auto & iter : combinationsOfWires) {

      const sutype::clause_t & numWiresPerWidth = iter;
      SUASSERT (numWiresPerWidth.size() <= wiresPerWidth.size(), "");

      suLayoutFunc * func1 = func0->add_func (sutype::logic_func_and, sutype::unary_sign_just, SUFILELINE_DLF);
      
      for (sutype::uvi_t i=0; i < numWiresPerWidth.size(); ++i) {
        
        const sutype::wires_t & wiresHaveTheSameWidth = wiresPerWidth[i];        
        sutype::satindex_t numwires = numWiresPerWidth[i];
        SUASSERT (numwires >= 0 && numwires <= (sutype::satindex_t)wiresHaveTheSameWidth.size(), "");
        
        sutype::layoutnodes_t nodes;
        for (sutype::uvi_t w=0; w < wiresHaveTheSameWidth.size(); ++w) {
          suWire * wire = wiresHaveTheSameWidth[w];
          nodes.push_back (suLayoutNode::create_leaf (wire->satindex()));
        }
        
        func1->add_node (suLayoutNode::create_counter (sutype::bc_greater_or_equal, sutype::unary_sign_just, nodes, numwires));
      }
    }
    
    return func0;
    
  } // end of suSatRouter::create_a_counter_for_total_wire_width_
  
  //
  void suSatRouter::enumerate_possible_sets_of_wires_ (const std::vector<sutype::wires_t> & wiresPerWidth,
                                                       sutype::uvi_t index,
                                                       sutype::dcoord_t targetwidth,
                                                       sutype::clause_t & currentwave,
                                                       sutype::clauses_t & combinationsOfWires)
    const
  {
    SUASSERT (targetwidth > 0, "");
    SUASSERT (!wiresPerWidth.empty(), "");
    SUASSERT (index >= 0 && index < wiresPerWidth.size(), "");
    
    const sutype::wires_t & wiresHaveTheSameWidth = wiresPerWidth[index];
    SUASSERT (!wiresHaveTheSameWidth.empty(), "");
    
    const sutype::dcoord_t width = wiresHaveTheSameWidth.front()->width();
    
    for (sutype::uvi_t numwires = 0; numwires <= wiresHaveTheSameWidth.size(); ++numwires) {

      currentwave.push_back (numwires);
      targetwidth -= (numwires * width);
      
      // reached target width
      if (targetwidth <= 0) {
        
        combinationsOfWires.push_back (currentwave);
        
        // restore
        targetwidth += (numwires * width);
        currentwave.pop_back();

        break;
      }
      
      // run recursion
      if (index+1 < wiresPerWidth.size()) {
        enumerate_possible_sets_of_wires_ (wiresPerWidth, index+1, targetwidth, currentwave, combinationsOfWires);
      }
      
      // restore
      targetwidth += (numwires * width);
      currentwave.pop_back();
    }
    
  } // end of suSatRouter::enumerate_possible_sets_of_wires_

  // net by net
  void suSatRouter::optimize_the_number_of_open_connected_entities_ ()
    const
  {
    for (const auto & iter1 : _connectedEntities) {
      
      sutype::id_t netid = iter1.first;
      const suNet * net = suCellManager::instance()->get_net_by_id (netid);

      const auto & ces = iter1.second;
      
      sutype::clause_t & clause = suClauseBank::loan_clause ();
    
      for (const auto & iter2 : ces) {

        const suConnectedEntity * ce = iter2;
        
        sutype::satindex_t openSatindex = ce->openSatindex();
        SUASSERT (openSatindex != sutype::UNDEFINED_SAT_INDEX, "");
        if (suSatSolverWrapper::instance()->is_constant (1, openSatindex)) continue; // nothing to optimize
        if (suSatSolverWrapper::instance()->is_constant (0, openSatindex)) continue; // nothing to optimize
        
        clause.push_back (openSatindex);
      }
    
      if (!clause.empty()) {
        SUINFO(1) << "Minimize the number of " << clause.size() << " open connected entities for net " << net->name() << std::endl;
        suSatSolverWrapper::instance()->optimize_satindices (clause, sutype::om_minimize);
      }
      else {
        SUINFO(1) << "Found no possible open connected entities for net " << net->name() << std::endl;
      }
      
      suClauseBank::return_clause (clause);
    }
    
  } // end of suSatRouter::optimize_the_number_of_open_connected_entities_

  // net by net
  void suSatRouter::optimize_the_number_of_open_ties_ ()
    const
  {
    std::map<sutype::dcoord_t, sutype::ids_t> netHalfPerimeters;

    // calculate half perimeter
    for (const auto & iter1 : _ties) {

      sutype::id_t netid = iter1.first;
      const suNet * net = suCellManager::instance()->get_net_by_id (netid);
      const sutype::wires_t & wires = net->wires();

      sutype::dcoord_t xl, yl, xh, yh;
      xl = yl = xh = yh = 0;

      for (sutype::uvi_t i=0; i < wires.size(); ++i) {

        suWire * wire = wires[i];
        
        if (i == 0 || wire->rect().xl() < xl) xl = wire->rect().xl();
        if (i == 0 || wire->rect().yl() < yl) yl = wire->rect().yl();
        if (i == 0 || wire->rect().xh() > xh) xh = wire->rect().xh();
        if (i == 0 || wire->rect().yh() > yh) yh = wire->rect().yh();
      }

      sutype::dcoord_t p = (xh - xl) + (yh - yl);
      netHalfPerimeters[p].push_back (netid);
    }

    // nets are already sorted
    // just put then in a vector
    sutype::ids_t sortednetids;

    for (const auto & iter1 : netHalfPerimeters) {
      const sutype::ids_t & netids = iter1.second;
      sortednetids.insert (sortednetids.end(), netids.begin(), netids.end());
    }

    // optimize opens net by net
    for (sutype::uvi_t i=0; i < sortednetids.size(); ++i) {

      sutype::id_t netid = sortednetids[i];
      const suNet * net = suCellManager::instance()->get_net_by_id (netid);
      
      const auto & ties = _ties.at (netid);
      
      sutype::clause_t & clause = suClauseBank::loan_clause ();

      for (const auto & iter2 : ties) {

        const suTie * tie = iter2;
        if (tie->routes().empty()) continue;

        sutype::satindex_t openSatindex = tie->openSatindex();
        SUASSERT (openSatindex != sutype::UNDEFINED_SAT_INDEX, "");
        
        if (suSatSolverWrapper::instance()->is_constant (openSatindex)) continue; // nothing to optimize
        
        clause.push_back (openSatindex);
      }
      
      if (!clause.empty()) {
        SUINFO(1) << "Minimize the number of " << clause.size() << " open ties for net " << net->name() << " (" << (i+1) << " of " << sortednetids.size() << ")" << std::endl;
        suSatSolverWrapper::instance()->optimize_satindices (clause, sutype::om_minimize);
      }
      else {
        SUINFO(1) << "Found no possible open ties for net " << net->name() << std::endl;
      }
      
      suClauseBank::return_clause (clause);
    }
    
  } // suSatRouter::optimize_the_number_of_open_ties_

  //
  void suSatRouter::optimize_the_number_of_direct_wires_between_preroutes_ (bool createpreroutes)
  {
    const sutype::layers_tc & layers = suLayerManager::instance()->layers();
    
    for (const auto & iter : layers) {
      
      const suLayer * layer = iter;
      if (!layer->is (sutype::lt_wire)) continue;
      if (!layer->is (sutype::lt_metal)) continue;
      
      sutype::opt_mode_t optmode = sutype::om_maximize;
      if (layer->level() <= suLayerManager::instance()->lowestDRLayer()->level())
        optmode = sutype::om_minimize;
      
      SUINFO(1) << "Optimize the number of direct I-routes on " << layer->name() << std::endl;
      optimize_the_number_of_direct_wires_between_preroutes_ (layer, optmode, createpreroutes);
    }
 
  } // end of suSatRouter::optimize_the_number_of_direct_wires_between_preroutes_

  //
  void suSatRouter::optimize_the_number_of_direct_wires_between_preroutes_ (const suLayer * targetbaselayer,
                                                                            sutype::opt_mode_t optmode,
                                                                            bool createpreroutes)
  {
    SUASSERT (targetbaselayer->is_base(), "");
    SUASSERT (targetbaselayer->is (sutype::lt_wire), "");

    const suRegion * region = suGlobalRouter::instance()->get_region (0, 0);
    SUASSERT (region, "");

    const sutype::dcoord_t maxdist = 2 * ((targetbaselayer->pgd() == sutype::go_hor) ? region->bbox().w() : region->bbox().h());
    
    // depth 1: one single wire connects two preroutes
    // depth 2: two wires together connect two preroutes
    int mindepth = 1;
    int maxdepth = 2;

    // during sutype::om_maximize, I optimize wires specially created for this optimization
    if (optmode == sutype::om_maximize) {
      maxdepth = 1;
    }

    std::map<const suNet*,sutype::wires_t,suNet::cmp_const_ptr> netWiresOfTargetLayer; // preroutes
    std::map<const suNet*,sutype::wires_t,suNet::cmp_const_ptr> workwiresPerNet; // wires here touch at least one net's preroute
    
    const sutype::nets_tc & netsToRoute = suCellManager::instance()->netsToRoute();

    // find preroutes
    for (sutype::uvi_t i=0; i < _satindexToPointer.size(); ++i) {

      suWire * netwire = _satindexToPointer[i];
      
      if (!netwire) continue;
      if (!netwire->is (sutype::wt_preroute)) continue;
      if (netwire->layer()->base() != targetbaselayer) continue;

      const suNet * net = netwire->net();
      if (std::find (netsToRoute.begin(), netsToRoute.end(), net) == netsToRoute.end()) continue;
      
      netWiresOfTargetLayer [net].push_back (netwire);
    }
    
    // create direct wires
    if (optmode == sutype::om_maximize) {
      
      for (const auto & iter : netWiresOfTargetLayer) {
        const sutype::wires_t & netwires = iter.second;
        create_direct_wires_ (netwires);
      }
    }

    // find work wires we want to optimize
    for (sutype::uvi_t i=0; i < _satindexToPointer.size(); ++i) {
        
      suWire * workwire = _satindexToPointer[i];

      if (!workwire) continue;
      if (workwire->layer()->base() != targetbaselayer) continue;
      
      sutype::satindex_t satindex = workwire->satindex();
      if (suSatSolverWrapper::instance()->is_constant (satindex)) continue;

      // I optimize wires specially created for this optimization
      if (optmode == sutype::om_maximize) {
        if (!workwire->is (sutype::wt_route)) continue;
      }

      SUASSERT (!workwire->is (sutype::wt_preroute), "");
      
      const suNet * net = workwire->net();
      if (netWiresOfTargetLayer.count (net) == 0) continue; // net has no preroutes
      
      const sutype::wires_t & netwires = netWiresOfTargetLayer.at (net);
      if (netwires.size() <= 1) continue; // there's nothing to connect directly
      
      for (sutype::uvi_t k=0; k < netwires.size(); ++k) {

        suWire * netwire1 = netwires[k];
        if (netwire1->sidel() != workwire->sidel()) continue;
        if (netwire1->sideh() != workwire->sideh()) continue;
        if (netwire1->layer() != workwire->layer()) continue;
        if (!netwire1->rect().has_at_least_common_point (workwire->rect())) continue;

        workwiresPerNet[net].push_back (workwire);
        break;
      }
    }

    // optimize net by net
    for (const auto & iter0 : workwiresPerNet) {

      const suNet * net = iter0.first;

      SUASSERT (netWiresOfTargetLayer.count (net) > 0, "");
      
      const sutype::wires_t & netwires = netWiresOfTargetLayer.at (net);
      SUASSERT (netwires.size() >= 2, "");

      const sutype::wires_t & workwires = iter0.second;
      SUASSERT (!workwires.empty(), "");

      // here I collect preroutes which touch a work wire
      std::vector<sutype::wires_t> preroutesOfWorkWires (workwires.size());
      SUASSERT (preroutesOfWorkWires.size() == workwires.size(), "");

      for (sutype::uvi_t i=0; i < workwires.size(); ++i) {

        suWire * workwire = workwires[i]; // a wire we want to optimize
        SUASSERT (preroutesOfWorkWires[i].empty(), ""); // not populated yet
      
        for (sutype::uvi_t k=0; k < netwires.size(); ++k) {
          
          suWire * netwire = netwires[k]; // preroute
          
          if (netwire->sidel() != workwire->sidel()) continue;
          if (netwire->sideh() != workwire->sideh()) continue;
          if (netwire->layer() != workwire->layer()) continue;
          if (!netwire->rect().has_at_least_common_point (workwire->rect())) continue;

          preroutesOfWorkWires[i].push_back (netwire);
        }

        SUASSERT (!preroutesOfWorkWires[i].empty(), "By default any work wire in this list has at least one preroute it touches");
      }

      // depth 1: one single wire connects two preroutes
      // depth 2: two wires together connect two preroutes
      for (int depth = mindepth; depth <= maxdepth; ++depth) {
        
        sutype::clause_t & clause = suClauseBank::loan_clause ();

        // find wires those connect two preroutes
        if (depth == 1) {

          for (sutype::uvi_t i=0; i < workwires.size(); ++i) {

            suWire * workwire = workwires[i];

            bool wireConnectsTwoPreroutes = false;
            
            const sutype::wires_t & preroutes = preroutesOfWorkWires[i];
            SUASSERT (!preroutes.empty(), "");
            if (preroutes.size() < 2) continue; // nothing to optimize
            
            for (sutype::uvi_t k=0; k < preroutes.size(); ++k) {

              suWire * netwire1 = preroutes[k];

              for (sutype::uvi_t j=k+1; j < preroutes.size(); ++j) {

                suWire * netwire2 = preroutes[j];

                SUASSERT (netwire1->sidel() == netwire2->sidel(), "");
                SUASSERT (netwire1->sideh() == netwire2->sideh(), "");
                SUASSERT (netwire1->layer() == netwire2->layer(), "");
                
                if (netwire2->rect().has_at_least_common_point (netwire1->rect())) continue; // these two net wires are already connected (but not merged for some reason)

                // too long
                if (optmode == sutype::om_maximize) {
                  sutype::dcoord_t dist = netwire1->rect().distance_to (netwire2->rect(), targetbaselayer->pgd()); // x for hor; y for ver
                  if (dist > maxdist) continue;
                }

                // I don't like cases where wires are maximized beyond original terminals
                if (optmode == sutype::om_maximize) {

                  sutype::dcoord_t bboxedgel = std::min (netwire1->edgel(), netwire2->edgel());
                  sutype::dcoord_t bboxedgeh = std::max (netwire1->edgeh(), netwire2->edgeh());

                  if (workwire->edgel() < bboxedgel) continue; // I don't like such long wires
                  if (workwire->edgeh() > bboxedgeh) continue; // I don't like such long wires
                }

                //SUINFO(1) << "Found a workwire=" << workwire->to_str() << "; netwire1=" << netwire1->to_str() << "; netwire2=" << netwire2->to_str() << std::endl;
                
                // we found netwire1 & netwire2 those are not connected but workwire touches both these wires
                wireConnectsTwoPreroutes = true;
                break;
              }
              
              if (wireConnectsTwoPreroutes) break;
            }
            
            if (!wireConnectsTwoPreroutes) continue;

            clause.push_back (workwire->satindex());
          }

          SUASSERT (clause.size() <= workwires.size(), "");
        }
        
        // find pairs of wires those connect two preroutes
        else if (depth == 2) {

          for (sutype::uvi_t i=0; i < workwires.size(); ++i) {

            suWire * workwire1 = workwires[i];
            const sutype::wires_t & preroutes1 = preroutesOfWorkWires[i];
            SUASSERT (!preroutes1.empty(), "");

            for (sutype::uvi_t k=i+1; k < workwires.size(); ++k) {

              suWire * workwire2 = workwires[k];
              const sutype::wires_t & preroutes2 = preroutesOfWorkWires[k];
              SUASSERT (!preroutes2.empty(), "");
              
              if (workwire1->sidel() != workwire2->sidel()) continue;
              if (workwire1->sideh() != workwire2->sideh()) continue;
              if (workwire1->layer() != workwire2->layer()) continue;
              if (!workwire1->rect().has_at_least_common_point (workwire2->rect())) continue;

              bool twoWireConnectTwoPreroutes = false;
              
              for (sutype::uvi_t j=0; j < preroutes1.size(); ++j) {

                suWire * netwire1 = preroutes1[j];
                if (netwire1->rect().has_at_least_common_point (workwire2->rect())) continue; // it's not our case; workwire2 by default already connects two preroutes

                for (sutype::uvi_t s=0; s < preroutes2.size(); ++s) {

                  suWire * netwire2 = preroutes2[s];
                  if (netwire2->rect().has_at_least_common_point (workwire1->rect())) continue; // it's not our case; workwire1 by default already connects two preroutes

                  SUASSERT (netwire1->sidel() == netwire2->sidel(), "");
                  SUASSERT (netwire1->sideh() == netwire2->sideh(), "");
                  SUASSERT (netwire1->layer() == netwire2->layer(), "");
                  
                  if (netwire2->rect().has_at_least_common_point (netwire1->rect())) continue; // these two net wires are already connected (but not merged for some reason)

                  // too long
                  if (optmode == sutype::om_maximize) {
                    sutype::dcoord_t dist = netwire1->rect().distance_to (netwire2->rect(), targetbaselayer->pgd()); // x for hor; y for ver
                    if (dist > maxdist) continue;
                  }

                  // I don't like cases where wires are maximized beyond original terminals
                  if (optmode == sutype::om_maximize) {
                    
                    sutype::dcoord_t bboxedgel = std::min (netwire1->edgel(), netwire2->edgel());
                    sutype::dcoord_t bboxedgeh = std::max (netwire1->edgeh(), netwire2->edgeh());
                    
                    if (workwire1->edgel() < bboxedgel) continue; // I don't like such long wires
                    if (workwire1->edgeh() > bboxedgeh) continue; // I don't like such long wires
                    if (workwire2->edgel() < bboxedgel) continue; // I don't like such long wires
                    if (workwire2->edgeh() > bboxedgeh) continue; // I don't like such long wires
                  }
                  
                  twoWireConnectTwoPreroutes = true;
                  break;
                }

                if (twoWireConnectTwoPreroutes) break;
              }

              if (!twoWireConnectTwoPreroutes) continue;

              // workwire1 & workwire2 touch each other
              // workwire1 touches some netwire1 but does not touch netwire2
              // workwire2 touches some netwire2 but does not touch netwire1
              // netwire2 and netwire1 do not touch each other
              
              sutype::clause_t & clause2 = suClauseBank::loan_clause ();
              clause2.push_back (workwire1->satindex());
              clause2.push_back (workwire2->satindex());
              sutype::satindex_t satindex = suSatSolverWrapper::instance()->emit_AND_or_return_constant (clause2);
              suClauseBank::return_clause (clause2);

              clause.push_back (satindex);
            }
          }
        }
        
        // unexpected depth
        else {
          SUASSERT (false, "depth=" << depth);
        }
        
        // nothing to optimize
        if (clause.empty()) {
          suClauseBank::return_clause (clause);
          continue;
        }
        
        SUASSERT (!clause.empty(), "");
        
        SUINFO(1)
          << suStatic::opt_mode_2_str (optmode) << " the number of " 
          << clause.size() << " I-cases"
          << " for layer " << targetbaselayer->name()
          << " and net " << net->name()
          << "; depth=" << depth
          << std::endl;
        
        suSatSolverWrapper::instance()->optimize_satindices (clause, optmode);
        
        SUASSERT (suSatSolverWrapper::instance()->model_is_valid(), "");

        // debug
        if (depth == 1) {

          SUASSERT (!clause.empty(), "");

          SUINFO(0) << "Num wires: " << clause.size() << std::endl;
        
          for (const auto & iter2 : clause) {
          
            sutype::satindex_t satindex = iter2;
            sutype::bool_t value = suSatSolverWrapper::instance()->get_modeled_value (satindex);
            suWire * wire = get_wire_by_satindex (satindex);

            SUINFO(0)
              << "  optmode=" << suStatic::opt_mode_2_str (optmode)
              << "  satindex=" << satindex
              << "; value=" << suStatic::bool_2_str(value)
              << "; " << wire->to_str()
              << std::endl;
          
            if (value == sutype::bool_true && optmode == sutype::om_maximize) continue;
            if (value == sutype::bool_false && optmode == sutype::om_minimize) continue;
            if (optmode != sutype::om_minimize) continue;
          
            SUISSUE("Found a suspect I-wire (written an error i_route") << ": " << wire->to_str() << std::endl;

            suErrorManager::instance()->add_error (std::string ("i_route"),
                                                   wire->rect().xl(),
                                                   wire->rect().yl(),
                                                   wire->rect().xh(),
                                                   wire->rect().yh());
            
            sutype::ties_t debugties;
            sutype::routes_t debugroutes;

            find_ties_having_a_route_with_this_wire_ (wire, debugties, debugroutes);

            SUASSERT (debugties.size() == debugroutes.size(), "");
            for (sutype::uvi_t i=0; i < debugties.size(); ++i) {

              suTie * tie = debugties[i];
              suRoute * route = debugroutes[i];

              SUISSUE("Debug message about I-route") << ": tie=" << tie->to_str() << "; wire1=" << route->wires()[0]->to_str() << "; wire2=" << route->wires()[1]->to_str() << std::endl;
            }
          }
        }

        // create preroutes
        if (createpreroutes && depth == 1 && optmode == sutype::om_maximize) {

          sutype::wires_t wirestomakepermanent;

          for (const auto & iter1 : clause) {

            sutype::satindex_t satindex = iter1;
            SUASSERT (satindex > 0, "");

            suWire * wire = get_wire_by_satindex (satindex);
            SUASSERT (wire, "");
            SUASSERT (wire->is (sutype::wt_route), "");
            SUASSERT (wire->satindex() == satindex, "");

            //if (!suSatSolverWrapper::instance()->is_constant (1, satindex)) continue;
            sutype::bool_t value = suSatSolverWrapper::instance()->get_modeled_value (satindex);
            if (value != sutype::bool_true) continue;

            wirestomakepermanent.push_back (wire);
          }

          for (const auto & iter1 : wirestomakepermanent) {

            suWire * wire = iter1;
            sutype::satindex_t satindex = wire->satindex();

            if (!suSatSolverWrapper::instance()->is_constant (1, satindex)) {
              suSatSolverWrapper::instance()->emit_ALWAYS_ONE (satindex);
            }
            
            suWire * newwire = suWireManager::instance()->create_wire_from_wire (wire->net(), wire, sutype::wt_preroute); // detected wire that could be a preroute
            SUASSERT (newwire, "");
            
            if (newwire->satindex() == sutype::UNDEFINED_SAT_INDEX) {
              
              ((suNet*)wire->net())->add_wire (newwire);
              register_a_sat_wire (newwire);
              suSatSolverWrapper::instance()->emit_ALWAYS_ONE (newwire->satindex());
              
              SUINFO(1) << "Auto-detected a preroute: " << newwire->to_str() << std::endl;
            }
          }
        }
        
        suClauseBank::return_clause (clause);
        
      } // for(depth)

    } // for(iter0/nets)
    
  } // suSatRouter::optimize_the_number_of_direct_wires_between_preroutes_

  // wires belong to one net and one layer
  void suSatRouter::create_direct_wires_ (const sutype::wires_t & netwires)
  {
    if (netwires.size() <= 1) return;

    typedef std::tuple<sutype::dcoord_t,sutype::dcoord_t,const suLayer*> key_t;

    std::map<key_t,sutype::wires_t> wiresPerTracks;

    const suNet * net = 0;
    const suLayer * baselayer = 0;

    for (const auto & iter : netwires) {

      suWire * wire = iter;

      if (net == 0) net = wire->net();
      if (baselayer == 0) baselayer = wire->layer()->base();

      SUASSERT (net == wire->net(), "");
      SUASSERT (baselayer == wire->layer()->base(), "");

      key_t key (wire->sidel(), wire->sideh(), wire->layer());

      wiresPerTracks[key].push_back (wire);
    }

    SUASSERT (net, "");
    SUASSERT (baselayer, "");

    sutype::uvi_t counter = 0;

    for (auto & iter : wiresPerTracks) {

      sutype::wires_t & trackwires = iter.second;
      std::sort (trackwires.begin(), trackwires.end(), suStatic::compare_wires_by_edgel);
      
      for (sutype::uvi_t i=0; i < trackwires.size(); ++i) {

        suWire * wire1 = trackwires[i];

        for (sutype::uvi_t k=i+1; k < trackwires.size(); ++k) {

          suWire * wire2 = trackwires[k];
          
          SUASSERT (wire1->sidel() == wire2->sidel(), "");
          SUASSERT (wire1->sideh() == wire2->sideh(), "");
          SUASSERT (wire1->layer() == wire2->layer(), "");
          SUASSERT (wire1->edgel() <= wire2->edgel(), "");
          
          if (wire2->edgel() <= wire1->edgeh()) continue; // no break between wires

          bool mayskipthiscase = false;
          for (sutype::svi_t j = (sutype::svi_t)k - 1; j > (sutype::svi_t)i; --j) {
            suWire * wire3 = trackwires[j];
            if (wire3->edgeh() >= wire2->edgel()) {
              mayskipthiscase = true;
              break;
            }
          }
          if (mayskipthiscase) break; // no break between wires
          
          suWire * wire = suMetalTemplateManager::instance()->create_wire_to_match_metal_template_instance (wire1->net(),
                                                                                                            wire1->layer(),
                                                                                                            wire1->edgeh(), // fill a cut between wires
                                                                                                            wire2->edgel(), // fill a cut between wires
                                                                                                            wire1->sidel(),
                                                                                                            wire1->sideh(),
                                                                                                            sutype::wt_route);
          
          if (wire && wire->satindex() == sutype::UNDEFINED_SAT_INDEX && !suSatRouter::instance()->wire_is_legal_slow_check (wire)) {
            
            //SUASSERT (suWireManager::instance()->get_wire_usage (wire) == 1, wire->to_str());
            
            if (suWireManager::instance()->get_wire_usage (wire) != 1) {
              SUISSUE("Illegal wire has unexpected usage. Please ask Nikolai to review")
                << ": " << wire->to_str()
                << std::endl;
            }
            
            suWireManager::instance()->release_wire (wire);
            wire = 0;
          }

          // we don't need this extra instance of the wire; otherwise I will have to save it as homeless
          if (wire && suWireManager::instance()->get_wire_usage (wire) > 1) {
            suWireManager::instance()->release_wire (wire);
            wire = 0;
          }
          
          if (wire && wire->satindex() == sutype::UNDEFINED_SAT_INDEX) {
            register_a_sat_wire (wire);
            emit_conflicts_ (wire);
          }
          
          if (wire) {
            ++counter;
          }
          
          break;
        }
      }
    }

    if (counter > 0) {
      SUINFO(1) << "Created " << counter << " extra direct wires for net " << net->name() << " and layer " << baselayer->to_str() << std::endl;
    }
       
  } // end of suSatRouter::create_direct_wires_

  // IMPORTANT: shapes of via enclosures may be less than shapes of metal templates
  void suSatRouter::optimize_the_number_of_tracks_ (sutype::wire_type_t targetwiretype,
                                                    bool fixWiresOnFly,
                                                    bool splitWiresPerGrStripes,
                                                    int numberOfWiresWeMayLeave)
    const
  {
    std::map<const suNet*,sutype::wires_t,suNet::cmp_const_ptr> wiresPerNet;
    
    for (sutype::uvi_t i=0; i < _satindexToPointer.size(); ++i) {
      
      suWire * wire = _satindexToPointer[i];
      
      if (!wire) continue;
      if (!wire->layer()->is (sutype::lt_wire)) continue; // skip vias and other strange layers
      
      if (targetwiretype != sutype::wt_undefined) {
        if (!wire->is (targetwiretype)) continue;
      }

      if (!wire->is (sutype::wt_preroute) && 
          !wire->is (sutype::wt_trunk)    &&
          !wire->is (sutype::wt_route)    &&
          !wire->is (sutype::wt_shunt)) continue;

      sutype::satindex_t satindex = wire->satindex();
      SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");
      if (suSatSolverWrapper::instance()->is_constant (0, satindex)) continue; // skip unfeasible wires
      
      wiresPerNet[wire->net()].push_back (wire);
    }

    // sort nets by criticality

    // net by net to reduce complexity
    for (auto & iter : wiresPerNet) {
      
      sutype::wires_t & wires = iter.second;
      std::sort (wires.begin(), wires.end(), suStatic::compare_wires_by_id);
      
      for (sutype::uvi_t i=1; i < wires.size(); ++i) {
        suWire * wire = wires[i];
        SUASSERT (wire->id() > wires[i-1]->id(), "");
      }

      suSatRouter::optimize_the_number_of_tracks_ (wires, fixWiresOnFly, splitWiresPerGrStripes, numberOfWiresWeMayLeave);
    }
    
  } // end of suSatRouter::optimize_the_number_of_tracks_

  //
  void suSatRouter::optimize_the_number_of_tracks_ (const sutype::wires_t & wires,
                                                    bool fixWiresOnFly,
                                                    bool splitWiresPerGrStripes,
                                                    int numberOfWiresWeMayLeave)
    const
  {
    SUASSERT (numberOfWiresWeMayLeave == -1 || numberOfWiresWeMayLeave == 1, "");

    std::map<const suLayer*,sutype::wires_t,suLayer::cmp_const_ptr_by_level_reverse> wiresPerLayer;

    const suNet * net = 0;

    for (const auto & iter : wires) {

      suWire * wire = iter;

      if (net == 0)
        net = wire->net();

      SUASSERT (net == wire->net(), "");
      
      const suLayer * baselayer = wire->layer()->base();
      
      SUASSERT (baselayer, "");
      SUASSERT (baselayer->is (sutype::lt_wire), "");
      SUASSERT (baselayer->pgd() == sutype::go_ver || baselayer->pgd() == sutype::go_hor, "");
      SUASSERT (baselayer->is_base(), "");

      wiresPerLayer[baselayer].push_back (wire);
    }
    
    // layer by layer (highest first)
    for (const auto & iter0 : wiresPerLayer) {

      const suLayer * baselayer = iter0.first;
      const sutype::wires_t & layerwires = iter0.second;

      if (!splitWiresPerGrStripes) {
        
        optimize_the_number_of_tracks_of_layered_wires_ (layerwires, fixWiresOnFly, numberOfWiresWeMayLeave);
        continue;
      }
      
      SUINFO(1)
        << "Split wires per GR slices"
        << "; net=" << (net ? net->name() : "")
        << "; baselayer=" << baselayer->to_str()
        << "; wires=" << layerwires.size()
        << std::endl;
        
      std::map<int,sutype::wires_t> wiresPerGrStripe; // GR stripe
        
      for (const auto & iter1 : layerwires) {

        suWire * wire = iter1;
          
        // col for ver layers
        // row for hor layers
        int c = suGlobalRouter::instance()->dcoord_to_region_coord (wire->sidec(),
                                                                    baselayer->ogd(), 
                                                                    (baselayer->ogd() == sutype::go_hor) ? sutype::side_west : sutype::side_south);

        SUINFO(0)
          << "sidec=" << wire->sidec()
          << "; coord=" << c
          << "; isconstant1=" << (suSatSolverWrapper::instance()->is_constant (1, wire->satindex()) ? "yes" : "no")
          << "; wire=" << wire->to_str() << std::endl;
          
        wiresPerGrStripe[c].push_back (wire);
      }
        
      SUINFO(1) << "Found " << wiresPerGrStripe.size() << " GR slices." << std::endl;
        
      for (const auto & iter1 : wiresPerGrStripe) {

        //int slicecoord = iter1.first;
        const sutype::wires_t & layerWiresPerGrStripe = iter1.second;

        std::map<int,sutype::wires_t> wiresPerGrTile; // GR tile    

        for (const auto & iter2 : layerWiresPerGrStripe) {

          suWire * wire = iter2;

          // row for ver layers
          // col for hor layers
          int c = suGlobalRouter::instance()->dcoord_to_region_coord (wire->edgel(),
                                                                      baselayer->pgd(), 
                                                                      (baselayer->pgd() == sutype::go_hor) ? sutype::side_west : sutype::side_south);

          wiresPerGrTile[c].push_back (wire);
        }

        for (const auto & iter2 : wiresPerGrTile) {

          //int tilecoord = iter2.first;
          const sutype::wires_t & layerWiresPerGrTile = iter2.second;
          
          int numberOfWiresWeMayLeave2 = numberOfWiresWeMayLeave;
            
          for (const auto & iter3 : layerWiresPerGrTile) {
            
            suWire * wire = iter3;
            if (suSatSolverWrapper::instance()->is_constant (1, wire->satindex())) {
              numberOfWiresWeMayLeave2 = -1;
              //break;
            }
              
            //suErrorManager::instance()->add_error ("DBG_" + baselayer->name() + "_" + net->name() + "_slice_" + suStatic::dcoord_to_str(slicecoord) + "_tile_" + suStatic::dcoord_to_str(tilecoord) + "_" + suStatic::dcoord_to_str(numberOfWiresWeMayLeave2), wire->rect());
          }
            
          optimize_the_number_of_tracks_of_layered_wires_ (layerWiresPerGrTile, fixWiresOnFly, numberOfWiresWeMayLeave2);
        }
      } 
    }
    
  } // end of suSatRouter::optimize_the_number_of_tracks_

  //
  void suSatRouter::optimize_the_number_of_tracks_of_layered_wires_ (const sutype::wires_t & wires,
                                                                     bool fixWiresOnFly,
                                                                     int numberOfWiresWeMayLeave)
    const
  {
    SUASSERT (numberOfWiresWeMayLeave == -1 || numberOfWiresWeMayLeave > 0, "");

    std::map <sutype::dcoord_t, std::map <sutype::dcoord_t,sutype::wires_t> > wiresPerSideLAndSideH;
    
    for (const auto & iter : wires) {
      
      suWire * wire = iter;
      
      sutype::satindex_t satindex = wire->satindex();
      SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");

      if (suSatSolverWrapper::instance()->is_constant (0, satindex)) {
        SUASSERT (false, "");
        continue;
      }

      wiresPerSideLAndSideH [wire->sidel()][wire->sideh()].push_back (wire);
    }
    
    sutype::clause_t & clause = suClauseBank::loan_clause ();
    
    for (const auto & iter0 : wiresPerSideLAndSideH) {

      sutype::dcoord_t sidel  = iter0.first;
      const auto & container1 = iter0.second;

      for (const auto & iter1 : container1) {

        sutype::dcoord_t sideh             = iter1.first;
        const sutype::wires_t & trackwires = iter1.second;

        SUASSERT (!trackwires.empty(), "");

        // There's nothing to optimizeif we have only constants here
        bool foundWireToOptimize = false;
        for (const auto & iter2 : trackwires) {
          suWire * wire = iter2;
          sutype::satindex_t satindex = wire->satindex();
          if (suSatSolverWrapper::instance()->is_constant (satindex)) {
            SUASSERT (numberOfWiresWeMayLeave == -1, "unexpected all other values");
            continue;
          }
          foundWireToOptimize = true;
          break;
        }
        if (!foundWireToOptimize) continue;
                
        // find connected groups of wires
        sutype::wires_t trackwireshardcopy = trackwires;
        std::vector<sutype::wires_t> ces;
        calculate_connected_entities_ (trackwireshardcopy, ces);
        SUASSERT (!ces.empty(), "");
        
        if (ces.size() > 1) {
          SUINFO(1)
            << "Track wires were split onto " << ces.size() << " groups"
            << "; layer=" << trackwires.front()->layer()->to_str()
            << "; net=" << trackwires.front()->net()->name()
            << "; sidel=" << sidel
            << "; sideh=" << sideh
            << std::endl;
        }

        for (const auto & iter2 : ces) {
        
          const sutype::wires_t & subtrackwires = iter2;
          SUASSERT (!subtrackwires.empty(), "");
        
          sutype::clause_t & satindicesOfATrack = suClauseBank::loan_clause ();

          for (const auto & iter3 : subtrackwires) {

            suWire * wire = iter3;

            sutype::satindex_t satindex = wire->satindex();
            SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");

            if (suSatSolverWrapper::instance()->is_constant (1, satindex)) {
              satindicesOfATrack.clear ();
              break;
            }
            else {
              satindicesOfATrack.push_back (satindex);
            }
          }
          
          if (satindicesOfATrack.empty()) {
            suClauseBank::return_clause (satindicesOfATrack);
            continue;
          }

          sutype::satindex_t out = suSatSolverWrapper::instance()->emit_OR_or_return_constant (satindicesOfATrack);
          
          if (!suSatSolverWrapper::instance()->is_constant (out)) {
            clause.push_back (out);
          }
          
          suClauseBank::return_clause (satindicesOfATrack);
        }
      }
    }

    if (!clause.empty()) {
      suWire * wire = wires.front();
      const suNet * net = wire->net();
      const suLayer * layer = wire->layer()->base();
      SUINFO(1) << "Minimize the number of " << clause.size() << " tracks for layer " << layer->name() << " and net " << net->name() << std::endl;
      if (numberOfWiresWeMayLeave >= 0 && numberOfWiresWeMayLeave >= (int)clause.size()) {
        SUINFO(1) << "Skipped as trivial; may leave " << numberOfWiresWeMayLeave << " wire(s)." << std::endl;
      }
      else {
        suSatSolverWrapper::instance()->optimize_satindices (clause, sutype::om_minimize, numberOfWiresWeMayLeave);
      }
    }
    
    suClauseBank::return_clause (clause);

    if (fixWiresOnFly) {
      post_routing_fix_wires (wires);
    }
    
  } // end of suSatRouter::optimize_the_number_of_tracks_of_layered_wires_

  //
  void suSatRouter::minimize_the_number_of_preroute_extensions_ ()
    const
  {
    for (const auto & iter1 : _ties) {

      sutype::id_t netid = iter1.first;
      const suNet * net = suCellManager::instance()->get_net_by_id (netid);
      SUASSERT (net, "");

      const auto & netties = iter1.second;

      std::set<suWire *, suWire::cmp_ptr> badwires;
      
      for (const auto & iter2 : netties) {

        suTie * tie = iter2;
        SUASSERT (tie->net() == net, "");
        
        const sutype::routes_t & routes = tie->routes();

        for (const auto & iter3 : routes) {

          suRoute * route = iter3;
          const sutype::wires_t & wires = route->wires();
          SUASSERT (wires.size() >= 2, "");

          suWire * wire0 = wires[0];
          suWire * wire1 = wires[1];

          if (!wire0->is (sutype::wt_preroute) && !wire1->is (sutype::wt_preroute)) continue; // nothing to optimize
          
          for (sutype::uvi_t w = 2; w < wires.size(); ++w) {
            
            suWire * wire2 = wires[w];

            if (!wire2->is (sutype::wt_route)) continue;

            if (wire0->is (sutype::wt_preroute) && wire2->layer() == wire0->layer() && wire2->rect().has_at_least_common_point (wire0->rect())) {

              // good I-route
              if (wire2->layer() == wire1->layer() && wire2->rect().has_at_least_common_point (wire1->rect())) {
                // do nothing
              }
              else {
                badwires.insert (wire2);
              }

              continue;
            }

            if (wire1->is (sutype::wt_preroute) && wire2->layer() == wire1->layer() && wire2->rect().has_at_least_common_point (wire1->rect())) {

              // good I-route
              if (wire2->layer() == wire0->layer() && wire2->rect().has_at_least_common_point (wire0->rect())) {
                // do nothing
              }
              else {
                badwires.insert (wire2);
              }

              continue;
            }
          }
        }
      }

      if (badwires.empty()) continue;

      std::set<const suLayer *, suLayer::cmp_const_ptr_by_level> layersInUse;
      for (const auto & iter2 : badwires) {
        suWire * wire = iter2;
        layersInUse.insert (wire->layer()->base());
      }
      
      for (const auto & iter2 : layersInUse) {

        const suLayer * baselayer = iter2;
        SUASSERT (baselayer->is_base(), "");

        sutype::clause_t & clause = suClauseBank::loan_clause ();

        for (const auto & iter3 : badwires) {
          
          suWire * wire = iter3;
          if (wire->layer()->base() != baselayer) continue;

          clause.push_back (wire->satindex());
        }

        SUASSERT (!clause.empty(), "");
        
        const sutype::opt_mode_t optmode = sutype::om_minimize;
        
        SUINFO(1)
          << suStatic::opt_mode_2_str(optmode) << " the number of "<< clause.size() << " extensions of preroutes"
          << " for net " << net->name()
          << " for layer " << baselayer->name()
          << std::endl;
        
        suSatSolverWrapper::instance()->optimize_satindices (clause, optmode);
        
        suClauseBank::return_clause (clause);
        
      } // for(layersInUse)
      
    } // for(nets)
    
  } // end of suSatRouter::minimize_the_number_of_preroute_extensions_

  //
  void suSatRouter::optimize_the_number_of_connected_entities_ (int minLengthToConsider)
    const
  {
    std::map<sutype::svi_t,sutype::connectedentities_tc> cesPerLength;

    for (const auto & iter1 : _connectedEntities) {

      auto & objects = iter1.second;
      
      for (const auto & iter2 : objects) {
        
        const suConnectedEntity * ce = iter2;
      
        if (!ce->is (sutype::wt_trunk)) continue;

        const suGlobalRoute * trunkgr = ce->trunkGlobalRoute();
        SUASSERT (trunkgr, "");
        SUASSERT (trunkgr->bbox().w() == 0 || trunkgr->bbox().h() == 0, "");
        sutype::svi_t celength = trunkgr->bbox().w() ? trunkgr->bbox().w() : trunkgr->bbox().h();
        SUASSERT (celength > 0, "");

        cesPerLength [-celength].push_back (ce);
      }
    }

    // optimize the number of short connected entities
    for (const auto & iter1 : cesPerLength) {
      
      sutype::svi_t celength = -(iter1.first);
      const sutype::connectedentities_tc & ceOfTheSameLength = iter1.second;
      
      SUASSERT (celength > 0, "");
      SUASSERT (!ceOfTheSameLength.empty(), "");

      if (minLengthToConsider > 0 && celength < minLengthToConsider) continue;

      sutype::opt_mode_t defaultoptmode = sutype::om_maximize;

      // minimize short trunks
      if (0 && celength <= 1) {
        defaultoptmode = sutype::om_minimize;
      }

      sutype::clause_t & clause = suClauseBank::loan_clause ();

      for (const auto & iter2 : ceOfTheSameLength) {
          
        const suConnectedEntity * ce = iter2;
          
        const suGlobalRoute * gr = ce->trunkGlobalRoute();
        SUASSERT (gr, "");
        
        sutype::satindex_t satindex = ce->layoutFunc()->satindex();
        SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");
        clause.push_back (satindex);
      }
        
      if (!clause.empty()) {
        SUINFO(1) << suStatic::opt_mode_2_str(defaultoptmode) << " the number of "<< ceOfTheSameLength.size() << " trunks of length " << celength << std::endl;
        suSatSolverWrapper::instance()->optimize_satindices (clause, defaultoptmode);
      }
        
      suClauseBank::return_clause (clause);
    }
        
  } // end of suSatRouter::optimize_the_number_of_connected_entities_

  //
  void suSatRouter::optimize_the_number_of_bad_wires_ ()
    const
  {
    const bool np = false;

    const sutype::opt_mode_t optmode = sutype::om_minimize;
    //const suLayer * layerm1 = suLayerManager::instance()->get_lowest_routing_wire_layer();
    
    SUINFO(1)
      << "Optimize the number of bad wires"
      << "; optmode=" << suStatic::opt_mode_2_str (optmode)
      << std::endl;

    // bad wire touches at least two preroutes and overlaps at least one preroute
    // it means that these two preroutes could be connected directly; and this bad wire is just redundant
    std::map<const suNet *, sutype::wires_t, suNet::cmp_const_ptr> wiresPerNet0; // routes and shunts
    std::map<const suNet *, sutype::wires_t, suNet::cmp_const_ptr> wiresPerNet1; // preroutes

    for (sutype::uvi_t i=0; i < _satindexToPointer.size(); ++i) {
      
      suWire * wire = _satindexToPointer[i];
      if (!wire) continue;

      SUASSERT (wire->satindex(), "");
      SUASSERT (wire->satindex() > 0, "negative is not expected");

      const suNet * net = wire->net();

      if (wire->is (sutype::wt_route) || wire->is (sutype::wt_shunt)) {
        wiresPerNet0[net].push_back (wire);
      }
      
      if (wire->is (sutype::wt_preroute)) {
        wiresPerNet1[net].push_back (wire);
      }
    }

    for (const auto & iter1 : wiresPerNet0) {

      const suNet * net = iter1.first;
      if (wiresPerNet1.count (net) == 0) continue;

      const sutype::wires_t & wires0 = iter1.second;
      const sutype::wires_t & wires1 = wiresPerNet1.at (net);
      
      SUASSERT (!wires0.empty(), "");
      SUASSERT (!wires1.empty(), "");

      sutype::clause_t & clause = suClauseBank::loan_clause ();

      for (const auto & iter2 : wires0) {

        suWire * wire0 = iter2;
        SUASSERT (wire0->is (sutype::wt_route) || wire0->is (sutype::wt_shunt), "");
        const suRectangle & rect0 = wire0->rect();
        const suLayer * layer0 = wire0->layer();

        sutype::uvi_t numTouches  = 0;
        sutype::uvi_t numOverlaps = 0;

        for (const auto & iter3 : wires1) {

          suWire * wire1 = iter3;
          SUASSERT (wire1->is (sutype::wt_preroute), "");
          const suRectangle & rect1 = wire1->rect();
          const suLayer * layer1 = wire1->layer();

          if (layer1 != layer0) continue;

          if (rect1.has_at_least_common_point (rect0)) {
            SUINFO(np) << "Wires touch: wire0=" << wire0->to_str() << "; wire1=" << wire1->to_str() << std::endl;
            ++numTouches;
          }
          
          if (rect1.has_a_common_rect (rect0)) {
            SUINFO(np) << "Wires overlap: wire0=" << wire0->to_str() << "; wire1=" << wire1->to_str() << std::endl;
            ++numOverlaps;
          }
        }

        // bad wire wire0 touches at least two preroutes and overlaps at least one preroute
        // it means that these two preroutes could be connected directly; and this bad wire is just redundant
        if (numTouches < 2) continue;
        if (numOverlaps < 1) continue;

        SUINFO(np) << "Found bad wire; numTouches=" << numTouches << "; numOverlaps=" << numOverlaps << "; wire0=" << wire0->to_str();
        
        clause.push_back (wire0->satindex());
      }

      if (!clause.empty()) {
        SUINFO(1) << suStatic::opt_mode_2_str(optmode) << " the number of "<< clause.size() << " bad wires for net " << net->name() << std::endl;
        suSatSolverWrapper::instance()->optimize_satindices (clause, optmode);
      }
      
      suClauseBank::return_clause (clause);
    }
    
  } // end of suSatRouter::optimize_the_number_of_bad_wires_
  
  //
  void suSatRouter::optimize_the_number_of_ties_ (unsigned targetNumConnectedEntitiesWithTrunks,
                                                  sutype::opt_mode_t optmode,
                                                  int minlengthtoconsider, // works only for trunks
                                                  int maxlengthtoconsider)
    const
  {
    SUINFO(1)
      << "Optimize the number of ties: targetNumConnectedEntitiesWithTrunks=" << targetNumConnectedEntitiesWithTrunks
      << "; optmode=" << suStatic::opt_mode_2_str (optmode)
      << std::endl;

    // net by net
    for (const auto & iter1 : _ties) {
      
      const sutype::id_t netid = iter1.first;
      const suNet * net = suCellManager::instance()->get_net_by_id (netid);
      const auto & netties = iter1.second;
       
      std::map<sutype::uvi_t,sutype::clause_t> tiesPerLayerDiff;
      sutype::uvi_t count = 0;
      
      for (const auto & iter2 : netties) {

        const suTie * tie = iter2;
        SUASSERT (tie->net() == net, "");

        if (tie->routes().empty()) continue;
        
        unsigned numConnectedEntitiesWithTrunks = tie->calculate_the_number_of_connected_entities_of_a_particular_type (sutype::wt_trunk);
        if (numConnectedEntitiesWithTrunks != targetNumConnectedEntitiesWithTrunks) continue;
        
        //SUINFO(1) << "Found a tie " << tie->to_str() << std::endl;

        if (minlengthtoconsider > 0) {
          if (tie->entity0()->trunkGlobalRoute() && tie->entity0()->trunkGlobalRoute()->length_in_regions() < minlengthtoconsider) continue;
          if (tie->entity1()->trunkGlobalRoute() && tie->entity1()->trunkGlobalRoute()->length_in_regions() < minlengthtoconsider) continue;
        }

        if (maxlengthtoconsider > 0) {
          if (tie->entity0()->trunkGlobalRoute() && tie->entity0()->trunkGlobalRoute()->length_in_regions() > maxlengthtoconsider) continue;
          if (tie->entity1()->trunkGlobalRoute() && tie->entity1()->trunkGlobalRoute()->length_in_regions() > maxlengthtoconsider) continue;
        }
        
        sutype::uvi_t layerdiff = 0;

        if (1) {
          // first two wires are input wires that this route connects
          suRoute * route = tie->routes().front();
          const sutype::wires_t & wires = route->wires();
          SUASSERT (wires.size() >= 2, "");
          suWire * wire0 = wires[0];
          suWire * wire1 = wires[1];
          layerdiff = abs (wire0->layer()->level() - wire1->layer()->level());
          ++count;
        }
        
        tiesPerLayerDiff [layerdiff].push_back (tie->satindex());        
      }
      
      SUINFO(1)
        << "Found " << count << " sat indices we want to " << suStatic::opt_mode_2_str (optmode)
        << "; net=" << net->name()
        << std::endl;

      if (count == 0) continue;

      std::vector<sutype::uvi_t> layerdiffs;

      for (const auto & iter : tiesPerLayerDiff) {
        sutype::uvi_t layerdiff = iter.first;
        layerdiffs.push_back (layerdiff);
      }

      // default
      sutype::svi_t i0 = 0;
      sutype::svi_t incr = 1;

      // if mode == sutype::om_minimize then minimize ties with the maximal level diff first
      // if mode == sutype::om_maximize then maximize ties with the minimal level diff first
      if (optmode == sutype::om_minimize) {
        i0 = (sutype::svi_t)layerdiffs.size() - 1;
        incr = -1;
      }
      
      // if mode == sutype::om_minimize then minimize ties with the maximal level diff first
      // if mode == sutype::om_maximize then maximize ties with the minimal level diff first
      for (sutype::svi_t i = i0; i >= 0 && i < (sutype::svi_t)layerdiffs.size(); i += incr) {
        sutype::uvi_t layerdiff = layerdiffs[i];
        const sutype::clause_t & clause = tiesPerLayerDiff.at (layerdiff);
        SUINFO(1)
          << suStatic::opt_mode_2_str (optmode) << " " << clause.size() << " ties with layerdiff=" << layerdiff
          << "; net=" << net->name()
          << std::endl;
        suSatSolverWrapper::instance()->optimize_satindices (clause, optmode);
      }
    }

    for (const auto & iter1 : _ties) {

      break; // do not print ties

      const sutype::id_t netid = iter1.first;
      const suNet * net = suCellManager::instance()->get_net_by_id (netid);
      const auto & netties = iter1.second;

      SUINFO(1) << "Applied ties of net " << net->name() << ":" << std::endl;

      if (!suSatSolverWrapper::instance()->model_is_valid()) {
        SUISSUE("Model is invalid. Cannot print applied ties.") << std::endl;
        continue;
      }

      for (const auto & iter2 : netties) {

        suTie * tie = iter2;
        sutype::satindex_t tiesatindex = tie->satindex();
        SUASSERT (tiesatindex != sutype::UNDEFINED_SAT_INDEX, "");
        
        if (suSatSolverWrapper::instance()->get_modeled_value (tiesatindex) != sutype::bool_true) continue;
        SUINFO(1)
          << "  "
          << targetNumConnectedEntitiesWithTrunks
          << "_" << suStatic::opt_mode_2_str (optmode) << " "
          << tie->to_str() << std::endl;
      }
    }
    
  } // end of suSatRouter::optimize_the_number_of_ties_

  //
  void suSatRouter::optimize_wire_widths_ (sutype::wire_type_t targetwiretype)
    const
  {
    std::map<const suLayer *, std::map<sutype::dcoord_t, std::map<sutype::dcoord_t, std::map<const suNet *, sutype::wires_t, suNet::cmp_const_ptr> > >, suLayer::cmp_const_ptr_by_level> wiresPerLayerPerLengthPerWidthPerNet;
    
    std::map<const suLayer *, sutype::dcoord_t, suLayer::cmp_const_ptr_by_level> maxWidthOfLayers;

    for (sutype::uvi_t i=0; i < _satindexToPointer.size(); ++i) {
      
      suWire * wire = _satindexToPointer[i];
      if (!wire) continue;
      if (!wire->is (targetwiretype)) continue;

      sutype::satindex_t satindex = wire->satindex();
      if (suSatSolverWrapper::instance()->is_constant (satindex)) continue; // nothing to optimize
      
      const suLayer * layer = wire->layer()->base();
      if (!layer->is (sutype::lt_wire)) continue;
      
      sutype::dcoord_t length = wire->length();
      sutype::dcoord_t width  = wire->width();
      const suNet *    net    = wire->net();

      // optimize lonf wires first
      wiresPerLayerPerLengthPerWidthPerNet [layer] [-length] [width] [net].push_back (wire);

      if (maxWidthOfLayers.count(layer) == 0 || maxWidthOfLayers.at(layer) < width)
        maxWidthOfLayers[layer] = width;
    }

    for (const auto & iter0 : maxWidthOfLayers) {
      
      const suLayer * layer = iter0.first;
      const sutype::dcoord_t maxWidthOfThisLayer = iter0.second;
      SUINFO(1) << "Layer " << layer->name() << "; maximal wire width: " << maxWidthOfThisLayer << std::endl;
    }
    
    // optimize long wires first
    for (const auto & iter0 : wiresPerLayerPerLengthPerWidthPerNet) {

      const suLayer * layer   = iter0.first;
      const auto & container1 = iter0.second;
      
      const sutype::dcoord_t maxWidthOfThisLayer = maxWidthOfLayers[layer];
      
      for (const auto & iter1 : container1) {
        
        sutype::dcoord_t length = -iter1.first;
        const auto & container2 =  iter1.second;

        //sutype::uvi_t index = 0;

        for (const auto & iter2 : container2) {

          sutype::dcoord_t width  = iter2.first;
          const auto & container3 = iter2.second;

          // do not constrain maximal wire width
          if (width == maxWidthOfThisLayer) {
            SUINFO(1)
              << "Do not minimize the number of " << suStatic::wire_type_2_str (targetwiretype) << " wires"
              << "; layer=" << layer->name()
              << "; length=" << length
              << "; width=" << width
              << std::endl;
            break;
          }
        
          for (const auto & iter3 : container3) {

            const suNet * net       = iter3.first;
            const auto & container4 = iter3.second; // wires

            sutype::clause_t & clause = suClauseBank::loan_clause();
          
            for (const auto & iter4 : container4) {

              suWire * wire = iter4;
              sutype::satindex_t satindex = wire->satindex();
              SUASSERT (satindex > 0, "");
              clause.push_back (satindex);
            }

            SUINFO(1)
              << "Minimize the number of " << clause.size() << " " << suStatic::wire_type_2_str (targetwiretype) << " wires"
              << "; layer=" << layer->name()
              << "; length=" << length
              << "; width=" << width
              << "; net=" << net->name()
              << std::endl;
          
            suSatSolverWrapper::instance()->optimize_satindices (clause, sutype::om_minimize);
          
            suClauseBank::return_clause (clause);
          }
        }
      }
    }
    
  } // end of suSatRouter::optimize_wire_widths_ 

  //
  void suSatRouter::optimize_wire_lengths_ (sutype::wire_type_t targetwiretype)
    const
  {
    std::map<const suLayer *, std::map<sutype::dcoord_t, std::map<const suNet *, sutype::wires_t, suNet::cmp_const_ptr> > , suLayer::cmp_const_ptr_by_level> wiresPerLayerPerLengthPerNet;
    
    for (sutype::uvi_t i=0; i < _satindexToPointer.size(); ++i) {
      
      suWire * wire = _satindexToPointer[i];
      if (!wire) continue;
      if (!wire->is (targetwiretype)) continue;

      sutype::satindex_t satindex = wire->satindex();
      if (suSatSolverWrapper::instance()->is_constant (satindex)) continue; // nothing to optimize
      
      const suLayer * layer = wire->layer()->base();
      if (!layer->is (sutype::lt_wire)) continue;

      sutype::dcoord_t length = wire->length();
      const suNet *    net    = wire->net();

      // minimize lonf wires first
      wiresPerLayerPerLengthPerNet [layer] [-length] [net].push_back (wire);
    }
    
    // optimize long wires first
    for (const auto & iter0 : wiresPerLayerPerLengthPerNet) {

      const suLayer * layer   = iter0.first;
      const auto & container1 = iter0.second;
            
      for (const auto & iter1 : container1) {
        
        sutype::dcoord_t length = -iter1.first;
        const auto & container2 =  iter1.second;
          
        for (const auto & iter2 : container2) {

          const suNet * net       = iter2.first;
          const auto & container3 = iter2.second; // wires

          sutype::clause_t & clause = suClauseBank::loan_clause();
          
          for (const auto & iter3 : container3) {

            suWire * wire = iter3;
            sutype::satindex_t satindex = wire->satindex();
            SUASSERT (satindex > 0, "");
            clause.push_back (satindex);
          }

          SUINFO(1)
            << "Minimize the number of " << clause.size() << " " << suStatic::wire_type_2_str (targetwiretype) << " wires"
            << "; layer=" << layer->name()
            << "; length=" << length
            << "; net=" << net->name()
            << std::endl;
          
          suSatSolverWrapper::instance()->optimize_satindices (clause, sutype::om_minimize);
          
          suClauseBank::return_clause (clause);
        }
      }
    }
    
  } // end of suSatRouter::optimize_wire_lengths_ 

  //
  void suSatRouter::optimize_connections_to_preroutes_ ()
    const
  {
    sutype::opt_mode_t optmode = sutype::om_minimize;

    std::map<suWire*, std::map<sutype::dcoord_t, sutype::wires_t>, suWire::cmp_ptr> connectionsToWires1; // initial calculations
    std::map<suWire*, std::map<sutype::dcoord_t, sutype::wires_t>, suWire::cmp_ptr> connectionsToWires2; // updated calculations
    
    // calculate connectionsToWires1
    for (const auto & iter1 : _ties) {
      
      const auto & ties = iter1.second;
      
      for (const auto & iter2 : ties) {

        suTie * tie = iter2;
        const sutype::routes_t & routes = tie->routes();

        for (const auto & iter3 : routes) {

          suRoute * route = iter3;
          const sutype::wires_t & wires = route->wires();
          
          SUASSERT (wires.size() >= 2, "");
          if (wires.size() <= 2) continue;
          
          for (sutype::uvi_t i=0; i < 2; ++i) {

            suWire * wire0 = wires[i];
            if (!wire0->is (sutype::wt_preroute)) continue;
            
            const suLayer * layer0 = wire0->layer();
            SUASSERT (layer0->has_pgd(), "");
            SUASSERT (layer0->is (sutype::lt_metal), "");
            SUASSERT (layer0->is (sutype::lt_wire), "");
            
            sutype::dcoord_t edgec0 = wire0->edgec(); // center of the wire; the best connection
            
            for (sutype::uvi_t k=2; k < wires.size(); ++k) {
              
              suWire * wire1 = wires[k];
              sutype::satindex_t satindex1 = wire1->satindex();
              SUASSERT (satindex1 != sutype::UNDEFINED_SAT_INDEX, "");
              if (suSatSolverWrapper::instance()->is_constant (satindex1)) continue; // nothing to optimize
              
              const suLayer * layer1 = wire1->layer();
              if (!suLayerManager::instance()->layers_are_adjacent_metal_layers (layer0, layer1)) continue;
              
              SUASSERT (layer1->has_pgd(), "");
              SUASSERT (layer1->is (sutype::lt_metal), "");
              SUASSERT (layer1->is (sutype::lt_wire), "");
              SUASSERT (layer1->pgd() != layer0->pgd(), "");
              
              if (wire1->edgel() > wire0->sideh()) continue;
              if (wire1->edgeh() < wire0->sidel()) continue;

              sutype::dcoord_t sidec1 = wire1->sidec(); // center line

              sutype::dcoord_t delta0 = edgec0 - sidec1; // a delta between wire0 and wire1
              
              connectionsToWires1[wire0][delta0].push_back (wire1);
            }
          }
        }
      }
    }

    std::set<sutype::dcoord_t> deltasReverseOrder;
    std::set<const suLayer *, suLayer::cmp_const_ptr_by_level> baseLayersSortedByLevel;
    
    for (const auto & iter0 : connectionsToWires1) {
      
      suWire * wire0 = iter0.first;
      const auto & container0 = iter0.second;

      baseLayersSortedByLevel.insert (wire0->layer()->base());
      
      SUINFO(1)
        << "Wire " << wire0->to_str() 
        << " has " << container0.size() << " potential connections."
        << std::endl;

      sutype::uvi_t numconnections = 0;
      
      for (const auto & iter1 : container0) {

        const sutype::wires_t & wires1 = iter1.second;

        for (const auto & iter2 : wires1) {

          suWire * wire1 = iter2;
          sutype::satindex_t satindex1 = wire1->satindex();
          if (suSatSolverWrapper::instance()->get_modeled_value (satindex1) == sutype::bool_true) {
            ++numconnections;
            break;
          }
        }
      }

      SUINFO(0)
        << "Wire " << wire0->to_str() 
        << ": " << container0.size() << " potential connections"
        << "; " << numconnections << " current connections" 
        << std::endl;
      
      // connectionsToWires2 is not implemented very well yet
      // I want to consider cases when a wire has to have 2+ connections and central connection (delta0) is not the optimal.
      for (const auto & iter1 : container0) {
        
        sutype::dcoord_t delta0 = iter1.first; // may be null, negative, or possible
        const sutype::wires_t & wires1 = iter1.second;
        
        sutype::dcoord_t absdelta0 = abs (delta0);

        deltasReverseOrder.insert (-absdelta0);

        for (const auto & iter2 : wires1) {
          
          suWire * wire1 = iter2;
          connectionsToWires2[wire0][absdelta0].push_back (wire1);
        }
      }
    }

    SUINFO(1) << "Found " << baseLayersSortedByLevel.size() << " base layers to optimize." << std::endl;
    SUINFO(1) << "Found " << deltasReverseOrder.size() << " deltas to optimize." << std::endl;

    for (const auto & iter0 : baseLayersSortedByLevel) {
      
      const suLayer * targetbaselayer = iter0;
      
      for (const auto & iter1 : _ties) {
        
        sutype::id_t targetnetid = iter1.first;
        const suNet * targetnet = suCellManager::instance()->get_net_by_id (targetnetid);
        SUASSERT (targetnet, "");
        
        for (const auto & iter2 : deltasReverseOrder) {

          sutype::dcoord_t targetdelta = iter2;
          SUASSERT (targetdelta <= 0, "");
          targetdelta = -targetdelta;

          sutype::clause_t & clause = suClauseBank::loan_clause();

          for (const auto & iter3 : connectionsToWires2) {

            suWire * wire0 = iter3.first;
            if (wire0->layer()->base() != targetbaselayer) continue;
            if (wire0->net() != targetnet) continue;

            const auto & container1 = iter3.second;
            sutype::uvi_t counter = 0;

            for (const auto & iter4 : container1) {

              ++counter;
            
              sutype::dcoord_t delta = iter4.first;
              SUASSERT (delta >= 0, "");
            
              if (delta != targetdelta) continue;
              if (counter == 1) continue; // this delta is a minimal possible delta for this wire0; nothing to optimize
            
              const auto & container2 = iter4.second;

              for (const auto & iter5 : container2) {

                suWire * wire1 = iter5;
                sutype::satindex_t satindex1 = wire1->satindex();
                SUASSERT (satindex1 != sutype::UNDEFINED_SAT_INDEX, "");
                if (suSatSolverWrapper::instance()->is_constant (satindex1)) continue; // nothing to optimize
                
                clause.push_back (satindex1);

              } // for(iter5)
            } // for(iter4)
          } // for(iter3)

          if (clause.empty()) {
            suClauseBank::return_clause (clause);
            continue;
          }

          SUINFO(1)
            << suStatic::opt_mode_2_str (optmode) << " " << clause.size() << " imperfect connections to preroutes"
            << "; net=" << targetnet->name()
            << "; baselayer=" << targetbaselayer->to_str()
            << "; delta=" << targetdelta
            << std::endl;
          
          suSatSolverWrapper::instance()->optimize_satindices (clause, optmode);

//           //
//           bool allSatindicesAreConstant = true;

//           for (const auto & iter3 : clause) {

//             sutype::satindex_t satindex = iter3;
//             if (!suSatSolverWrapper::instance()->is_constant (satindex)) {
//               allSatindicesAreConstant = false;
//               break;
//             }
//           } // for(iter3)
          
          suClauseBank::return_clause (clause);
          
        } // for(iter2/deltasReverseOrder)
      } // for(iter1/_ties)
    } // for(iter0/baseLayersSortedByLevel)
    
  } // end of suSatRouter::optimize_connections_to_preroutes_

  //
  void suSatRouter::optimize_the_number_of_routes_ (unsigned targetNumConnectedEntitiesWithTrunks,
                                                    sutype::opt_mode_t optmode)
    const
  {
    for (const auto & iter1 : _ties) {
      
      const auto & ties = iter1.second;
      
      for (const auto & iter2 : ties) {

        const suTie * tie = iter2;
        const sutype::routes_t & routes = tie->routes();
        if (routes.empty()) continue;

        sutype::satindex_t tiesatindex = tie->satindex();
        SUASSERT (tiesatindex != sutype::UNDEFINED_SAT_INDEX, "");

        unsigned numConnectedEntitiesWithTrunks = tie->calculate_the_number_of_connected_entities_of_a_particular_type (sutype::wt_trunk);
        if (numConnectedEntitiesWithTrunks != targetNumConnectedEntitiesWithTrunks) continue;
        
//         // do not optimize the number of routes if no routes were implemented till now
//         if (suSatSolverWrapper::instance()->get_modeled_value (tiesatindex) == sutype::bool_false) {
//           continue;
//         }
        
        sutype::clause_t & clause = suClauseBank::loan_clause ();

        sutype::uvi_t numAppliedRoutes1 = 0;

        for (const auto & iter3 : routes) {
          
          suRoute * route = iter3;
          suLayoutFunc * func = route->layoutFunc();
                    
          sutype::satindex_t routesatindex = func->satindex();
          
          if (suSatSolverWrapper::instance()->get_modeled_value (routesatindex) == sutype::bool_true)
            ++numAppliedRoutes1;

          if (suSatSolverWrapper::instance()->is_constant (routesatindex)) continue;
          
          clause.push_back (routesatindex);
        }
        
        if (!clause.empty()) {
          SUINFO(1) << "Optimize the number of routes for tie " << tie->to_str() << std::endl;
          suSatSolverWrapper::instance()->optimize_satindices (clause, optmode);
        }

        sutype::uvi_t numAppliedRoutes2 = 0;

        for (const auto & iter3 : routes) {
          const suRoute * route = iter3;
          const suLayoutFunc * func = route->layoutFunc();
          sutype::satindex_t routesatindex = func->satindex();
          if (suSatSolverWrapper::instance()->get_modeled_value (routesatindex) == sutype::bool_true)
            ++numAppliedRoutes2;
        }
        
        if (numAppliedRoutes2 != numAppliedRoutes1) {
          SUINFO(1)
            << "Tie " << tie->to_str()
            << " had " << numAppliedRoutes1 << " applied routes before optimization and now it has " << numAppliedRoutes2 << "."
            << std::endl;
        }
        
        suClauseBank::return_clause (clause);
      }
    }
    
  } // end of suSatRouter::optimize_the_number_of_routes_

  //
  void suSatRouter::optimize_generator_instances_by_coverage_ ()
    const
  {
    std::map<sutype::dcoord_t,sutype::generatorinstances_t> gisPerCoverage;
    
    for (const auto & iter0 : _generatorinstances) {
      const auto & container1 = iter0.second;
      for (const auto & iter1 : container1) {
        const auto & container2 = iter1.second;
        for (const auto & iter2 : container2) {
          const auto & container3 = iter2.second;
          for (const auto & iter3 : container3) {
            const auto & container4 = iter3.second;
            for (const auto & iter4 : container4) {
              const auto & container5 = iter4.second;
              for (const auto & iter5 : container5) {

                suGeneratorInstance * gi = iter5;
                if (!gi->legal()) continue;
          
                sutype::satindex_t satindex = gi->layoutFunc()->satindex();
                SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");
                if (suSatSolverWrapper::instance()->is_constant (satindex)) continue; // nothing to optimize
          
                sutype::dcoord_t coverage = gi->coverage();
                gisPerCoverage[coverage].push_back (gi);
              }
            }
          }
        }
      }
    }
    
    for (const auto & iter1 : gisPerCoverage) {
      
      sutype::dcoord_t coverage = iter1.first;
      const sutype::generatorinstances_t & gis = iter1.second;

      if (coverage >= 100) continue;

      sutype::clause_t & clause = suClauseBank::loan_clause ();

      // fill clause by generator instances
      if (0) {
        
        for (const auto & iter2 : gis) {
        
          const suGeneratorInstance * gi = iter2;
          const suLayoutFunc * layoutfunc = gi->layoutFunc();
          sutype::satindex_t satindex = layoutfunc->satindex();
          SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");
          SUASSERT (satindex > 0, "");

          clause.push_back (satindex);
        }
      }

      // group generator instances by generator and location; create an extra SAT variable
      // fill clause by these groups
      else {

        std::map <std::tuple <sutype::id_t, sutype::dcoord_t, sutype::dcoord_t>, sutype::generatorinstances_t> netAgnosticGis;

        for (const auto & iter2 : gis) {

          suGeneratorInstance * gi = iter2;
          sutype::id_t id = gi->generator()->id();
          sutype::dcoord_t x = gi->point().x();
          sutype::dcoord_t y = gi->point().y();

          netAgnosticGis [std::tuple <sutype::id_t, sutype::dcoord_t, sutype::dcoord_t> (id, x, y)].push_back (gi);
        }

        for (const auto & iter2: netAgnosticGis) {

          const sutype::generatorinstances_t & gis2 = iter2.second;
        
          if (gis2.size() == 1) {
            clause.push_back (gis2.front()->layoutFunc()->satindex());
            continue;
          }

          sutype::clause_t & clause2 = suClauseBank::loan_clause ();
          for (const auto & iter3 : gis2) {
            suGeneratorInstance * gi = iter3;
            clause2.push_back (gi->layoutFunc()->satindex());
          }
          clause.push_back (suSatSolverWrapper::instance()->emit_OR_or_return_constant (clause2));
          suClauseBank::return_clause (clause2);
        }
        
        //SUINFO(1) << "Num generator instances: " << gis.size() << "; num groups: " << netAgnosticGis.size() << std::endl;
      }
      
      SUINFO(1) << "Minimize generator instances with coverage " << coverage << std::endl;
      suSatSolverWrapper::instance()->optimize_satindices (clause, sutype::om_minimize);
      
      suClauseBank::return_clause (clause);
    }
    
  } // end of suSatRouter::optimize_generator_instances_by_coverage_

  //
  sutype::routes_t suSatRouter::get_routes_ ()
    const
  {
    sutype::routes_t routes;

    for (const auto & iter1 : _ties) {

      const auto & netties = iter1.second;
      
      for (const auto & iter2 : netties) {

        const suTie * tie = iter2;
        const sutype::routes_t & tieroutes = tie->routes();

        routes.insert (routes.end(), tieroutes.begin(), tieroutes.end());
      }
    }

    return routes;
    
  } // end of suSatRouter::get_routes_
  
  // debug only
  void suSatRouter::find_ties_having_a_route_with_this_wire_ (const suWire * wire,
                                                              sutype::ties_t & ties,
                                                              sutype::routes_t & routes)
    const
  {
    const suNet * net = wire->net();
    SUASSERT (_ties.count (net->id()) == 1, "");

    const sutype::ties_t & neties = _ties.at (net->id());
    
    for (const auto & iter1 : neties) {
      
      suTie * tie = iter1;
      const sutype::routes_t & tieroutes = tie->routes();

      for (const auto & iter2 : tieroutes) {

        suRoute * route = iter2;
        const sutype::wires_t & routewires = route->wires();

        for (const auto & iter3 : routewires) {

          suWire * routewire = iter3;
          if (routewire != wire) continue;
          
          ties.push_back (tie);
          routes.push_back (route);
          break;
        }
      }
    }
    
  } // end of suSatRouter::find_ties_having_a_route_with_this_wire_

  //
  bool suSatRouter::path_exists_between_two_connected_entities_ (const suConnectedEntity * ce1,
                                                                 const suConnectedEntity * ce2,
                                                                 bool checkRoutes,
                                                                 bool useTrunksOnly,
                                                                 suTie * tieToSkip)
    const
  {
    const bool np = false;
    
    SUASSERT (ce1, "");
    SUASSERT (ce2, "");
    SUASSERT (ce1 != ce2, "");
    SUASSERT (ce1->net(), "");
    SUASSERT (ce1->net() == ce2->net(), "");
    //SUASSERT (ce1->is (sutype::wt_preroute), "");
    //SUASSERT (ce2->is (sutype::wt_preroute), "");

    if (_ties.count (ce1->net()->id()) == 0) return false;
    
    sutype::ties_t ties = _ties.at (ce1->net()->id()); // a hard copy

    sutype::uvi_t counter = 0;

    for (const auto & iter : ties) {

      suTie * tie = iter;

      if (checkRoutes && tie->routes().empty()) continue;
      if (tieToSkip && tie == tieToSkip) continue;
      
      sutype::satindex_t openSatindex = tie->openSatindex();
      if (openSatindex != sutype::UNDEFINED_SAT_INDEX && suSatSolverWrapper::instance()->is_constant (1, openSatindex)) continue; // skip open tie
      
      ties[counter] = tie;
      ++counter;
    }
    
    if (counter != ties.size()) {
      ties.resize (counter);
    }

    SUINFO(np) << std::endl;
    SUINFO(np) << "Tie to skip: " << tieToSkip->to_str() << std::endl;
    
    // trim unfeasible trunk ties; some ties might become unfeasible after removal of tieToSkip
    trim_unfeasible_trunk_ties_ (ties);
    
    // trim more ties
    counter = 0;

    for (const auto & iter : ties) {

      suTie * tie = iter;
      
      bool skiptie = true;
      
      if (useTrunksOnly) {
        if (tie->has_entity(ce1) && tie->get_another_entity(ce1)->is (sutype::wt_trunk)) skiptie = false;
        if (tie->has_entity(ce2) && tie->get_another_entity(ce2)->is (sutype::wt_trunk)) skiptie = false;
        if (tie->entity0()->is (sutype::wt_trunk) && tie->entity1()->is (sutype::wt_trunk)) skiptie = false;
      }
      else {
        skiptie = false;
      }

      if (skiptie) continue;

      ties[counter] = tie;
      ++counter;
    }

    if (counter != ties.size()) {
      ties.resize (counter);
    }
        
    if (np) {
      SUINFO(np) << "Check is there's a path between: " << ce1->id() << "-" << ce2->id() << "; ce1=" << ce1->to_str() << "; ce2=" << ce2->to_str() << std::endl;
      SUINFO(np) << "There're " << ties.size() << " available ties." << std::endl;
      for (const auto & iter : ties) {
        suTie * tie = iter;
        SUINFO(np) << " -  " << tie->to_str() << std::endl;
      }
    }
     
    bool ret = can_reach_target_connected_entity_ (ce1,
                                                   ce2,
                                                   ties);
    
    if (ret) {
      SUINFO(np) << "Path exists" << std::endl;
      return true;
    }
    
    return false;
    
  } // end of suSatRouter::path_exists_between_two_connected_entities_

  //
  void suSatRouter::trim_unfeasible_trunk_ties_ (sutype::ties_t & ties)
    const
  {
    const bool np = false;

    while (1) {

      bool repeat = false;

      std::set<const suConnectedEntity *, suConnectedEntity::cmp_const_ptr> trunkces;

      for (const auto & iter0 : ties) {

        suTie * tie = iter0;
        SUINFO(np) << "Available tie: " << tie->to_str() << std::endl;
      
        const suConnectedEntity * ce0 = tie->entity0();
        const suConnectedEntity * ce1 = tie->entity1();

        if (ce0->is (sutype::wt_trunk)) trunkces.insert (ce0);
        if (ce1->is (sutype::wt_trunk)) trunkces.insert (ce1);
      }

      for (const auto & iter0 : trunkces) {

        const suConnectedEntity * ce = iter0;

        SUINFO(np) << "Check ce=" << ce->to_str() << std::endl;
        
        const sutype::wires_t & cewires = ce->interfaceWires();

        bool wiresAreUnfeasible = true;

        for (const auto & iter1 : cewires) {

          suWire * cewire = iter1;
          SUASSERT (cewire->is (sutype::wt_trunk), "");

          sutype::uvi_t numties = 0;
          
          for (const auto & iter2 : ties) {

            suTie * tie = iter2;
            const sutype::routes_t & routes = tie->routes();
            
            for (const auto & iter3 : routes) {

              suRoute * route = iter3;
              const sutype::wires_t & routewires = route->wires();
              SUASSERT (routewires.size() >= 2, "");

              suWire * routewire0 = routewires[0];
              suWire * routewire1 = routewires[1];

              SUASSERT (routewire0 != routewire1, "");
            
              if (routewire0 == cewire || routewire1 == cewire) {
                ++numties;
                break;
              }
            }
            
            if (numties >= 2) {
              wiresAreUnfeasible = false;
              break;
            }
          }
          if (!wiresAreUnfeasible) break;
        }

        if (!wiresAreUnfeasible) continue;

        SUINFO(np) << "Found unfeasible entity: " << ce->to_str() << std::endl;
        
        for (sutype::svi_t i=0; i < (sutype::svi_t)ties.size(); ++i) {

          suTie * tie = ties[i];
          if (tie->entity0() != ce && tie->entity1() != ce) continue;

          ties[i] = ties.back();
          ties.pop_back();
          --i;
          repeat = true;
        }
      }

      if (!repeat) break;
    }
    
  } // end of suSatRouter::trim_unfeasible_trunk_ties_
  
  //
  bool suSatRouter::can_reach_target_connected_entity_ (const suConnectedEntity * currentce,
                                                        const suConnectedEntity * targetce,
                                                        sutype::ties_t & ties)
    const
  {
    const bool np = false;

    for (sutype::svi_t i=0; i < (sutype::svi_t)ties.size(); ++i) {

      suTie * tie = ties[i];
      
      if (!tie->has_entity (currentce)) continue;
      
      if (tie->has_entity (targetce)) {
        SUINFO(np) << " o " << tie->to_str() << std::endl;
        return true;
      }
      
      ties[i] = ties.back();
      ties.pop_back();
      --i;

      bool ret = can_reach_target_connected_entity_ (tie->get_another_entity (currentce),
                                                     targetce,
                                                     ties);

      if (ret) {
        SUINFO(np) << " o " << tie->to_str() << std::endl;
      }
      
      if (ret) return true;
    }

    return false;
    
  } // end of suSatRouter::can_reach_target_connected_entity_

  //
  void suSatRouter::update_affected_routes_ (std::map<sutype::satindex_t,sutype::routes_t> & satindexToRoutes)
    const
  {
    sutype::clause_t & targetsatindices = suClauseBank::loan_clause ();
    
    for (const auto & iter1 : satindexToRoutes) {

      sutype::satindex_t satindex = iter1.first;
      if (!suSatSolverWrapper::instance()->is_constant (satindex)) continue;
      
      targetsatindices.push_back (satindex);
    }

    if (!targetsatindices.empty()) {
      update_affected_routes_ (satindexToRoutes, targetsatindices);
    }
    
    suClauseBank::return_clause (targetsatindices);
    
  } // end of suSatRouter::update_affected_routes_

  //
  void suSatRouter::update_affected_routes_ (std::map<sutype::satindex_t,sutype::routes_t> & satindexToRoutes,
                                             sutype::satindex_t targetsatindex)
    const
  {
    sutype::clause_t & targetsatindices = suClauseBank::loan_clause ();

    targetsatindices.push_back (targetsatindex);

    update_affected_routes_ (satindexToRoutes, targetsatindices);
    
    suClauseBank::return_clause (targetsatindices);
    
  } // end of suSatRouter::update_affected_routes_

  //
  void suSatRouter::update_affected_routes_ (std::map<sutype::satindex_t,sutype::routes_t> & satindexToRoutes,
                                             const sutype::clause_t & targetsatindices)
    const
  {
    const bool np = false;

    SUASSERT (!targetsatindices.empty(), "");

    std::set<suRoute*> unuqueroutes;
    sutype::routes_t routes;

    for (const auto & iter1 : targetsatindices) {

      sutype::satindex_t satindex = iter1;
      SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");
      SUASSERT (suSatSolverWrapper::instance()->is_constant (satindex), "");

      satindex = abs (satindex);
      SUASSERT (satindex > 0, "");
      
      if (satindexToRoutes.count (satindex) == 0) continue;
      
      const sutype::routes_t & affectedroutes = satindexToRoutes.at (satindex);

      for (const auto & iter2 : affectedroutes) {
        suRoute * route = iter2;
        if (unuqueroutes.count (route) > 0) continue;
        unuqueroutes.insert (route);
        routes.push_back (route);
      }

      satindexToRoutes.erase (satindex);
    }

    SUINFO(np) << "Found " << routes.size() << " affected routes." << std::endl;

    for (const auto & iter1 : routes) {

      suRoute * route = iter1;

      if (_option_simplify_routes) {
        route->layoutFunc()->simplify();
      }
      
      route->release_useless_wires ();
    }
    
  } // end of suSatRouter::update_affected_routes_

  //
  void suSatRouter::create_new_global_routes_ (suGlobalRoute * gr0,
                                               bool upper,
                                               sutype::globalroutes_t & grs,
                                               int depth)
    const
  {    
    suGlobalRoute * gr1 = gr0->create_light_clone (); // copy only net, layer, and bbox
    
    bool layerchanged = gr1->change_layer (upper);
    if (!layerchanged) {
      delete gr1;
      return;
    }
    
    // recursion
    if (depth < 0 || depth > 0) {
      create_new_global_routes_ (gr1, upper, grs, depth-1);
    }
    
    for (const auto & iter : grs) {

      suGlobalRoute * gr2 = iter;
      
      if (gr2->net() != gr1->net()) continue;
      if (gr2->layer()->base() != gr1->layer()->base()) continue;
      if (gr2->bbox() != gr1->bbox()) continue;

      // delete gr1; it's not unique
      delete gr1;
      return;
    }

    SUINFO(1)
      << "Created a new incremental global route"
      << "; oldgr=" << gr0->to_str()
      << ": newgr=" << gr1->to_str()
      << std::endl;    
    
    grs.push_back (gr1);
    
  } // end of suSatRouter::create_new_global_routes_
  
} // end of namespace amsr

// end of suSatRouter.cpp
