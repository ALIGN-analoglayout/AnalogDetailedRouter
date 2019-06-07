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
//! \date   Mon Oct  9 12:16:05 2017

//! \file   suCellManager.cpp
//! \brief  A collection of methods of the class suCellManager.

// std includes
#include <algorithm>
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suCellMaster.h>
#include <suGenerator.h>
#include <suGeneratorInstance.h>
#include <suGeneratorManager.h>
#include <suGlobalRouter.h>
#include <suErrorManager.h>
#include <suLayer.h>
#include <suLayerManager.h>
#include <suNet.h>
#include <suOptionManager.h>
#include <suRegion.h>
#include <suSatRouter.h>
#include <suStatic.h>
#include <suToken.h>
#include <suTokenParser.h>
#include <suWire.h>
#include <suWireManager.h>

// module include
#include <suCellManager.h>

namespace amsr
{
  //
  suCellManager * suCellManager::_instance = 0;

  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  //
  suCellManager::~suCellManager ()
  {
    for (sutype::uvi_t i=0; i < _cellMasters.size(); ++i) {
      delete _cellMasters[i];
    }

    _topCellMaster = 0;
    
  } // end of suCellManager::~suCellManage

  // ------------------------------------------------------------
  // -
  // --- Public static methods
  // -
  // ------------------------------------------------------------

  // static
  void suCellManager::delete_instance ()
  {
    if (suCellManager::_instance)
      delete suCellManager::_instance;

    suCellManager::_instance = 0;
    
  } // end of suCellManager::delete_instance
  
  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  // quite simplified procedure just to create a dummy cell to test routing algorithms
  void suCellManager::read_input_file (const std::string & filename)
  {
    SUASSERT (_topCellMaster == 0, "Top cell master was already created. Found a second one.");
    
    suTokenParser tokenparser;
    
    bool ok = tokenparser.read_file (filename);
    SUASSERT (ok, "");
    SUASSERT (tokenparser.rootToken(), "");

    sutype::tokens_t tokens = tokenparser.rootToken()->get_tokens ("Cell");
    SUASSERT (tokens.size() == 1, "Only one cell master is expected at this moment");
    const suToken * token = tokens.front();

    //SUINFO(1) << token->to_str("") << std::endl;

    std::vector<int> bbox = token->get_list_of_integers ("bbox", ':');
    SUASSERT (bbox.size() == 4, token->hint() << " bad bbox");
    
    // get name of the cell; take a value of paramater "name" or an indexed value
    const std::string & cellname = token->is_defined("name") ? token->get_string_value("name") : token->get_string_value(0);
    SUASSERT (!cellname.empty(), "");
    _topCellMaster = new suCellMaster (cellname);
    _cellMasters.push_back (_topCellMaster);
        
    unsigned i = 0;
    _topCellMaster->_bbox.xl (bbox[i]); ++i;
    _topCellMaster->_bbox.yl (bbox[i]); ++i;
    _topCellMaster->_bbox.xh (bbox[i]); ++i;
    _topCellMaster->_bbox.yh (bbox[i]); ++i;

    SUINFO(1) << "Read wires." << std::endl;

    read_wires_ (_topCellMaster, token);
    read_wires_ (_topCellMaster, tokenparser.rootToken());

    sutype::tokens_t objtokens = tokenparser.rootToken()->get_tokens ("Obj");
    for (const auto & iter0 : objtokens) {
      suToken * objtoken = iter0;
      read_wires_ (_topCellMaster, objtoken);
    }
    
  } // end of suCellManager::read_input_file

  // very dummy implementation
  void suCellManager::dump_out_file (const std::string & filename,
                                     bool lgfstyle)
    const
  {
    suStatic::create_parent_dir (filename);

    std::ofstream out (filename);
    if (!out.is_open()) {
      SUISSUE("Could not open file for writing") << ": " << filename << std::endl;
      SUASSERT (false, "");
    }
    
    const suCellMaster * cellmaster = _topCellMaster;

    const bool option_dump_global_regions                  = suOptionManager::instance()->get_boolean_option ("dump_global_regions",                  true);
    const bool option_dump_global_regions_with_coordinates = suOptionManager::instance()->get_boolean_option ("dump_global_regions_with_coordinates", false);
    
    out
      << "Cell ";

    if (lgfstyle) {
      out
        << "ec0_dummy_prefix_";
    }
    
    out
      << cellmaster->name()
      << " bbox=" << cellmaster->bbox().to_str(":")
      << std::endl;

    out << std::endl;

    const sutype::nets_t & nets = cellmaster->nets();

    for (sutype::uvi_t i=0; i < nets.size(); ++i) {

      const suNet * net = nets[i];
      const sutype::wires_t & wires = net->wires ();
      const sutype::generatorinstances_t & generatorinstances = net->generatorinstances ();

      for (const auto & iter0 : wires) {

        const suWire * wire = iter0;
        SUASSERT (wire->net() == net, "");
        
        out
          << "Wire"
          << " net="   << wire->net()->name()
          << " layer=" << wire->layer()->name()
          << " rect="  << wire->rect().to_str(":");
        
        if (!lgfstyle) {
          if (wire->gid() != sutype::UNDEFINED_GLOBAL_ID) {
            out << " gid=" << wire->gid();
          }
        }
        
        out << std::endl;
      }
      
      for (const auto & iter0 : generatorinstances) {
        
        const suGeneratorInstance * gi = iter0;
        SUASSERT (gi->net() == net, "");
        
        if (!lgfstyle) {
          out
            << "Obj"
            << " net=" << gi->net()->name()
            << " gen=" << gi->generator()->name()
            << " x="   << gi->point().x()
            << " y="   << gi->point().y()
            << " {"
            << std::endl;
        }

        const sutype::wires_t & giwires = gi->wires();

        for (const auto & iter1 : giwires) {

          const suWire * wire = iter1;
          SUASSERT (wire->net() == net, "");

          if (!lgfstyle) {
            out
              << "  ";
          }

          out
            << "Wire"
            << " net="   << wire->net()->name()
            << " layer=" << wire->layer()->name()
            << " rect="  << wire->rect().to_str(":")
            << std::endl;
        }
        
        if (!lgfstyle) {
          out << "}" << std::endl;
        }
      }
    }

    if (option_dump_global_regions) {

      const sutype::regions_t & regions = suGlobalRouter::instance()->regions();

      const suLayer * welllayer = suLayerManager::instance()->get_base_layer_by_type (sutype::lt_well);

      for (sutype::uvi_t i=0; i < regions.size(); ++i) {

        const suRegion * region = regions[i];
      
        out
          << "Wire"
          << " net=z";

        if (option_dump_global_regions_with_coordinates) {
          out
            << "_" << region->col() << "_" << region->row();
        }
      
        out
          << " layer=" << (welllayer ? welllayer->name() : "nwell")
          << " rect=" << region->bbox().to_str(":")
          << std::endl;
      }
    }
    
    out.close ();

    SUINFO(2) << "Written " << filename << std::endl;
    
  } // end of suCellManager::dump_out_file

  //
  void suCellManager::register_a_net (const suNet * net)
  {
    SUASSERT (net->id() >= 0 && net->id() == (sutype::id_t)_idToNet.size(), net->id());
    
    _idToNet.push_back (net);
    
    SUASSERT (net->id() >= 0 && net->id() < (sutype::id_t)_idToNet.size(), "");
    SUASSERT (_idToNet[net->id()] == net, "");

  } // end of suCellManager::register_a_net

  //
  void suCellManager::detect_nets_to_route ()
  {
    // route all nets by default
    _netsToRoute = _idToNet;

    // exclude nets those are not "nets-to-route"
    for (int mode=1; mode <= 2; ++mode) {

      const std::string & optionname = (mode == 1) ? "nets_to_route" : "nets_not_to_route";
      
      sutype::tokens_t tokens1 = suOptionManager::instance()->tokenParser()->rootToken()->get_tokens ("Option", "name", optionname);
      
      std::set<std::string> netnames;

      for (const auto & iter1 : tokens1) {

        const suToken * token = iter1;
        sutype::strings_t strvalues = token->get_list_of_strings ("value");
        for (const auto & iter2 : strvalues) {
          netnames.insert (iter2);
        }
      }

      if (netnames.empty()) continue;

      for (sutype::svi_t i=0; i < (sutype::svi_t)_netsToRoute.size(); ++i) {

        const suNet * net = _netsToRoute[i];
        const std::string & netname = net->name();

        //bool netIsInList = (std::find (netnames.begin(), netnames.end(), netname) != netnames.end());
        bool netIsInList = suStatic::string_matches_one_of_regular_expressions (netname, netnames);
        
        // "to route"
        if (mode == 1) {
          if (netIsInList) {
            // do nothing; keep this net
          }
          else {
            // remove this net
            SUISSUE("Do not route net") << ": " << net->name() << std::endl;
            _netsToRoute[i] = _netsToRoute.back();
            _netsToRoute.pop_back();
            --i;
          }
        }
        
        // "not to route"
        else if (mode == 2) {
          if (netIsInList) {
            // remove this net
            SUISSUE("Do not route net") << ": " << net->name() << std::endl;
            _netsToRoute[i] = _netsToRoute.back();
            _netsToRoute.pop_back();
            --i;
          }
          else {
            // do nothing; keep this net
          }
        }

        else {
          SUASSERT (false, "");
        }
      }
    }
    
    std::sort (_netsToRoute.begin(), _netsToRoute.end(), suStatic::compare_nets_by_id);

    SUINFO(1) << "Route following nets (nets to route):" << std::endl;
    
    for (const auto & iter : _netsToRoute) {
      SUINFO(1) << iter->name() << std::endl;
    }
    
  } // end of suCellManager::detect_nets_to_route

  //
  void suCellManager::fix_conflicts ()
  {
    sutype::nets_t relaxednets;
    
    // hardcoded for a while
    if (1) {
      suNet * net = _topCellMaster->get_net_by_name ("ALS_KOR_DO_NOT_ROUTE");
      if (net) relaxednets.push_back (net);
    }

    if (relaxednets.empty()) return;

    bool repeat = true;
    
    while (repeat) {

      repeat = false;

      // wires
      for (const auto & iter0 : relaxednets) {

        suNet * net = iter0;

        sutype::wires_t wires = net->wires(); // a hard copy

        for (const auto & iter1 : wires) {

          suWire * wire = iter1;
        
          const bool verbose = false;
          suNet * conflictingnet = 0;
        
          bool wireok = suSatRouter::instance()->wire_is_legal_slow_check (wire, verbose, conflictingnet);
          if (wireok) continue;
        
          SUISSUE("Deleted an illegal wire")
            << ": " << wire->to_str()
            << std::endl;

          // replace by a new wire of another net
          if (conflictingnet && std::find (relaxednets.begin(), relaxednets.end(), conflictingnet) == relaxednets.end()) {

            suWire * newwire = suWireManager::instance()->create_wire_from_wire (conflictingnet,
                                                                                 wire,
                                                                                 wire->get_wire_type());
            SUASSERT (newwire, "");

            repeat = true;
            
            SUISSUE("Created a new wire to fix a conflict")
              << ": " << newwire->to_str()
              << std::endl;
            
            conflictingnet->add_wire (newwire);
          }
        
          net->remove_wire (wire);
        
          SUASSERT (suWireManager::instance()->get_wire_usage (wire) == 1, wire->to_str());
          suWireManager::instance()->release_wire (wire);
          
        } // for(wires)
      } // for(relaxednets)

      if (repeat) continue;

      // generator instances
      for (const auto & iter0 : relaxednets) {

        suNet * net = iter0;

        sutype::generatorinstances_t gis = net->generatorinstances(); // a hard copy

        for (const auto & iter1 : gis) {

          suGeneratorInstance * gi = iter1;
          
          const sutype::wires_t & giwires = gi->wires();

          const bool verbose = false;
          suNet * conflictingnet = 0;
          bool giok = true;
          
          for (const auto & iter2 : giwires) {

            suWire * wire = iter2;

            bool wireok = suSatRouter::instance()->wire_is_legal_slow_check (wire, verbose, conflictingnet);
            if (wireok) continue;

            giok = false;
            break;
          }

          if (giok) continue;

          SUISSUE("Deleted an illegal generator instance")
            << ": " << gi->to_str()
            << std::endl;

          // replace by a new gi of another net
          if (conflictingnet && std::find (relaxednets.begin(), relaxednets.end(), conflictingnet) == relaxednets.end()) {

            // (not implemented yet)

            //repeat = true;
          }

          net->remove_generator_instance (gi);

          delete gi;
        }
      }
      
    } // while(repeat)
    
  } // end of suCellManager::fix_conflicts

  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------

  //
  void suCellManager::read_wires_ (suCellMaster * cellmaster,
                                   const suToken * token)
    const
  {
    const bool verbose = false;

    sutype::tokens_t tokens = token->get_tokens ("Wire");

    std::map<std::string,sutype::uvi_t> messages;

    std::map<std::string,sutype::tokens_t> wireTokens;
    std::map<std::string,sutype::tokens_t>  viaTokens;
    
    for (sutype::uvi_t i=0; i < tokens.size(); ++i) {

      suToken * token = tokens[i];
      
      const std::string & netname = token->get_string_value ("net");
      const std::string & layername  = token->get_string_value ("layer");
            
      const suLayer* layer = suLayerManager::instance()->get_colored_layer_by_name (layername);
      if (layer == 0) {
        std::string msg ("Skipped a wire of unknown layer " + layername);
        ++messages[msg];
        //SUISSUE("") << token->hint() << " " << msg << std::endl;
        continue;
      }

      if (layer->electricallyConnected().empty()) {
        std::string msg ("Skipped a wire of layer " + layername + " (for a while): it has no electrically connected layers");
        ++messages[msg];
        //SUISSUE("") << token->hint() << " " << msg << std::endl;
        continue;
      }

      if (!layer->is (sutype::lt_wire) && !layer->is (sutype::lt_via)) {
        std::string msg ("Skipped a wire of layer " + layername + " (for a while): unsupported layer type " + suStatic::layer_type_2_str(layer->type()));
        ++messages[msg];
        //SUISSUE("") << token->hint() << " " << msg << std::endl;
        continue;
      }

      if (layer->is (sutype::lt_wire)) {
        SUASSERT (!layer->is (sutype::lt_via), "");
        wireTokens [netname].push_back (token);
      }

      if (layer->is (sutype::lt_via)) {
        SUASSERT (!layer->is (sutype::lt_wire), "");
        viaTokens [netname].push_back (token);
      }
    }

    // mode 1: create wires
    // mode 2: create vias
    for (int mode = 1; mode <= 2; ++mode) {
      
      const std::map<std::string,sutype::tokens_t> & netTokens = (mode == 1) ? wireTokens : viaTokens;
      
      for (const auto & iter1 : netTokens) {
        
        const sutype::tokens_t & tokens2 = iter1.second;
        
        for (sutype::uvi_t i=0; i < tokens2.size(); ++i) {

          suToken * token = tokens2[i];
      
          const std::string & netname = token->get_string_value ("net");
          const std::string & layername  = token->get_string_value ("layer");
          std::vector<int> rectcoords = token->get_list_of_integers ("rect", ':');
          SUASSERT (rectcoords.size() >= 4, token->hint());      

          const suLayer* layer = suLayerManager::instance()->get_colored_layer_by_name (layername);
          SUASSERT (layer, "");

          suNet* net = cellmaster->get_net_by_name (netname);
          if (net == 0)
            net = cellmaster->create_net (netname);
          SUASSERT (net, "");
      
          sutype::id_t gid = sutype::UNDEFINED_GLOBAL_ID;
          if (token->is_defined ("gid")) {
            gid = token->get_integer_value ("gid");
            SUASSERT (gid >= 0, "");
          }

          unsigned j = 0;
          sutype::dcoord_t rectxl = rectcoords[j]; ++j;
          sutype::dcoord_t rectyl = rectcoords[j]; ++j;
          sutype::dcoord_t rectxh = rectcoords[j]; ++j;
          sutype::dcoord_t rectyh = rectcoords[j]; ++j;
      
          // create a wire
          if (layer->is (sutype::lt_wire)) {
            
            SUASSERT (!layer->is (sutype::lt_via), "");
            
            suWire* wire = suWireManager::instance()->create_wire_from_dcoords (net, layer, rectxl, rectyl, rectxh, rectyh, sutype::wt_preroute);
            SUASSERT (wire->is (sutype::wt_preroute), "");
            
            if (gid != sutype::UNDEFINED_GLOBAL_ID) {
              suWireManager::instance()->reserve_gid_for_wire (wire,
                                                               gid,
                                                               true); // check is this id was pre-reserved
            }
            
            if (net->has_wire (wire)) {
              SUASSERT (suWireManager::instance()->get_wire_usage (wire) == 2, "");
              suWireManager::instance()->release_wire (wire);
              SUASSERT (suWireManager::instance()->get_wire_usage (wire) == 1, "");
              continue;
            }
            
            net->add_wire (wire);
            SUASSERT (suWireManager::instance()->get_wire_usage (wire) == 1, "");
            
            //bool wireok = suSatRouter::instance()->wire_is_legal_slow_check (wire, true);
            //if (!wireok) {
            //  SUISSUE("Input wire is illegal") << ": " << wire->to_str() << std::endl;
            //}
            
          } // if(wire)
          
          // create a via
          // here, I create a via generator instance from a wire layer and geometry
          // several generators may match this
          // so, I use just create net wires to detect a correct color of landing/enclosure
          // I use suSatRouter::wires_are_in_conflict to detect illegal overlaps
          if (layer->is (sutype::lt_via)) {
            
            SUASSERT (!layer->is (sutype::lt_wire), "");

            sutype::dcoord_t dx = (rectxl + rectxh) / 2;
            sutype::dcoord_t dy = (rectyl + rectyh) / 2;

            sutype::dcoord_t rectw = rectxh - rectxl;
            sutype::dcoord_t recth = rectyh - rectyl;

            const sutype::generators_tc & generators = suGeneratorManager::instance()->get_generators (layer);
            SUASSERT (!generators.empty(), "No generators for layer " << layer->to_str());

            suGeneratorInstance * gi = 0;
            sutype::uvi_t numgis = 0;

            SUINFO(verbose)
              << "Find GI for a via"
              << "; layer=" << layer->to_str()
              << "; rectw=" << rectw
              << "; recth=" << recth
              << std::endl;
          
            for (sutype::uvi_t g=0; g < generators.size(); ++g) {

              const suGenerator * generator = generators[g];
              if (generator->get_shape_width(sutype::vgl_cut) != rectw || generator->get_shape_height(sutype::vgl_cut) != recth) continue;
              
              suGeneratorInstance * tmpgi = new suGeneratorInstance (generator, net, dx, dy, sutype::wt_preroute);
              ++numgis;

              SUINFO(verbose)
                << "Found a preliminary GI: " << tmpgi->to_str()
                << std::endl;
              
              if (suSatRouter::wires_are_in_conflict (tmpgi->wires(), net->wires(), verbose)) {
                delete tmpgi;
                continue;
              }
              
              //SUASSERT (gi == 0, "Several generator instances match an input via: gi1=" << gi->to_str() << "; gi2=" << tmpgi->to_str());
              
              if (gi) {
                SUISSUE("Several generator instances match an input via")
                  << ": gi1=" << gi->to_str() << "; gi2=" << tmpgi->to_str()
                  << std::endl;
                delete tmpgi;
              }
              else {
                gi = tmpgi;
              }
            }

            if (!gi) {
              
              if (numgis == 0) {
                SUISSUE("Input via does not match any generator (written an error)")
                  << ": net=" << netname << "; layer=" << layer->to_str() << "; xl=" << rectxl << "; yl=" << rectyl << "; xh=" << rectxh << "; yh=" << rectyh
                  << "; rectw=" << rectw << "; recth=" << recth
                  << "; gid=" << gid
                  << std::endl;
              }
              else {
                SUISSUE("All possible generator instances for an input via are conflicting with other wires and vias (written an error)")
                  << ": net=" << netname << "; layer=" << layer->to_str() << "; xl=" << rectxl << "; yl=" << rectyl << "; xh=" << rectxh << "; yh=" << rectyh
                  << "; rectw=" << rectw << "; recth=" << recth
                  << "; gid=" << gid
                  << std::endl;
              }
              
              suErrorManager::instance()->add_error (std::string("no_gi_") + layer->name(),
                                                     rectxl,
                                                     rectyl,
                                                     rectxh,
                                                     rectyh);
              
              SUASSERT (false, "Could not create a legal generator instance for an input via. Read an issue message above.");
              continue;
            }
            
            SUINFO(verbose) << "Created a generator instance: " << gi->to_str() << std::endl;
            
            net->add_generator_instance (gi);
            
          } // if(via)
        } // for(tokens)
      } // for(netTokens)      
    } // for(mode)
    
    if (!messages.empty()) {

      SUINFO(1) << "Done reading wires. Found following issues:" << std::endl;
      
      for (const auto & iter : messages) {
        const std::string & msg = iter.first;
        sutype::uvi_t count = iter.second;
        SUISSUE("Skipped an unexpected wire") << ": " << msg << " (" << count << " times)" << std::endl;
      }
    }
    
  } // end of suCellManager::read_wires_


} // end of namespace amsr

// end of suCellManager.cpp
