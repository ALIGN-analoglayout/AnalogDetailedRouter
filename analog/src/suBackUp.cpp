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
//! \date   Mon Oct 16 09:55:22 2017

//! \file   suBackUp.cpp
//! \brief  Here I collect obsolete code which I want to save to keep some learnings. The code is disabled.

// std includes
#include <algorithm>
#include <iostream>
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

// module include

namespace amsr
{

#if 0

  //
  void suSatRouter::create_connected_entities (const sutype::tokens_t & cetokens)
  {
    const bool option_allow_opens = suOptionManager::instance()->get_boolean_option ("allow_opens", false);    
    const sutype::nets_tc & netsToRoute = suCellManager::instance()->netsToRoute();
    
    for (sutype::uvi_t i=0; i < cetokens.size(); ++i) {
      
      const suToken * token = cetokens[i];
      SUASSERT (token->is_defined ("terms"), "Connected Entity has no terminals");
      
      sutype::wires_t wires = suWireManager::instance()->get_wires_by_permanent_gid (token->get_list_of_integers ("terms"), "Connected entity");
      if (wires.empty()) {
        SUISSUE("Connected entity has no wires. Skipped.") << std::endl;
        continue;
      }

      const suNet * cenet = 0;
      const suLayer * ceinterfacelayer = 0;

      for (const auto & iter : wires) {
        
        suWire * wire = iter;
        SUASSERT (wire->is (sutype::wt_preroute), "");

        const suNet * wirenet = wire->net();
        const suLayer * wirelayer = wire->layer()->base();
        
        if (cenet == 0) cenet = wirenet;
        SUASSERT (cenet, "");
        SUASSERT (cenet == wirenet, "Wires of a connected entity belong to different nets: " << cenet->name() << " and " << wirenet->name());
        
        if (ceinterfacelayer == 0) ceinterfacelayer = wirelayer;
        SUASSERT (ceinterfacelayer, "");
        SUASSERT (ceinterfacelayer == wirelayer, "Wires of a connected entity belong to different layers: " << ceinterfacelayer->to_str() << " and " << wirelayer->to_str() << "; it's OK but unexpected; aborted just in case; need review");
      }
      
      SUASSERT (ceinterfacelayer, "");
      SUASSERT (cenet, "");
      SUASSERT (std::find (netsToRoute.begin(), netsToRoute.end(), cenet) != netsToRoute.end(), "cetokens should be trimmed already");
      
      suConnectedEntity * connectedentity = new suConnectedEntity (cenet);
      connectedentity->add_type (sutype::wt_preroute);
      
      connectedentity->set_interface_layer (ceinterfacelayer);
      
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
      
      for (const auto & iter1 : wires) {
        suWire * wire = iter1;
        SUASSERT (wire->satindex() != sutype::UNDEFINED_SAT_INDEX, wire->to_str());
        connectedentity->add_wire (wire);
        connectedentity->add_interface_wire (wire);
        layoutfunc0->add_leaf (wire->satindex());
      }
     
      // read mesh routes (not used now)
      // old implementation was moved to suBackUp.cpp
      sutype::tokens_t tokens2 = token->get_tokens ("Route");

      for (sutype::uvi_t k=0; k < tokens2.size(); ++k) {
        
        const suToken * token2 = tokens2[k];

        const std::string & layername = token2->get_string_value ("layer");
        const suLayer * layer = suLayerManager::instance()->get_base_layer_by_name (layername);

        std::vector<int> widths = token2->get_list_of_integers ("widths");
        SUASSERT (widths.size() == 1, "Another set of widths is not supported yet.");

        sutype::dcoord_t minwirewidth = widths.front();
        SUASSERT (minwirewidth > 0, "");

        suGlobalRoute * gr = new suGlobalRoute (cenet, layer, minwirewidth);
        connectedentity->add_mesh_global_route (gr);
      }

      add_connected_entity_ (connectedentity);
    }
    
  } // end of suSatRouter::create_connected_entities

  //
  void suSatRouter::create_mesh_routing_for_connected_entities ()
  {
    for (const auto & iter1 : _connectedEntities) {
      
      auto & ces = iter1.second;
      
      for (sutype::uvi_t i=0; i < ces.size(); ++i) {

        suConnectedEntity * ce = ces[i];
        create_mesh_routing_for_connected_entity_ (ce);
      }
    }
    
  } // end of suSatRouter::create_mesh_routing_for_connected_entities

  //
  void suSatRouter::create_mesh_routing_for_connected_entity_ (suConnectedEntity * ce)
  {
    if (!ce->is (sutype::wt_preroute)) return;

    const sutype::wires_t & originalcewires = ce->wires();
    SUASSERT (!originalcewires.empty(), "Connected entity has no wires: " << ce->to_str());

    if (originalcewires.size() == 1) {
      SUASSERT (ce->get_interface_layer() == 0, "");
      SUASSERT (ce->interfaceWires().empty(), "");
      ce->set_interface_layer (originalcewires.front()->layer()->base());
      ce->interface_layer_can_be_extended (!originalcewires.front()->layer()->fixed()); // maybe sometimes we do not want to extend pre-routes
      ce->add_interface_wire (originalcewires.front());
      return;
    }

    const suNet * net = ce->net();
    SUASSERT (net, "");
    
    // merge collinear wires
    const suLayer * commonlayer = 0;

    std::map<sutype::dcoord_t,sutype::wires_t> wiresPerSidel;

    for (const auto & iter : originalcewires) {

      suWire * wire = iter;
      const suLayer * layer = wire->layer();

      if (commonlayer == 0) {
        commonlayer = layer;
      }

      SUASSERT (commonlayer == layer, "Mesh routing: wires have different layers; it's not supported yet; wires: " << suStatic::to_str (originalcewires));

      wiresPerSidel[wire->sidel()].push_back (wire);
    }

    sutype::wires_t interfaceWires;

    // merge wires 
    for (const auto & iter : wiresPerSidel) {

      const sutype::wires_t & wires = iter.second;

      // nothing to merge
      if (wires.size() == 1) {
        suWire * mergedwire = suWireManager::instance()->create_wire_from_wire (net, wires.front(), sutype::wt_mesh); // single wire acts as a mesh wire
        interfaceWires.push_back (mergedwire);
        continue;
      }

      suWire * wire0 = wires[0];

      sutype::dcoord_t edgel = wire0->edgel();
      sutype::dcoord_t edgeh = wire0->edgeh();
      sutype::dcoord_t sidel = wire0->sidel();
      sutype::dcoord_t sideh = wire0->sideh();

      SUASSERT (sidel == iter.first, "");

      for (sutype::uvi_t i=1; i < wires.size(); ++i) {

        suWire * wire = wires[i];
        
        edgel = std::min (edgel, wire->edgel());
        edgeh = std::max (edgeh, wire->edgeh());
        sidel = std::min (sidel, wire->sidel());
        sideh = std::max (sideh, wire->sidel());
        
        SUASSERT (sidel == wire0->sidel(), "");
        SUASSERT (sideh == wire0->sideh(), "");
      }

      suWire * mergedwire = suWireManager::instance()->create_wire_from_edge_side (net, commonlayer, edgel, edgeh, sidel, sideh, sutype::wt_mesh); // mesh wire covers at least two wires
      SUASSERT (mergedwire->is (sutype::wt_mesh), "");
      
      SUINFO(1) << "Created a merged interface wire: " << mergedwire->to_str() << std::endl;
      interfaceWires.push_back (mergedwire);

      if (!wire_is_legal_slow_check (mergedwire)) {
        SUASSERT (false, "A wire that connectes adjacent terminal wires of a connected entity is illegal. Most likely there's an blockage between terminal wires. Split your connected entity onto two or several smaller connected entities.");
      }
    }

    SUINFO(1) << "Found " << interfaceWires.size() << " interface wires." << std::endl;

    suLayoutFunc * layoutfunc0 = ce->get_main_layout_function ();

    for (const auto & iter : interfaceWires) {
      
      suWire * mergedwire = iter;
      register_a_sat_wire (mergedwire);
      SUASSERT (mergedwire->satindex() != sutype::UNDEFINED_SAT_INDEX, "");
      SUINFO(1) << "An interface wire: " << mergedwire->to_str() << std::endl;
      
      layoutfunc0->add_leaf (mergedwire->satindex());
    }

    const sutype::globalroutes_tc & meshGlobalRoutes = ce->meshGlobalRoutes();
    
    // no mesh routes
    if (meshGlobalRoutes.empty()) {
      ce->set_interface_layer (commonlayer);
      ce->interface_layer_can_be_extended (!commonlayer->fixed()); // maybe sometimes we do not want to extend pre-routes
      for (const auto & iter : interfaceWires) {
        ce->add_interface_wire (iter);
      }
      return;
    }

    // can't create mesh
    if (interfaceWires.size() == 1) {
      ce->set_interface_layer (commonlayer);
      ce->interface_layer_can_be_extended (!commonlayer->fixed()); // maybe sometimes we do not want to extend pre-routes
      for (const auto & iter : interfaceWires) {
        ce->add_interface_wire (iter);
      }
      return;
    }

    // find mesh bounds
    bool needinit = true;
    sutype::dcoord_t edgel = 0;
    sutype::dcoord_t edgeh = 0;
    sutype::dcoord_t sidel = 0;
    sutype::dcoord_t sideh = 0;

    for (const auto & iter : interfaceWires) {

      const suWire * wire = iter;
            
      edgel = needinit ? wire->edgel() : std::min (edgel, wire->edgel());
      edgeh = needinit ? wire->edgeh() : std::max (edgeh, wire->edgeh());
      sidel = needinit ? wire->sidec() : std::min (sidel, wire->sidec()); // center line
      sideh = needinit ? wire->sidec() : std::max (sideh, wire->sidec()); // center line
      
      needinit = false;
    }

    const suGlobalRoute * gr = meshGlobalRoutes.front();
    const suLayer * adjacentMetalLayer = suLayerManager::instance()->get_base_adjacent_layer (commonlayer, sutype::lt_metal | sutype::lt_wire, true, 0);
    SUASSERT (adjacentMetalLayer, "Mesh routing: can't get upper adjacent metal layer for commonlayer=" << commonlayer->name() << "; wires: " << suStatic::to_str (originalcewires));
    SUASSERT (adjacentMetalLayer == gr->layer(), "Mesh routing: adjacent metal layer is expected to be " << gr->layer()->name() << " but got " << adjacentMetalLayer->name() << "; wires: " << suStatic::to_str (originalcewires));

    suRectangle meshrect;

    if (commonlayer->pgd() == sutype::go_ver) {
      meshrect.yl (edgel);
      meshrect.yh (edgeh);
      meshrect.xl (sidel);
      meshrect.xh (sideh);
    }
    else if (commonlayer->pgd() == sutype::go_hor) {
      meshrect.xl (edgel);
      meshrect.xh (edgeh);
      meshrect.yl (sidel);
      meshrect.yh (sideh);
    }
    else {
      SUASSERT (false, "");
    }

    const sutype::metaltemplateinstances_tc & mtis = suMetalTemplateManager::instance()->allMetalTemplateInstances ();    
    
    sutype::wires_t meshwires;
    sutype::uvi_t counter = 0;
    
    for (sutype::uvi_t m=0; m < mtis.size(); ++m) {
      const suMetalTemplateInstance * mti = mtis[m];       
      if (mti->metalTemplate()->baseLayer() != gr->layer()) continue;
      if (!mti->region().covers_compeletely (meshrect)) continue;
      SUASSERT (meshwires.empty(), "");
      mti->create_wires_in_rectangle (meshrect, net, sutype::wt_mesh, meshwires); // mesh wires
      SUASSERT (!meshwires.empty(), "");
    }

    for (sutype::uvi_t i=0; i < meshwires.size(); ++i) {
      
      suWire * meshwire = meshwires[i];
      SUASSERT (meshwire->layer()->base() == gr->layer(), "");

      bool skipmeshwire = true;
      
      for (const auto & iter2 : interfaceWires) {
        
        suWire * cewire = iter2;
        sutype::generatorinstances_t gis = get_via_generator_instances_ (meshwire, cewire);
        
        if (!gis.empty()) {
          skipmeshwire = false;
          break;
        }
      }
      
      if (skipmeshwire) {
        suWireManager::instance()->release_wire (meshwire);
        continue;
      }

      meshwires [counter] = meshwire;
      ++counter;
    }

    if (meshwires.size() != counter) {
      meshwires.resize (counter);
    }

    SUINFO(1) << "Found " << meshwires.size() << " feasible mesh wires." << std::endl;

    // no feasible mesh
    if (meshwires.empty()) {
      SUISSUE("Found no feasible mesh routes for a connected entity") << ": " << ce->to_str() << " with " << originalcewires.size() << " wires: " << suStatic::to_str(originalcewires) << std::endl;
      ce->set_interface_layer (commonlayer);
      ce->interface_layer_can_be_extended (!commonlayer->fixed()); // maybe sometimes we do not want to extend pre-routes
      for (const auto & iter : interfaceWires) {
        ce->add_interface_wire (iter);
      }
      return;
    }

    ce->set_interface_layer (meshwires.front()->layer());
    ce->interface_layer_can_be_extended (!meshwires.front()->layer()->fixed()); // maybe sometimes we do not want to extend pre-routes

    for (sutype::uvi_t i=0; i < meshwires.size(); ++i) {

      suWire * meshwire = meshwires[i];
      SUASSERT (meshwire->is (sutype::wt_mesh), "");
      
      SUINFO(1) << "Created a mesh wire " << meshwire->to_str() << std::endl;

      //ce->add_wire (meshwire);
      ce->add_interface_wire (meshwire);
      register_a_sat_wire (meshwire);
    }
    
    // minimal total trunk total width
    layoutfunc0->add_node (create_a_counter_for_total_wire_width_ (meshwires, gr->minWireWidth()));

    //
    sutype::clause_t tmpvector (meshwires.size(), sutype::UNDEFINED_SAT_INDEX);
    sutype::clauses_t viasTermToMesh (interfaceWires.size(), tmpvector);
    
    for (sutype::uvi_t i=0; i < interfaceWires.size(); ++i) {
      
      suWire * termwire = interfaceWires[i];
      SUASSERT (i < viasTermToMesh.size(), "");

      bool foundAtLeastOneVia = false;
      
      for (sutype::uvi_t k=0; k < meshwires.size(); ++k) {

        suWire * meshwire = meshwires[k];
        SUASSERT (k < viasTermToMesh[i].size(), "");

        sutype::generatorinstances_t gis = get_via_generator_instances_ (meshwire, termwire);
        if (gis.empty()) continue;

        foundAtLeastOneVia = true;

        suLayoutFunc * tmplayoutfunc = suLayoutNode::create_func (sutype::logic_func_or, sutype::unary_sign_just, SUFILELINE_DLF);

        for (const auto & iter : gis) {

          suGeneratorInstance * gi = iter;
          SUASSERT (gi->legal(), "");
          SUASSERT (gi->layoutFunc(), "");
          
          sutype::satindex_t satindex = gi->layoutFunc()->satindex();
          SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");
          
          tmplayoutfunc->add_leaf (satindex);
        }

        SUASSERT (!tmplayoutfunc->nodes().empty(), "");
        viasTermToMesh[i][k] = tmplayoutfunc->calculate_satindex();

        // don't need anymore
        tmplayoutfunc->delete_nodes ();
        suLayoutNode::delete_func (tmplayoutfunc);
      }

      if (!foundAtLeastOneVia) {
        SUISSUE("Could not connect wire to at least one mesh wire") << ": " << termwire->to_str() << std::endl;
      }
    }

    // every termwire must be connected to at least one meshwire
    for (sutype::uvi_t i=0; i < viasTermToMesh.size(); ++i) {
      
      suLayoutFunc * layoutfunc1 = 0;
      
      for (sutype::uvi_t k=0; k < viasTermToMesh[i].size(); ++k) {
        
        sutype::satindex_t satindex = viasTermToMesh[i][k];
        if (satindex == sutype::UNDEFINED_SAT_INDEX) continue;
        
        if (layoutfunc1 == 0) {
          layoutfunc1 = layoutfunc0->add_func (sutype::logic_func_or, sutype::unary_sign_just, SUFILELINE_DLF);
        }
        
        layoutfunc1->add_leaf (satindex);
      }

      if (layoutfunc1 == 0) {
        continue;
      }
    }
    
    // every mesh wire must be absent or to be connected to every available term wire
    for (sutype::uvi_t k=0; k < meshwires.size(); ++k) {

      suWire * meshwire = meshwires[k];
      
      suLayoutFunc * layoutfunc1 = layoutfunc0->add_func (sutype::logic_func_or, sutype::unary_sign_just, SUFILELINE_DLF);
      layoutfunc1->add_leaf (-meshwire->satindex());

      suLayoutFunc * layoutfunc2 = layoutfunc1->add_func (sutype::logic_func_and, sutype::unary_sign_just, SUFILELINE_DLF);

      for (sutype::uvi_t i=0; i < viasTermToMesh.size(); ++i) {

        sutype::satindex_t satindex = viasTermToMesh[i][k];
        if (satindex == sutype::UNDEFINED_SAT_INDEX) continue;
        
        layoutfunc2->add_leaf (satindex);
      }
    }
    
  } // end of suSatRouter::create_mesh_routing_for_connected_entity_

  //
  sutype::generatorinstances_t suSatRouter::get_via_generator_instances_ (const suWire * wire1,
                                                                          const suWire * wire2)
  {
    sutype::generatorinstances_t gis;
    
    const suNet * net1 = wire1->net();
    const suNet * net2 = wire2->net();
    
    SUASSERT (net1, "");
    SUASSERT (net2, "");
    SUASSERT (net1 == net2, "wire1=" << wire1->to_str() << "; wire2=" << wire2->to_str());
    
    const suNet * net = net1;
    
    const suLayer * layer1 = wire1->layer();
    const suLayer * layer2 = wire2->layer();
    
    SUASSERT (layer1->pgd() == sutype::go_ver || layer1->pgd() == sutype::go_hor, "");
    SUASSERT (layer2->pgd() == sutype::go_ver || layer2->pgd() == sutype::go_hor, "");
    SUASSERT (layer1->pgd() != layer2->pgd(), layer1->name() << "; " << layer2->name());

    sutype::dcoord_t centerdx = (layer1->pgd() == sutype::go_ver) ? wire1->sidec() : wire2->sidec();
    sutype::dcoord_t centerdy = (layer1->pgd() == sutype::go_ver) ? wire2->sidec() : wire1->sidec();
    
    sutype::dcoord_t width1 = wire1->width();
    sutype::dcoord_t width2 = wire2->width();

    SUASSERT (wire1->sidec() >= wire2->edgel() && wire1->sidec() <= wire2->edgeh(), "");
    SUASSERT (wire2->sidec() >= wire1->edgel() && wire2->sidec() <= wire1->edgeh(), "");

    const suLayer * cutlayer = suLayerManager::instance()->get_base_via_layer (layer1, layer2);
    SUASSERT (cutlayer, "wire1=" << wire1->to_str() << "; wire2=" << wire2->to_str());
    SUASSERT (cutlayer->is (sutype::lt_via), "");
    SUASSERT (cutlayer->is_base(), "");
    
    const sutype::generators_tc & generators = suGeneratorManager::instance()->get_generators (cutlayer);
    SUASSERT (!generators.empty(), "Can't find generators for a via");    
    
    for (const auto & iter1 : generators) {
      
      const suGenerator * generator = iter1;
      
      if (!generator->matches (layer1, layer2, width1, width2)) continue;
      
      suGeneratorInstance * generatorinstance = create_generator_instance_ (generator, net, centerdx, centerdy);
      SUASSERT (generatorinstance, "");
      
      // generator instance was created and then found (and marked) later as illegal
      if (!generatorinstance->legal()) continue;
      
      gis.push_back (generatorinstance);
    }

    if (gis.size() <= 1) return gis;
    
    // all generator instances are expected to have the same width or the same height
    sutype::dcoord_t commonw = -1;
    sutype::dcoord_t commonh = -1;
    sutype::dcoord_t maxw = -1;
    sutype::dcoord_t maxh = -1;
    bool gisHaveTheSameWidth  = true;
    bool gisHaveTheSameHeight = true;
    
    for (const auto & iter : gis) {

      suGeneratorInstance * gi = iter;
      
      sutype::dcoord_t w = gi->generator()->get_shape_width  ();
      sutype::dcoord_t h = gi->generator()->get_shape_height ();

      SUASSERT (w > 0, "");
      SUASSERT (h > 0, "");

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
    
    SUASSERT (gisHaveTheSameWidth || gisHaveTheSameHeight, "Unexpected. Wires have vias with arbitrary width and height: " << wire1->to_str() << "; wire2=" << wire2->to_str());

    // leave only gis with the maximal width/height
    if (1) {
      sutype::uvi_t counter = 0;

      for (sutype::uvi_t i=0; i < gis.size(); ++i) {

        suGeneratorInstance * gi = gis[i];
        if (gisHaveTheSameWidth  && gi->generator()->get_shape_height() < maxh) continue;
        if (gisHaveTheSameHeight && gi->generator()->get_shape_width () < maxw) continue;
        gis [counter] = gi;
        ++counter;
      }

      SUASSERT (counter > 0, "Unexpected. Wires don't have a via with the maximal width/height: " << wire1->to_str() << "; wire2=" << wire2->to_str() << "; gisHaveTheSameWidth=" << gisHaveTheSameWidth << "; gisHaveTheSameHeight=" << gisHaveTheSameHeight << "; commonw=" << commonw << "; commonh=" << commonh << "; maxw=" << maxw << "; maxh=" << maxh);

      if (counter != gis.size()) {
        gis.resize (counter);
      }
    }
    
    SUASSERT (!gis.empty(), "");

//     if (gis.size() != 1) {

//       for (const auto & iter : gis) {
//         suGeneratorInstance * gi = iter;
//         SUINFO(1) << gi->to_str() << std::endl;
//       }

//       SUASSERT (gis.size() == 1, "Expected only one generator instance but we have " << gis.size());
//     }

    return gis;
    
  } // end of suSatRouter::get_via_generator_instances_

  // simplified; just a wire per connected entity
  // not used not
  void suSatRouter::detect_connected_entities ()
  {
    const bool option_allow_opens = suOptionManager::instance()->get_boolean_option ("allow_opens", false);

    SUINFO(1)
      << "Detect connected entities"
      << "; allow_opens=" << option_allow_opens
      << std::endl;

    const sutype::nets_tc & netsToRoute = suCellManager::instance()->netsToRoute();
    
    for (sutype::uvi_t i=0; i < netsToRoute.size(); ++i) {
      
      const suNet * net = netsToRoute[i];
      SUINFO(1) << "Found net to route " << net->name() << " id=" << net->id() << std::endl;
      
      const sutype::wires_t & netwires = net->wires();

      const sutype::connectedentities_t & netces = _connectedEntities[net->id()];
      
      std::set<sutype::satindex_t> usedWires;
      for (const auto & iter1 : netces) {
        suConnectedEntity * ce = iter1;
        SUASSERT (ce->is (sutype::wt_preroute), "");
        const sutype::wires_t & cewires = ce->wires();
        for (const auto & iter2 : cewires) {
          suWire * wire = iter2;
          sutype::satindex_t satindex = wire->satindex();
          SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");
          if (usedWires.count (satindex) > 0) {
            SUISSUE("Wire is used in more than one connected entity") << ": " << wire->to_str() << std::endl;
            SUASSERT (false, "");
          }
          usedWires.insert (satindex);
        }
      }
      
      // detect wires those don't belong to any connected entity yet
      for (sutype::uvi_t k=0; k < netwires.size(); ++k) {

        suWire * wire = (suWire*)netwires[k];
        sutype::satindex_t satindex = wire->satindex();
        if (usedWires.count (satindex) > 0) continue;
        
        suConnectedEntity * connectedentity = new suConnectedEntity (net);
        
        //const sutype::satindex_t openSatindex = suSatSolverWrapper::instance()->get_constant(0);
        const sutype::satindex_t openSatindex = option_allow_opens ? suSatSolverWrapper::instance()->get_next_sat_index() : suSatSolverWrapper::instance()->get_constant(0);
        connectedentity->openSatindex (openSatindex);
        
        connectedentity->add_type (sutype::wt_preroute);
        connectedentity->set_interface_layer (wire->layer()->base());
        
        suLayoutFunc * layoutfuncCE = connectedentity->layoutFunc();
        SUASSERT (layoutfuncCE, "");
        SUASSERT (layoutfuncCE->nodes().empty(), "");
        layoutfuncCE->func (sutype::logic_func_or);
        layoutfuncCE->sign (sutype::unary_sign_just);
        
        layoutfuncCE->add_leaf (connectedentity->openSatindex());
        suLayoutFunc * layoutfunc0 = layoutfuncCE->add_func (sutype::logic_func_and, sutype::unary_sign_just, SUFILELINE_DLF);
        
        connectedentity->add_wire (wire);
        connectedentity->add_interface_wire (wire);
        
        layoutfunc0->add_leaf (satindex);
        
        add_connected_entity_ (connectedentity);
      }
    }
    
  } // end of suSatRouter::detect_connected_entities (not used now)

#endif // end of the root "if 0"


} // end of namespace amsr

// end of suBackUp.cpp
