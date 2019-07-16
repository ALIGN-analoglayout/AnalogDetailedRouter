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
//! \date   Tue Oct 10 14:00:01 2017

//! \file   suGlobalRouter.cpp
//! \brief  A collection of methods of the class suGlobalRouter.

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
#include <suGlobalRoute.h>
#include <suGREdge.h>
#include <suLayer.h>
#include <suLayerManager.h>
#include <suNet.h>
#include <suMetalTemplate.h>
#include <suMetalTemplateInstance.h>
#include <suMetalTemplateManager.h>
#include <suOptionManager.h>
#include <suToken.h>
#include <suTokenParser.h>
#include <suRectangle.h>
#include <suRegion.h>
#include <suStatic.h>
#include <suWire.h>
#include <suWireManager.h>

// module include
#include <suGlobalRouter.h>

namespace amsr
{
  //
  suGlobalRouter * suGlobalRouter::_instance = 0;
  
  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  //
  suGlobalRouter::~suGlobalRouter ()
  {
    for (sutype::uvi_t i=0; i < _idToRegion.size(); ++i)
      delete _idToRegion[i];

    for (sutype::uvi_t i=0; i < _globalRoutes.size(); ++i)
      delete _globalRoutes[i];

    for (sutype::uvi_t i=0; i < _gredges.size(); ++i)
      delete _gredges[i];

    if (_grTokenParser) delete _grTokenParser;
    
  } // end of suGlobalRouter::~suGlobalRouter

  // ------------------------------------------------------------
  // -
  // --- Public static methods
  // -
  // ------------------------------------------------------------

  // static
  void suGlobalRouter::delete_instance ()
  {
    if (suGlobalRouter::_instance)
      delete suGlobalRouter::_instance;

    suGlobalRouter::_instance = 0;
    
  } // end of suGlobalRouter::delete_instance
  
  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  // no much science for now
  void suGlobalRouter::create_global_routing_grid ()
  {
    create_regions_ ();

    create_edges_ ();

  } // end of suGlobalRouter::create_global_routing

  //
  void suGlobalRouter::read_global_routing ()
  {
    SUASSERT (_grTokenParser == 0, "Global routing has been read already");

    sutype::tokens_t tokens = suOptionManager::instance()->token()->get_tokens ("Option", "name", "global_routing_file");
        
    // read input files
    for (sutype::uvi_t i=0; i < tokens.size(); ++i) {

      const std::string & filename = tokens[i]->get_string_value ("value");
      
      if (_grTokenParser == 0) {
        _grTokenParser = new suTokenParser ();
      }

      bool ok = _grTokenParser->read_file (filename);
      SUASSERT (ok, "");
      SUASSERT (_grTokenParser->rootToken(), "");
    }
    
  } // end of suGlobalRouter::read_global_routing

  //
  sutype::ids_t suGlobalRouter::get_terminal_gids ()
    const
  {
    sutype::ids_t ids;

    if (_grTokenParser == 0) {
       
      SUISSUE("No global routing to parse")
        << std::endl;
      
      return ids;
    }

    sutype::tokens_t cetokens = _grTokenParser->rootToken()->get_tokens ("ConnectedEntity");

    std::set<sutype::id_t> gidsInUse;

    for (sutype::uvi_t i=0; i < cetokens.size(); ++i) {
      
      suToken * token = cetokens[i];
      if (!token->is_defined ("terms")) continue;
      
      std::vector<int> rawgids = token->get_list_of_integers ("terms");

      for (sutype::uvi_t k=0; k < rawgids.size(); ++k) {      
        sutype::id_t gid = (sutype::id_t)rawgids[k];
        gidsInUse.insert (gid);
      }
    }

    for (const auto & iter : gidsInUse) {
      sutype::id_t gid = iter;
      ids.push_back (gid);
    }

    return ids;
    
  } // end of suGlobalRouter::get_terminal_gids

  //
  void suGlobalRouter::parse_global_routing ()
  {
    if (_grTokenParser == 0) {
      
      SUISSUE("No global routing to parse")
        << std::endl;
      
      return;
    }

    parse_global_routings_ (_grTokenParser->rootToken());

    parse_raw_external_ties_ (_grTokenParser->rootToken());
    
  } // end of suGlobalRouter::parse_global_routing_file
  
  //
  void suGlobalRouter::create_fake_global_routes ()
    const
  {
    for (sutype::uvi_t i=0; i < _globalRoutes.size(); ++i) {
      
      const suGlobalRoute * globalroute = _globalRoutes[i];
      
      const suNet * net = globalroute->net();
      if (!suCellManager::instance()->route_this_net (net)) continue;
      
      const suLayer * layer = globalroute->layer();
      SUASSERT (layer->is_base(), "");
     
      SUASSERT (layer->pgd() == sutype::go_ver || layer->pgd() == sutype::go_hor, "");
      
      sutype::regions_t regions = globalroute->get_regions();
      SUASSERT (!regions.empty(), "");
      SUASSERT (regions.size() >= 2, "small regions are not supported yet");

      const suRegion * region0 = regions.front();
      const suRegion * region1 = regions.back();

      const suRectangle & bbox0 = region0->bbox();
      const suRectangle & bbox1 = region1->bbox();

      sutype::dcoord_t xl = std::min (bbox0.xc(), bbox1.xc());
      sutype::dcoord_t xh = std::max (bbox0.xc(), bbox1.xc());
      sutype::dcoord_t yl = std::min (bbox0.yc(), bbox1.yc());
      sutype::dcoord_t yh = std::max (bbox0.yc(), bbox1.yc());

      sutype::dcoord_t halfWireWidth = globalroute->minWireWidth() / 2;
      
      if (halfWireWidth < 10) {
        halfWireWidth = suMetalTemplateManager::instance()->get_minimal_wire_width (layer) / 2;
      }
      
      if (halfWireWidth < 10) {
        halfWireWidth = 10;
      }     
      
      if (layer->pgd() == sutype::go_ver) {

        SUASSERT (xl == xh, "");                
        xl -= halfWireWidth;
        xh += halfWireWidth;
      }
      
      else if (layer->pgd() == sutype::go_hor) {
        
        SUASSERT (yl == yh, "");
        yl -= halfWireWidth;
        yh += halfWireWidth;
      }
      
      else {
        SUASSERT (false, "");
      }
      
      std::string netname (net->name() + "_gr");

      suNet * net2 = suCellManager::instance()->topCellMaster()->create_net_if_needed (netname);
      SUASSERT (net2, "");

      suWire * wire = suWireManager::instance()->create_wire_from_dcoords (net2, layer, xl, yl, xh, yh);

      net2->add_wire (wire);
    }
    
  } // end of suGlobalRouter::create_fake_global_routes

  //
  void suGlobalRouter::dump_global_routing (const std::string & filename)
    const
  {
    suStatic::create_parent_dir (filename);

    std::ofstream out (filename);
    if (!out.is_open()) {
      SUISSUE("Could not open file for writing") << ": " << filename << std::endl;
      SUASSERT (false, "");
    }

    for (const auto & iter : _globalRoutes) {

      const suGlobalRoute * globalroute = iter;

      out
        << "GlobalRouting net=" << globalroute->net()->name()
        << " routes=" << globalroute->to_str()
        << std::endl;
        
    }

    out.close ();

    SUINFO(1) << "Written " << filename << std::endl;
    
  } // end of suGlobalRouter::dump_global_routing
  
  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------

  //
  void suGlobalRouter::parse_global_routings_ (const suToken * roottoken)
  {
    SUASSERT (roottoken, "");

    SUINFO(1) << "Read global routing" << std::endl;
    
    sutype::tokens_t tokens = roottoken->get_tokens ("GlobalRouting");
    
    std::map<sutype::id_t,sutype::globalroutes_t> globalRoutesPerNet;
    std::set<const suNet *, suNet::cmp_const_ptr> allNetsWithGRs; // to report only

    const sutype::nets_tc & netsToRoute = suCellManager::instance()->netsToRoute();
    
    for (sutype::uvi_t i=0; i < tokens.size(); ++i) {

      const suToken * token = tokens[i];
      const std::string & netname = token->get_string_value ("net");

      const suNet * net = suCellManager::instance()->topCellMaster()->get_net_by_name (netname);
      if (net == 0) {
        SUISSUE("Could not get net for a global route")
          << ": " << netname
          << std::endl;
        //SUASSERT (false, "");
        continue;
      }

      allNetsWithGRs.insert (net);
      
      if (std::find (netsToRoute.begin(), netsToRoute.end(), net) == netsToRoute.end()) {
        SUISSUE("Skipped a global route") << ": Net " << net->name() << std::endl;
        continue;
      }
      
      sutype::strings_t routes = token->get_list_of_strings ("routes", ';');

      for (sutype::uvi_t k=0; k < routes.size(); ++k) {

        sutype::strings_t strs = suToken::parse_as_list_of_strings (routes[k], ',', '(', ')');
        SUASSERT (strs.size() >= 5, token->hint() << " Bad global route: " << routes[k]);
        
        sutype::uvi_t j = 0;

        // mandatory
        int x1 = suToken::string_2_int (strs[j]); ++j;
        int y1 = suToken::string_2_int (strs[j]); ++j;
        int x2 = suToken::string_2_int (strs[j]); ++j;
        int y2 = suToken::string_2_int (strs[j]); ++j;
        const std::string & layername = strs[j]; ++j;

        int xl = std::min (x1, x2);
        int xh = (x1 == xl) ? x2 : x1;
        int yl = std::min (y1, y2);
        int yh = (y1 == yl) ? y2 : y1;

        // optional
        sutype::dcoord_t minw    = 0; // minimal wire width
        sutype::id_t     gid     = sutype::UNDEFINED_GLOBAL_ID;
        sutype::id_t     samewas = sutype::UNDEFINED_GLOBAL_ID; // same wire width as another gid

        for (sutype::uvi_t s=j; s < strs.size(); ++s) {
          
          const std::string & str = strs[s];

          sutype::strings_t parameterNameAndValues;
          suTokenParser::parse_parameter_name_and_values (str, "=", parameterNameAndValues);

//           SUINFO(1) << "\"" << str << "\"" << std::endl;
//           for (const auto & iter0 : parameterNameAndValues) {
//             SUINFO(1) << "\"" << iter0 << "\"" << std::endl;
//           }
          
          if (parameterNameAndValues.size() != 2) {
            SUASSERT (false, "Unexpected format of a GR parameter; expected values are: minw=<INT> or gid=<INT> or samewas=<INT>; but found: " << str);
            continue;
          }

          const std::string & parametername  = parameterNameAndValues[0];
          const std::string & parametervalue = parameterNameAndValues[1];

          if (parametername.empty() || parametervalue.empty()) {
            SUASSERT (false, "Unexpected format of a GR parameter; expected values are: minw=<INT> or gid=<INT> or samewas=<INT>; but found: " << str);
            continue;
          }
          
          if        (parametername.compare ("minw")    == 0) { minw    = suToken::string_2_int (parametervalue); SUASSERT (minw    >  0, "Bad GR: minw must be a positive integer: " << minw);
          } else if (parametername.compare ("gid")     == 0) { gid     = suToken::string_2_int (parametervalue); SUASSERT (gid     >= 0, "Bad GR: gid cannot be negative: " << gid);
          } else if (parametername.compare ("samewas") == 0) { samewas = suToken::string_2_int (parametervalue); SUASSERT (samewas >= 0, "Bad GR: samewas cannot be negative: " << samewas);
          } else {
            SUISSUE("Unexpected GR parameter") << ": " << str << std::endl;
          }
        }
        
        const suLayer * layer = suLayerManager::instance()->get_base_layer_by_name (layername);
        SUASSERT (layer, token->hint() << " Bad global route: " << routes[k] << "; can't get layer " << layername);
        SUASSERT (xl != xh || yl != yh, token->hint() << " Bad global route: " << routes[k] << "; can't be a point");
        SUASSERT (xl == xh || yl == yh, token->hint() << " Bad global route: " << routes[k] << "; must be either vertical ot horizontal: can't be diagonal");
        
        if (xl == xh) {
          if (layer->pgd() == sutype::go_hor) {
            SUISSUE("Bad global route") << ": net " << net->name() << ": " << token->hint() << ": " << routes[k] << "; horizontal layer " << layer->name() << " gets vertical global route. Skipped." << std::endl;
            SUASSERT (false, "Net " << net->name() << ": " << token->hint() << " Bad global route: " << routes[k] << "; horizontal layer " << layer->name() << " gets vertical global route");
            continue;
          }
        }

        if (yl == yh) {
          if (layer->pgd() == sutype::go_ver) {
            SUISSUE("Bad global route") << ": net " << net->name() << ": " << token->hint() << ": " << routes[k] << "; vertical layer " << layer->name() << " gets horizontal global route. SKipped." << std::endl;
            SUASSERT (false, token->hint() << " Bad global route: " << routes[k] << "; vertical layer " << layer->name() << " gets horizontal global route");
            continue;
          }
        }
        
        suGlobalRoute * globalroute = new suGlobalRoute (net, layer, xl, yl, xh, yh, minw, gid, samewas);
        
        globalRoutesPerNet[net->id()].push_back (globalroute);
      }
    }

    //SUABORT;
    
    for (const auto & iter : allNetsWithGRs) {

      const suNet * net = iter;
      SUINFO(1) << "Net had global routes in a file: " << net->name() << std::endl;
    }
    
    for (const auto & iter : netsToRoute) {
      
      const suNet * net = iter;
      sutype::id_t netid = net->id();
      if (globalRoutesPerNet.count (netid) == 0) {
        SUISSUE("Net is to be routed but it has no global routes") << ": " << net->name() << std::endl;
      }
    }
    
    const sutype::nets_t & allnets = suCellManager::instance()->topCellMaster()->nets();
    
    for (const auto & iter : allnets) {
      
      const suNet * net = iter;
      
      if (std::find (netsToRoute.begin(), netsToRoute.end(), net) != netsToRoute.end()) continue; // already reported as an issue
      if (allNetsWithGRs.count (net) != 0) continue; // net is not to be routed but has some GRs; it's OK
      
      SUINFO(1) << "Net had no global routes in a file: " << net->name() << std::endl;
    }
    
    for (const auto & iter : globalRoutesPerNet) {
      
      sutype::id_t netid = iter.first;
      sutype::globalroutes_t & grs = globalRoutesPerNet[netid];
      SUASSERT (!grs.empty(), "");
      
      merge_global_routes_of_one_net_ (grs);
      SUASSERT (!grs.empty(), "");
      
      _globalRoutes.insert (_globalRoutes.end(), grs.begin(), grs.end());
    }
    
    std::sort (_globalRoutes.begin(), _globalRoutes.end(), suStatic::compare_global_routes);
    
    sanity_check_ ();
    
  } // end of suGlobalRouter::parse_global_routings_

//
  void suGlobalRouter::parse_raw_external_ties_ (const suToken * roottoken)
  {
    SUASSERT (roottoken, "");

    SUINFO(1) << "Read raw external ties" << std::endl;
    
    sutype::tokens_t tokens = roottoken->get_tokens ("Tie");

    for (sutype::uvi_t i=0; i < tokens.size(); ++i) {

      const suToken * token = tokens[i];
      
      sutype::id_t gid0 = token->get_integer_value ("term0");
      sutype::id_t gid1 = token->get_integer_value ("gr0");

      _rawExternalTies.push_back (sutype::exttie_t (gid0, gid1));
    }

    SUINFO(1) << "Found " << _rawExternalTies.size() << " raw external ties." << std::endl;
    
  } // end of suGlobalRouter::parse_raw_external_ties_
  
  //
  void suGlobalRouter::merge_global_routes_of_one_net_ (sutype::globalroutes_t & inputgrs)
    const
  {
    std::map<sutype::id_t, sutype::globalroutes_t> globalRoutesPerLayer;
    
    for (const auto & iter : inputgrs) {
      
      suGlobalRoute * gr = iter;
      const suLayer * layer = gr->layer();
      SUASSERT (layer, "");
      SUASSERT (layer->is_base(), "");
      globalRoutesPerLayer[layer->base_id()].push_back (gr);
    }
    
    inputgrs.clear();
    
    for (const auto & iter : globalRoutesPerLayer) {
      
      sutype::id_t layerid = iter.first;
      const suLayer * layer = suLayerManager::instance()->get_base_layer_by_id (layerid);
      SUASSERT (layer, "");

      //SUINFO(1) << "Merge global routes of layer " << layer->name() << std::endl;
      
      sutype::globalroutes_t & grs = globalRoutesPerLayer[layerid];
      
      merge_global_routes_of_one_layer_ (grs);
      inputgrs.insert (inputgrs.end(), grs.begin(), grs.end());
    }
    
  } // end of suGlobalRouter::merge_global_routes_of_one_net_

  //
  void suGlobalRouter::merge_global_routes_of_one_layer_ (sutype::globalroutes_t & inputgrs)
    const
  {
    std::map<sutype::dcoord_t, sutype::globalroutes_t> globalRoutesPerCoord;

    for (const auto & iter : inputgrs) {
      
      suGlobalRoute * gr = iter;
      const suRectangle & bbox = gr->bbox();
      const suLayer * layer = gr->layer();
      SUASSERT (layer, "");
      SUASSERT (!bbox.is_point(), "");
      SUASSERT (bbox.is_line(), "");
      SUASSERT (layer->pgd() == sutype::go_ver || layer->pgd() == sutype::go_hor, "");
      SUASSERT (layer->pgd() != sutype::go_ver || bbox.xl() == bbox.xh(), "");
      SUASSERT (layer->pgd() != sutype::go_hor || bbox.yl() == bbox.yh(), "");

      if        (bbox.xl() == bbox.xh()) { globalRoutesPerCoord[bbox.xl()].push_back (gr);
      } else if (bbox.yl() == bbox.yh()) { globalRoutesPerCoord[bbox.yl()].push_back (gr);
      } else {
        SUASSERT (false, "");
      }
    }
    
    inputgrs.clear();
    
    for (auto & iter : globalRoutesPerCoord) {
      
      sutype::globalroutes_t & grs = iter.second;
      std::sort (grs.begin(), grs.end(), suStatic::compare_global_routes);

      for (sutype::svi_t i=0; i < (sutype::svi_t)grs.size(); ++i) {

        if (!grs[i]) continue;
        
        suGlobalRoute * gr1 = (suGlobalRoute *) grs[i];
        const suLayer * layer1 = gr1->layer();
        suRectangle & bbox1 = gr1->bbox();
        const sutype::dcoord_t minWireWidth1 = gr1->minWireWidth();
        //const bool mandatory1 = gr1->mandatory();

        for (sutype::svi_t k=i+1; k < (sutype::svi_t)grs.size(); ++k) {

          if (!grs[k]) continue;

          const suGlobalRoute * gr2 = grs[k];
          const suRectangle & bbox2 = gr2->bbox();
          const sutype::dcoord_t minWireWidth2 = gr2->minWireWidth();
          //const bool mandatory2 = gr2->mandatory();

          SUASSERT (gr1->net() == gr2->net(), "");
          SUASSERT (gr1->layer() == gr2->layer(), "");

          // global routes can't be merged because they have different minimal wire widths
          //if (minWireWidth1 != minWireWidth2) continue;
          
          // global routes can't be merged because they have different mandatory flag
          //if (mandatory1 != mandatory2) continue;
          
          sutype::dcoord_t maxWireWidth = std::max (minWireWidth1, minWireWidth2);
          
          // merge vertical global routes
          if (layer1->pgd() == sutype::go_ver) {

            SUASSERT (bbox1.xl() == bbox1.xh(), "");
            SUASSERT (bbox1.yl()  < bbox1.yh(), "");
            SUASSERT (bbox2.xl() == bbox2.xh(), "");
            SUASSERT (bbox2.yl()  < bbox2.yh(), "");
            SUASSERT (bbox1.xl() == bbox2.xh(), "");
            SUASSERT (bbox1.yl() <= bbox2.yl(), "");

            if (bbox2.yl() > bbox1.yh()) continue;
            
            SUISSUE("Two vertical global routes overlap") << ": " << gr1->to_str() << "; " << gr2->to_str() << std::endl;
            bbox1.yh (std::max (bbox1.yh(), bbox2.yh()));
            gr1->minWireWidth (maxWireWidth);
            SUISSUE("Two vertical GRs were merged into a single global route") << ": " << gr1->to_str() << "; deleted global route: " << gr2->to_str() << std::endl;
            delete gr2;
            grs[k] = 0;
            --i;
            break;
          }
          
          else if (layer1->pgd() == sutype::go_hor) {

            SUASSERT (bbox1.yl() == bbox1.yh(), "");
            SUASSERT (bbox1.xl()  < bbox1.xh(), "");
            SUASSERT (bbox2.yl() == bbox2.yh(), "");
            SUASSERT (bbox2.xl()  < bbox2.xh(), "");
            SUASSERT (bbox1.yl() == bbox2.yh(), "");
            SUASSERT (bbox1.xl() <= bbox2.xl(), "");

            if (bbox2.xl() > bbox1.xh()) continue;

            SUISSUE("Two horizontal global routes overlap") << ": " << gr1->to_str() << "; " << gr2->to_str() << std::endl;
            bbox1.xh (std::max (bbox1.xh(), bbox2.xh()));
            gr1->minWireWidth (maxWireWidth);
            SUISSUE("Two horizontal GRs were merged into a single global route") << ": " << gr1->to_str() << "; deleted global route: " << gr2->to_str() << std::endl;
            delete gr2;
            grs[k] = 0;
            --i;
            break;
          }
          
          else {
            SUASSERT (false, "");
          }
        }
      }

      for (sutype::uvi_t i=0; i < grs.size(); ++i) {
        
        suGlobalRoute * gr = grs[i];
        if (!gr) continue;
        inputgrs.push_back (gr);
      }
    }
    
  } // end of suGlobalRouter::merge_global_routes_of_one_layer_
                                      
  //
  void suGlobalRouter::create_regions_ ()
  {
    SUASSERT (suCellManager::instance()->topCellMaster(), "");
    const suRectangle & topCellMasterBbox = suCellManager::instance()->topCellMaster()->bbox();
    SUASSERT (!topCellMasterBbox.is_point(), "");
    SUASSERT (!topCellMasterBbox.is_line(), "");
    
    const suLayer * polylayer = suLayerManager::instance()->get_base_layer_by_type (sutype::lt_poly);
    const suLayer * difflayer = suLayerManager::instance()->get_base_layer_by_type (sutype::lt_diffusion);

    SUASSERT (polylayer, "Could not find poly layer. Poly pitch is used to calculate the size of global regions.");
    SUASSERT (difflayer, "Could not find diffusion layer. Diffusion grid is used to calculate the size of global regions.");
    
    const sutype::dcoord_t polypitch = polylayer->token()->get_integer_value ("Technology", "pitch");
    const sutype::dcoord_t diffpitch = difflayer->token()->get_integer_value ("Technology", "pitch");

    const float gr_region_width_in_poly_pitches  = suOptionManager::instance()->token()->get_float_value ("Option", "name", "gr_region_width_in_poly_pitches",  "value");
    const float gr_region_height_in_diff_pitches = suOptionManager::instance()->token()->get_float_value ("Option", "name", "gr_region_height_in_diff_pitches", "value");

    _regionw = (sutype::dcoord_t)(gr_region_width_in_poly_pitches  * float(polypitch) + 0.5);
    _regionh = (sutype::dcoord_t)(gr_region_height_in_diff_pitches * float(diffpitch) + 0.5);
    
    SUASSERT (_regionw > 0, "");
    SUASSERT (_regionh > 0, "");

    if (polypitch % 10 == 0) { SUASSERT (_regionw %  5 == 0, "unexpected granularity of GR grid [A]: " << _regionw); }
    if (polypitch % 20 == 0) { SUASSERT (_regionw % 10 == 0, "unexpected granularity of GR grid [B]: " << _regionw); }

    if (diffpitch % 10 == 0) { SUASSERT (_regionh %  5 == 0, "unexpected granularity of GR grid [C]: " << _regionh); }
    if (diffpitch % 20 == 0) { SUASSERT (_regionh % 10 == 0, "unexpected granularity of GR grid [D]: " << _regionh); }
    
    _bbox.copy (topCellMasterBbox);
    
    _numcols = (_bbox.w() / _regionw) + ((_bbox.w() % _regionw) ? 1 : 0);
    _numrows = (_bbox.h() / _regionh) + ((_bbox.h() % _regionh) ? 1 : 0);
    
    SUINFO(1)
      << "Number of columns: " << _numcols
      << "; number of rows: " << _numrows
      << std::endl;
    
    if (1) {
      sutype::regions_t tmp (_numrows, (suRegion*)0);
      _regions.resize (_numcols, tmp);
    }

    _minRegionId = suRegion::get_next_id ();
    _idToRegion.resize (_numcols * _numrows, (suRegion*)0);
    
    for (sutype::uvi_t x = 0; x < _numcols; ++x) {

      sutype::dcoord_t xl = _bbox.xl() + _regionw * x;
      sutype::dcoord_t xh = xl + _regionw;

      for (sutype::uvi_t y = 0; y < _numrows; ++y) {

        sutype::dcoord_t yl = _bbox.yl() + _regionh * y;
        sutype::dcoord_t yh = yl + _regionh;
        
        suRegion * region = new suRegion (xl, yl, xh, yh, x, y);
        sutype::svi_t index = region->id() - _minRegionId;
        SUASSERT (index >= 0 && index < (sutype::svi_t)_idToRegion.size(), region->id() << " " << _minRegionId << " " << _idToRegion.size());
        
        _idToRegion [index] = region;
        _regions[x][y] = region;
      }
    }    
    
  } // end of suGlobalRouter::create_regions_

  //
  void suGlobalRouter::create_edges_ ()
  {
    // create north/east edges
    for (sutype::uvi_t x = 0; x < _numcols; ++x) {

      for (sutype::uvi_t y = 0; y < _numrows; ++y) {

        suRegion * region = _regions[x][y];

        // create north edge
        if (y+1 < _numrows) {
          
          sutype::dcoord_t xh = region->bbox().xh();
          sutype::dcoord_t xl = region->bbox().xl();
          sutype::dcoord_t yh = region->bbox().yh();
          sutype::dcoord_t yl = yh;
          
          suGREdge * gredge = new suGREdge (xl, yl, xh, yh, sutype::go_hor);
          region->set_edge (sutype::side_north, gredge);
          
          _gredges.push_back (gredge);
        }

        // create east edge
        if (x+1 < _numcols) {

          sutype::dcoord_t yh = region->bbox().yh();
          sutype::dcoord_t yl = region->bbox().yl();
          sutype::dcoord_t xh = region->bbox().xh();
          sutype::dcoord_t xl = xh;

          suGREdge * gredge = new suGREdge (xl, yl, xh, yh, sutype::go_ver);
          region->set_edge (sutype::side_east, gredge);
          
          _gredges.push_back (gredge);
        }
      }
    }

    // share edges
    for (sutype::uvi_t x = 0; x < _numcols; ++x) {

      for (sutype::uvi_t y = 0; y < _numrows; ++y) {

        if (y > 0) _regions[x][y]->set_edge (sutype::side_south, _regions[x][y-1]->get_edge (sutype::side_north));
        if (x > 0) _regions[x][y]->set_edge (sutype::side_west,  _regions[x-1][y]->get_edge (sutype::side_east));
      }
    }

    //
    for (sutype::uvi_t i = 0; i < _gredges.size(); ++i) {

      suGREdge * gredge = _gredges[i];
      gredge->calculate_capacity ();
    }
    
  } // end of suGlobalRouter::create_edges_
  
  //
  void suGlobalRouter::get_regions_ (const suRectangle & rect,
                                     const suRegion * & swregion,
                                     const suRegion * & nwregion,
                                     const suRegion * & seregion,
                                     const suRegion * & neregion)
    const
  {
    SUASSERT (_numcols > 0, "");
    SUASSERT (_numrows > 0, "");

    int wcol = 0;
    int ecol = 0;
    int srow = 0;
    int nrow = 0;

    get_region_bounds_ (rect, wcol, ecol, srow, nrow);

    SUASSERT (wcol >= 0 && wcol < (int)_numcols, rect.to_str(":") << "; wcol=" << wcol);
    SUASSERT (ecol >= 0 && ecol < (int)_numcols, rect.to_str(":") << "; ecol=" << ecol);
    SUASSERT (srow >= 0 && srow < (int)_numrows, rect.to_str(":") << "; srow=" << srow);
    SUASSERT (nrow >= 0 && nrow < (int)_numrows, rect.to_str(":") << "; nrow=" << nrow);
    SUASSERT (wcol <= ecol, "");
    SUASSERT (srow <= nrow, "");
    
    swregion = _regions [wcol] [srow];
    nwregion = _regions [wcol] [nrow];
    seregion = _regions [ecol] [srow];
    neregion = _regions [ecol] [nrow];

    SUASSERT (swregion, "");
    SUASSERT (nwregion, "");
    SUASSERT (seregion, "");
    SUASSERT (neregion, "");

    SUASSERT (swregion->col() == nwregion->col(), "");
    SUASSERT (seregion->col() == neregion->col(), "");
    SUASSERT (swregion->row() == seregion->row(), "");
    SUASSERT (nwregion->row() == neregion->row(), "");

    SUASSERT (swregion->col() == wcol, ""); SUASSERT (swregion->row() == srow, "");
    SUASSERT (nwregion->col() == wcol, ""); SUASSERT (nwregion->row() == nrow, "");
    SUASSERT (seregion->col() == ecol, ""); SUASSERT (seregion->row() == srow, "");
    SUASSERT (neregion->col() == ecol, ""); SUASSERT (neregion->row() == nrow, "");
    
  } // end of suGlobalRouter::get_regions_

  //
  void suGlobalRouter::get_region_bounds_ (const suRectangle & rect,
                                           int & wcol,
                                           int & ecol,
                                           int & srow,
                                           int & nrow)
    const
  {
    SUASSERT (_numcols > 0, "");
    SUASSERT (_numrows > 0, "");

    wcol = dcoord_to_region_coord (rect.xl(), sutype::go_hor, sutype::side_west);
    ecol = dcoord_to_region_coord (rect.xh(), sutype::go_hor, sutype::side_east);
    srow = dcoord_to_region_coord (rect.yl(), sutype::go_ver, sutype::side_south);
    nrow = dcoord_to_region_coord (rect.yh(), sutype::go_ver, sutype::side_north);

    SUASSERT (wcol >= 0 && wcol < (int)_numcols, rect.to_str(":") << "; wcol=" << wcol);
    SUASSERT (ecol >= 0 && ecol < (int)_numcols, rect.to_str(":") << "; ecol=" << ecol);
    SUASSERT (srow >= 0 && srow < (int)_numrows, rect.to_str(":") << "; srow=" << srow);
    SUASSERT (nrow >= 0 && nrow < (int)_numrows, rect.to_str(":") << "; nrow=" << nrow);
    SUASSERT (wcol <= ecol, "");
    SUASSERT (srow <= nrow, "");    

  } // end of suGlobalRouter::get_region_bounds_

  // the procedure supports regular grid at this moment
  int suGlobalRouter::dcoord_to_region_coord (sutype::dcoord_t dcoord,
                                              sutype::grid_orientation_t pgd,
                                              sutype::side_t priorityside)
    const
  {
    sutype::dcoord_t dcoord0 = (pgd == sutype::go_ver) ? _bbox.yl() : _bbox.xl();
    sutype::dcoord_t pitch   = (pgd == sutype::go_ver) ? _regionh   : _regionw;

    sutype::dcoord_t refdcoord = dcoord - dcoord0;

    int c = refdcoord / pitch;
    
    if ((refdcoord % pitch) == 0) {

      if (priorityside == sutype::side_south || priorityside == sutype::side_west)
        --c;
    }

    return c;
    
  } // end of suGlobalRouter::dcoord_to_region_coord
  
  //
  void suGlobalRouter::sanity_check_ ()
    const
  {
    // check global ids
    std::map<sutype::id_t, suGlobalRoute*> grgids;
    
    // collect gids
    for (const auto & iter0 : _globalRoutes) {
      
      suGlobalRoute * gr0 = iter0;
      //SUINFO(1) << gr0->to_str() << std::endl;
      
      sutype::id_t gid0 = gr0->gid();
      sutype::id_t gid1 = gr0->sameWidthAsAnotherGid();

      if (gid0 != sutype::UNDEFINED_GLOBAL_ID || gid1 != sutype::UNDEFINED_GLOBAL_ID) {
        //SUASSERT (gid0 != gid1, "GR has idential gid and sameWidthAsAnotherGid: " << gid0);
      }
      
      if (gid0 == sutype::UNDEFINED_GLOBAL_ID) continue;
      SUASSERT (gid0 >= 0, "");
      
      if (grgids.count (gid0) > 0) {
        SUASSERT (false, "Two GRs use the same gid: " << gid0);
        continue;
      }
            
      grgids[gid0] = gr0;
    }
    
    // check gids
    for (const auto & iter0 : _globalRoutes) {

      suGlobalRoute * gr0 = iter0;
      
      sutype::id_t gid1 = gr0->sameWidthAsAnotherGid();
      if (gid1 == sutype::UNDEFINED_GLOBAL_ID) continue;
      SUASSERT (gid1 >= 0, "");

      if (grgids.count (gid1) == 0) {
        SUISSUE("Could not find GR by gid") << ": " << gid1 << std::endl;
        //SUASSERT (false, "Could not find GR by gid");
        continue;
      }
      
      suGlobalRoute * gr1 = grgids.at (gid1);
      
      if (gr1->layer() != gr0->layer()) {
        SUISSUE("Two GRs must have the same width but they belong to different layers")
          << ": gr0=" << gr0->to_str()
          << "; gr1=" << gr1->to_str()
          << std::endl;
        SUASSERT (false, "");
        continue;
      }

      if (gr1->minWireWidth() != gr0->minWireWidth()) {
        SUISSUE("Two GRs must have the same width but they have different minimal wire width")
          << ": gr0=" << gr0->to_str()
          << "; gr1=" << gr1->to_str()
          << std::endl;
        SUASSERT (false, "");
        continue;
      }
    }
    
    for (sutype::uvi_t i=0; i < _globalRoutes.size(); ++i) {

      const suGlobalRoute * globalroute1 = _globalRoutes[i];
      const suLayer * layer1 = globalroute1->layer();
      const suNet * net1 = globalroute1->net();

      for (sutype::uvi_t k=i+1; k < _globalRoutes.size(); ++k) {

        const suGlobalRoute * globalroute2 = _globalRoutes[k];
        const suLayer * layer2 = globalroute2->layer();
        const suNet * net2 = globalroute2->net();

        SUASSERT (net2->id() >= net1->id(), "expected to be sorted");
        
        if (net2 != net1) break;
        if (layer2->pgd() != layer1->pgd()) continue;

        if (globalroute1->bbox().has_at_least_common_line (globalroute2->bbox())) {
          SUISSUE("Two parallel routes have at least two common regions") << ": " << globalroute1->to_str() << "; " << globalroute2->to_str() << std::endl;
          //SUASSERT (false, "");
        }
      }
    }
    
  } // end of suGlobalRouter::sanity_check_


} // end of namespace amsr

// end of suGlobalRouter.cpp
