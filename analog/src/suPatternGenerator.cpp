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
//! \date   Thu Jan 25 12:01:41 2018

//! \file   suPatternGenerator.cpp
//! \brief  A collection of methods of the class suSatRouter.

// std includes
#include <algorithm>
#include <fstream>
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
#include <suGenerator.h>
#include <suGeneratorManager.h>
#include <suGrid.h>
#include <suLayoutFunc.h>
#include <suLayoutLeaf.h>
#include <suLayoutNode.h>
#include <suLayer.h>
#include <suLayerManager.h>
#include <suOptionManager.h>
#include <suPattern.h>
#include <suPatternManager.h>
#include <suRectangle.h>
#include <suSatRouter.h>
#include <suStatic.h>
#include <suToken.h>
#include <suWire.h>
#include <suWireManager.h>

// module include
#include <suPatternGenerator.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  suPatternGenerator * suPatternGenerator::_instance = 0;
  
  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  //! destructor
  suPatternGenerator::~suPatternGenerator ()
  {
    for (const auto & iter : _grids) {
      suGrid * grid = iter;
      delete grid;
    }
    _grids.clear();

  } // end of suPatternGenerator::~suPatternGenerator


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
  void suPatternGenerator::generate_pattern_file (const std::string & filename)
  {
    // process specific
    const std::string & process_tech = suOptionManager::instance()->get_string_option ("process_technology", "");
    
    SUINFO(1) << "Generate pattern tech=" << process_tech << " file=" << filename << std::endl;
    
    sutype::tokens_t tokens = suOptionManager::instance()->token()->get_tokens ("Rule");
    SUINFO(1) << "Found " << tokens.size() << " layout rules." << std::endl;

    suStatic::create_parent_dir (filename);
    
    std::ofstream ofs (filename);
    
    if (!ofs.is_open()) {
      SUISSUE("Could not open file for writing") << ": " << filename << std::endl;
      SUASSERT (false, "");
    }

    SUASSERT (!_grids.empty(), "");
    
    // basic
    dump_patterns_forbid_via_overlap_ ();

    //anton June 12, 2018. Split pattern genertors by process technology
    if (process_tech.empty()) {
      SUASSERT (false, "Option process_tech is not set");
    }
    else if (process_tech.compare ("1274dX") == 0) {
      generate_pattern_file_1274dX_ (tokens);
    }
    else if (process_tech.compare ("1273d1") == 0) { 
      generate_pattern_file_1273d1_ (tokens);
    }
    else if (process_tech.compare ("1273d5") == 0) { 
      generate_pattern_file_1273d5_ (tokens);
    }
    else {
      SUASSERT (false, "Unexpected process technoly string: " << process_tech);
    }
    
    for (const auto & iter : _patterns) {
      
      suPattern * pattern = iter;
      pattern->dump (ofs);
      ofs << std::endl;
    }
    
    ofs.close();

    SUINFO(1) << "Written " << _patterns.size() << " patterns to file " << filename << std::endl;
    
    for (const auto & iter : _patterns) {
      suPattern * pattern = iter;
      delete pattern;
    }
    
    SUABORT;
    
  } // end of suPatternGenerator::generate_pattern_file

  void suPatternGenerator::generate_pattern_file_1274dX_ (const sutype::tokens_t & tokens)
  {
    // constants
    const bool ruleWorksForTheSameColors = true;

    // V1
    dump_patterns_VX_spacing_ (tokens, sutype::go_hor,  ruleWorksForTheSameColors, "V1_28", "");
    dump_patterns_VX_spacing_ (tokens, sutype::go_ver, !ruleWorksForTheSameColors, "V1_30", "");
    dump_patterns_VX_spacing_ (tokens, 2,               ruleWorksForTheSameColors, "V1_24", ""); // diagonal rule
    
    // V2
    dump_patterns_VX_spacing_ (tokens, sutype::go_ver,  ruleWorksForTheSameColors, "V2_28",  "");
    dump_patterns_VX_spacing_ (tokens, sutype::go_ver,  ruleWorksForTheSameColors, "V2_29",  "V2_01");
    dump_patterns_VX_spacing_ (tokens, sutype::go_hor, !ruleWorksForTheSameColors, "V2_30",  "");
    dump_patterns_VX_spacing_ (tokens, sutype::go_hor,  ruleWorksForTheSameColors, "V2_128", "");
    
    dump_patterns_V2_size_based_rules_ (tokens);

    dump_patterns_VX_25_ (tokens, "V2_25", ruleWorksForTheSameColors);

  } // end of generate_pattern_file_1274.x_

  void suPatternGenerator::generate_pattern_file_1273d1_ (const sutype::tokens_t & tokens)
  {
    // constants
    const bool sameLayer = true;
    const bool differentLayers = false;
    
    dump_patterns_VX_spacing_ (tokens, sutype::go_hor, sameLayer, "V1_28", "");
    dump_patterns_VX_spacing_ (tokens, sutype::go_ver, sameLayer, "V1_28", "");
    // use V2_25_ for via1 layer
    dump_patterns_VX_25_ (tokens, "V1_25", false/*all colors mode*/);

    dump_patterns_VX_spacing_ (tokens, sutype::go_hor, sameLayer, "V2_28", "");
    dump_patterns_VX_spacing_ (tokens, sutype::go_ver, sameLayer, "V2_28", "");

    dump_patterns_VX_spacing_ (tokens, sutype::go_hor, sameLayer, "V3_28", "");
    dump_patterns_VX_spacing_ (tokens, sutype::go_ver, sameLayer, "V3_28", "");
    dump_patterns_VX_spacing_ (tokens, 2,              sameLayer, "V3_24", "");

    dump_patterns_VX_spacing_ (tokens, sutype::go_hor, sameLayer, "V5_28", "");
    dump_patterns_VX_spacing_ (tokens, sutype::go_ver, sameLayer, "V5_28", "");

    dump_patterns_VX_spacing_2layers_ (tokens, 2,              differentLayers, "V2_32", "");
    dump_patterns_VX_spacing_2layers_ (tokens, sutype::go_ver, differentLayers, "V2_152", "");
    
  } // end of generate_pattern_file_1273d1_

  void suPatternGenerator::generate_pattern_file_1273d5_ (const sutype::tokens_t & tokens)
  {
    // constants
    const bool sameLayer = true;
    //const bool differentLayers = false;
    const bool ruleWorksForTheSameColors = false;
    
    dump_patterns_VX_spacing_ (tokens, sutype::go_hor, sameLayer, "V1_28", "");
    dump_patterns_VX_spacing_ (tokens, sutype::go_ver, sameLayer, "V2_28", "");
    
    dump_patterns_VX_25_ (tokens, "V1_25", ruleWorksForTheSameColors);
    
  } // end of suPatternGenerator::generate_pattern_file_1273d5_

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
  void suPatternGenerator::create_grids_ ()
  {
    SUASSERT (_grids.empty(), "");
    
    suSatRouter::instance()->create_grids_for_generators (_grids);
    
  } // end of suPatternGenerator::create_grids_

  //
  bool suPatternGenerator::find_the_rule_ (const sutype::tokens_t & tokens,
                                           const std::string & rulename,
                                           sutype::dcoord_t & rulevalue,
                                           const suLayer * & rulelayer)
    const
  {
    rulevalue = 0;
    rulelayer = 0;
    bool rulefound = false;

    for (const auto & iter : tokens) {
      const suToken * token = iter;
      if (!token->is_defined("name")) continue;
      const std::string & name = token->get_string_value ("name");
      if (name.compare (rulename) != 0) continue;
      rulefound = true;
      SUASSERT (token->is_defined("value"), "");
      rulevalue = token->get_integer_value ("value");
      SUASSERT (token->is_defined("layer"), "");
      const std::string & layername = token->get_string_value ("layer");
      rulelayer = suLayerManager::instance()->get_base_layer_by_name (layername);
      SUASSERT (rulelayer, "");
    }

    if (!rulefound) {
      SUISSUE("Rule was not found") << ": " << rulename << std::endl;
      return rulefound;
    }

    SUASSERT (rulevalue > 0, "");
    SUASSERT (rulelayer, "");
    SUASSERT (rulelayer->is_base(), "");  
    
    return rulefound;
    
  } // end of suPatternGenerator::find_the_rule_

  //
  bool suPatternGenerator::find_the_rule_ (const sutype::tokens_t & tokens,
                                           const std::string & rulename,
                                           sutype::dcoord_t & rulevalue,
                                           const suLayer * & rulelayer1,
                                           const suLayer * & rulelayer2)
    const
  {
    rulevalue = 0;
    rulelayer1 = 0;
    rulelayer2 = 0;
    bool rulefound = false;

    for (const auto & iter : tokens) {
      const suToken * token = iter;
      if (!token->is_defined("name")) continue;
      const std::string & name = token->get_string_value ("name");
      if (name.compare (rulename) != 0) continue;
      rulefound = true;
      SUASSERT (token->is_defined("value"), "");
      rulevalue = token->get_integer_value ("value");
      {
        SUASSERT (token->is_defined("layer1"), "");
        const std::string & layername1 = token->get_string_value ("layer1");
        rulelayer1 = suLayerManager::instance()->get_base_layer_by_name (layername1);
        SUASSERT (rulelayer1, "");
      }
      {
        SUASSERT (token->is_defined("layer2"), "");
        const std::string & layername2 = token->get_string_value ("layer2");
        rulelayer2 = suLayerManager::instance()->get_base_layer_by_name (layername2);
        SUASSERT (rulelayer2, "");
      }
    }

    if (!rulefound) {
      SUISSUE("Rule was not found") << ": " << rulename << std::endl;
      return rulefound;
    }

    SUASSERT (rulevalue > 0, "");
    SUASSERT (rulelayer1, "");
    SUASSERT (rulelayer1->is_base(), "");  
    SUASSERT (rulelayer2, "");
    SUASSERT (rulelayer2->is_base(), "");  
   
    return rulefound;
    
  } // end of suPatternGenerator::find_the_rule_

  // forbid any overlap of vias
  // two vias can't occupy the same point even common via shape is still legal
  void suPatternGenerator::dump_patterns_forbid_via_overlap_ ()
  {
    std::string pbn ("Via_overlap");

    for (const auto & iter1 : _grids) {

      suGrid * grid = iter1;
      //grid->print ();

      sutype::svi_t gxperiod = grid->get_x_gperiod ();
      sutype::svi_t gyperiod = grid->get_y_gperiod ();

      for (sutype::svi_t gxli1 = 0; gxli1 < gxperiod; ++gxli1) {
        
        for (sutype::svi_t gyli1 = 0; gyli1 < gyperiod; ++gyli1) {
          
          const sutype::generators_tc & generators1 = grid->get_generators_at_gpoint (gxli1, gyli1);

          for (sutype::uvi_t i = 0; i < generators1.size(); ++i) {

            const suGenerator * generator1 = generators1[i];
            const suLayer * layer1 = generator1->get_cut_layer();
            suRectangle shape1 = grid->get_generator_shape (generator1, gxli1, gyli1);
            
            for (sutype::uvi_t k = i+1; k < generators1.size(); ++k) {

              sutype::svi_t gxli2 = gxli1;
              sutype::svi_t gyli2 = gyli1;
              
              const suGenerator * generator2 = generators1[k];
              const suLayer * layer2 = generator2->get_cut_layer();
              suRectangle shape2 = grid->get_generator_shape (generator2, gxli2, gyli2);
                                          
              if (layer1 != layer2) continue; // other patterns control this
              if (shape1.xl() == shape2.xl() && shape1.yl() == shape2.yl() && shape1.yl() == shape2.yl() && shape1.yh() == shape2.yh()) continue; // same shape

              suPattern * pattern = new suPattern ();
              
              pattern->name (
                             pbn
                             + "_"
                             + layer1->name() + "_" + suStatic::dcoord_to_str(shape1.w()) + "x" + suStatic::dcoord_to_str(shape1.h())
                             + "_"
                             + layer2->name() + "_" + suStatic::dcoord_to_str(shape2.w()) + "x" + suStatic::dcoord_to_str(shape2.h())
                             );

              pattern->shortname (pbn);
              
              pattern->comment ("Forbid useless overlap of vias");
              
              suLayoutFunc * layoutfunc0 = suLayoutNode::create_func (sutype::logic_func_and, sutype::unary_sign_just);
              
              pattern->layoutFunc (layoutfunc0);
              
              layoutfunc0->add_leaf (suPatternManager::instance()->create_wire_index_for_via_cut (grid, generator1, gxli1, gyli1));
              layoutfunc0->add_leaf (suPatternManager::instance()->create_wire_index_for_via_cut (grid, generator2, gxli2, gyli2));
              
              add_pattern_if_unique_ (pattern);
            }
          }
        }
      }
    }
    
  } // end of suPatternGenerator::dump_patterns_forbid_via_overlap_

  //
  void suPatternGenerator::dump_patterns_V2_size_based_rules_ (const sutype::tokens_t & tokens)
  {
    const int numrules = 4;
    const suLayer * rulelayer [numrules];
    bool rulefound            [numrules];
    sutype::dcoord_t v2_421, v2_422, v2_424, v2_425;

    int r = 0;
    rulefound[r] = find_the_rule_ (tokens, "V2_421", v2_421, rulelayer[r]); ++r;
    rulefound[r] = find_the_rule_ (tokens, "V2_422", v2_422, rulelayer[r]); ++r;
    rulefound[r] = find_the_rule_ (tokens, "V2_424", v2_424, rulelayer[r]); ++r;
    rulefound[r] = find_the_rule_ (tokens, "V2_425", v2_425, rulelayer[r]); ++r;
    SUASSERT (r == numrules, "");

    for (int i=0; i < numrules; ++i) {
      SUASSERT (rulefound[i], "");
      SUASSERT (i == 0 || rulelayer[i] == rulelayer[i-1], "");
    }

    const suLayer * ruleLayer = rulelayer[0];
    
    const char sizeS = 'S';
    const char sizeM = 'M';
    const char sizeL = 'L';

    for (const auto & iter1 : _grids) {

      suGrid * grid = iter1;
      //grid->print ();

      sutype::svi_t gxperiod = grid->get_x_gperiod () * 2;
      sutype::svi_t gyperiod = grid->get_y_gperiod () * 2;

      for (sutype::svi_t gxli1 = 0; gxli1 < gxperiod; ++gxli1) {
        
        for (sutype::svi_t gyli1 = 0; gyli1 < gyperiod; ++gyli1) {
          
          const sutype::generators_tc & generators1 = grid->get_generators_at_gpoint (gxli1, gyli1);

          sutype::dcoordpairs_t coords2;
          coords2.push_back (std::make_pair (gxli1 + 1, gyli1 + 1));
          coords2.push_back (std::make_pair (gxli1 + 1, gyli1 - 1));
          coords2.push_back (std::make_pair (gxli1 - 1, gyli1 + 1));
          coords2.push_back (std::make_pair (gxli1 - 1, gyli1 - 1));
          
          for (const auto & iter2 : coords2) {

            sutype::svi_t gxli2 = iter2.first;
            sutype::svi_t gyli2 = iter2.second;
          
            const sutype::generators_tc & generators2 = grid->get_generators_at_gpoint (gxli2, gyli2);
            
            for (sutype::uvi_t i = 0; i < generators1.size(); ++i) {

              const suGenerator * generator1 = generators1[i];
              const suLayer * layer1 = generator1->get_cut_layer();
              if (layer1->base() != ruleLayer) continue;

              suRectangle shape1 = grid->get_generator_shape (generator1, gxli1, gyli1);

              char size1 = sizeM;
              if      (shape1.w() <= v2_421) size1 = sizeS;
              else if (shape1.w() >  v2_422) size1 = sizeL;

              for (sutype::uvi_t k = 0; k < generators2.size(); ++k) {

                const suGenerator * generator2 = generators2[k];
                const suLayer * layer2 = generator2->get_cut_layer();
                if (layer2->base() != ruleLayer) continue;
              
                if (layer1 != layer2) continue;

                suRectangle shape2 = grid->get_generator_shape (generator2, gxli2, gyli2);

                char size2 = sizeM;
                if      (shape2.w() <= v2_421) size2 = sizeS;
                else if (shape2.w() >  v2_422) size2 = sizeL;
              
                // distance_to returns >= 0 only
                sutype::dcoord_t spacex = shape2.distance_to (shape1, sutype::go_hor);
                sutype::dcoord_t spacey = shape2.distance_to (shape1, sutype::go_ver);

                SUASSERT (spacex > 0, "");
                SUASSERT (spacey > 0, "");

                std::string pbn = ""; // pattern base name

                // v2_424
                if (spacex > 0 &&
                    spacey > 0 &&
                    spacex < v2_424 &&
                    spacey < v2_424 &&
                    (size1 == sizeS || size2 == sizeS) &&
                    (spacex * spacex + spacey * spacey < v2_424 * v2_424)) {

                  pbn = "V2_424";
                }

                // v2_425
                if (spacex > 0 &&
                    spacey > 0 &&
                    spacex < v2_425 &&
                    spacey < v2_425 &&
                    (size1 == sizeM || size2 == sizeM) &&
                    (spacex * spacex + spacey * spacey < v2_425 * v2_425)) {
                
                  pbn = "V2_425";
                }
              
                if (pbn.empty()) continue;
              
                suPattern * pattern = new suPattern ();

                pattern->name (
                               pbn
                               + "_"
                               + layer1->name() + "_" + suStatic::dcoord_to_str(shape1.w()) + "x" + suStatic::dcoord_to_str(shape1.h())
                               + "_"
                               + layer2->name() + "_" + suStatic::dcoord_to_str(shape2.w()) + "x" + suStatic::dcoord_to_str(shape2.h())
                               + "_spacex_" + suStatic::dcoord_to_str(spacex)
                               + "_spacey_" + suStatic::dcoord_to_str(spacey)
                               );

                pattern->shortname (pbn);

                pattern->comment ("Size-based spacing rule");
              
                suLayoutFunc * layoutfunc0 = suLayoutNode::create_func (sutype::logic_func_and, sutype::unary_sign_just);

                pattern->layoutFunc (layoutfunc0);
                
                layoutfunc0->add_leaf (suPatternManager::instance()->create_wire_index_for_via_cut (grid, generator1, gxli1, gyli1));
                layoutfunc0->add_leaf (suPatternManager::instance()->create_wire_index_for_via_cut (grid, generator2, gxli2, gyli2));
                
                add_pattern_if_unique_ (pattern);
              }
            }
          }
        }
      }
    }
    
  } // end of suPatternGenerator::dump_patterns_V2_size_based_rules_
  
  // spacing between vias
  void suPatternGenerator::dump_patterns_VX_spacing_ (const sutype::tokens_t & tokens,
                                                      const int rulegd,
                                                      const bool ruleWorksForTheSameColors,
                                                      const std::string & rulename1,
                                                      const std::string & rulename2)
  {
    sutype::dcoord_t rulevalue1 = 0;
    const suLayer * rulelayer1 = 0;
    bool rulefound1 = find_the_rule_ (tokens, rulename1, rulevalue1, rulelayer1);
    SUASSERT (rulefound1, rulename1);

    sutype::dcoord_t rulevalue2 = 0;
    const suLayer * rulelayer2 = 0;
    if (!rulename2.empty()) {
      bool rulefound2 = find_the_rule_ (tokens, rulename2, rulevalue2, rulelayer2);
      SUASSERT (rulefound2, "");
      SUASSERT (rulelayer1 == rulelayer2, "");
    }

    const suLayer * rulelayer = rulelayer1;
    SUASSERT (rulelayer->is_base(), "");

    const sutype::dcoord_t sqr_rulevalue1 = rulevalue1 * rulevalue1;

    unsigned counter = 0;
    
    //Anton June 20 12018. Store pattern keys for faster search
    std::set<std::string> patternKeys;
        
    for (const auto & iter1 : _grids) {
      suGrid * grid = iter1;
      if (grid->get_generators_of_base_layer(rulelayer).empty()) continue; // grid has no vias
      
      sutype::svi_t gxperiod = grid->get_x_gperiod () * 4;
      sutype::svi_t gyperiod = grid->get_y_gperiod () * 2;
      
      //  SUINFO(1) << " gxperiod=" << gxperiod << "; gyperiod=" << gyperiod << " total grids=" << _grids.size() << std::endl;

      for (sutype::svi_t gxli1 = 0; gxli1 < gxperiod; ++gxli1) {
        
        for (sutype::svi_t gxli2 = 0; gxli2 < gxperiod; ++gxli2) {

          if (rulegd == (int)sutype::go_ver && gxli2 != gxli1) continue;
          
          for (sutype::svi_t gyli1 = 0; gyli1 < gyperiod; ++gyli1) {
            
            for (sutype::svi_t gyli2 = 0; gyli2 < gyperiod; ++gyli2) {
              
              if (rulegd == (int)sutype::go_hor && gyli2 != gyli1) continue;
              
              const sutype::generators_tc & generators1 = grid->get_generators_at_gpoint (gxli1, gyli1);
              const sutype::generators_tc & generators2 = grid->get_generators_at_gpoint (gxli2, gyli2);
              
              for (sutype::uvi_t i = 0; i < generators1.size(); ++i) {
                
                const suGenerator * generator1 = generators1[i];
                const suLayer * layer1 = generator1->get_cut_layer();
                if (layer1->base() != rulelayer) continue;
                
                suRectangle shape1 = grid->get_generator_shape (generator1, gxli1, gyli1);

                // variations of the rule
                if (!rulename2.empty() && shape1.h() != rulevalue2) continue;
                //

                for (sutype::uvi_t k = 0; k < generators2.size(); ++k) {

                  const suGenerator * generator2 = generators2[k];
                  const suLayer * layer2 = generator2->get_cut_layer();
                  if (layer2->base() != rulelayer) continue;

                  // check colors
                  if (ruleWorksForTheSameColors) {
                    if (layer1 != layer2) continue;
                  }
                  else {
                    if (layer1 == layer2) continue;
                  }
                  //
                  
                  suRectangle shape2 = grid->get_generator_shape (generator2, gxli2, gyli2);

                  // variations of the rule
                  if (!rulename2.empty() && shape2.h() != rulevalue2) continue;
                  //
                  
                  // distance_to returns >= 0 only
                  sutype::dcoord_t space = 0;

                  // ver or hor
                  if (rulegd == (int)sutype::go_ver || rulegd == (int)sutype::go_hor) {
                    
                    space = shape2.distance_to (shape1, (sutype::grid_orientation_t)rulegd);
                    if (space == 0 || space >= rulevalue1) continue;
                  }

                  // diagonal
                  else {

                    // "2" is used to model diagonal spacing
                    SUASSERT (rulegd == 2, "");
                    
                    sutype::dcoord_t xspace = shape2.distance_to (shape1, sutype::go_hor);
                    sutype::dcoord_t yspace = shape2.distance_to (shape1, sutype::go_ver);
                    if (xspace >= rulevalue1) continue;
                    if (yspace >= rulevalue1) continue;
                    if (xspace == 0) continue;
                    if (yspace == 0) continue;
                    if (xspace * xspace + yspace * yspace >= sqr_rulevalue1) continue;
                  }
                   
                  //Anton June 20 2018. Avoid duplicate patterns. It is faster than add_pattern_if_unique_
                  const std::string patternKey = layer1->name() 
                                 + "_" + suStatic::dcoord_to_str(shape1.w()) 
                                 + "x" + suStatic::dcoord_to_str(shape1.h())
                                 + "_"
                                 + layer2->name() + "_" + suStatic::dcoord_to_str(shape2.w()) 
                                 + "x" + suStatic::dcoord_to_str(shape2.h())
                                 + "_space_" + suStatic::dcoord_to_str(space);
                  if (patternKeys.find(patternKey) != patternKeys.end()) continue;

                  patternKeys.insert(patternKey);
                  
                  suPattern * pattern = new suPattern ();
                  
                  pattern->name (
                                 rulename1
                                 + "_"
                                 + layer1->name() + "_" + suStatic::dcoord_to_str(shape1.w()) + "x" + suStatic::dcoord_to_str(shape1.h())
                                 + "_"
                                 + layer2->name() + "_" + suStatic::dcoord_to_str(shape2.w()) + "x" + suStatic::dcoord_to_str(shape2.h())
                                 + "_space_" + suStatic::dcoord_to_str(space)
                                 + "_" + suStatic::dcoord_to_str(counter)
                                 );

                  if (!rulename2.empty()) {
                    pattern->shortname (rulename2);
                  }
                  else {
                    pattern->shortname (rulename1);
                  }

                  std::string comment = "Min spacing between " + layer1->name() + " vias";

                  // variations of the rule
                  if (!rulename2.empty()) {
                    comment += (" if both are " + rulename2);
                  }
                  //
                  
                  pattern->comment (comment);
                  
                  suLayoutFunc * layoutfunc0 = suLayoutNode::create_func (sutype::logic_func_and, sutype::unary_sign_just);

                  pattern->layoutFunc (layoutfunc0);
                  
                  layoutfunc0->add_leaf (suPatternManager::instance()->create_wire_index_for_via_cut (grid, generator1, gxli1, gyli1));
                  layoutfunc0->add_leaf (suPatternManager::instance()->create_wire_index_for_via_cut (grid, generator2, gxli2, gyli2));
                  bool added = add_pattern_if_unique_ (pattern);
                  if (added)
                    ++counter;
                }
              }
            }
          } // for(gyli2)
        } // for(gyli1)
      } // for(gxli2)
    } // for(gxli1)
    
    SUINFO(1) << "Written min spacing rule " << rulename1 << " count=" << counter << std::endl;

    
  } // end of suPatternGenerator::dump_patterns_VX_spacing_
  // spacing between vias
  void suPatternGenerator::dump_patterns_VX_spacing_2layers_ (const sutype::tokens_t & tokens,
                                                              const int rulegd,
                                                              const bool ruleWorksForTheSameColors,
                                                              const std::string & rulename1,
                                                              const std::string & rulename2)
  {
    sutype::dcoord_t rulevalue1 = 0;
    const suLayer * rulelayer1_1 = 0;
    const suLayer * rulelayer1_2 = 0;
    bool rulefound1 = find_the_rule_ (tokens, rulename1, rulevalue1, rulelayer1_1, rulelayer1_2);
    SUASSERT (rulefound1, "");

    sutype::dcoord_t rulevalue2 = 0;
    {
      const suLayer * rulelayer2 = 0;
      if (!rulename2.empty()) {
        bool rulefound2 = find_the_rule_ (tokens, rulename2, rulevalue2, rulelayer2);
        SUASSERT (rulefound2, "");
        SUASSERT (rulelayer1_1 == rulelayer2, ""); // value is for layer 1_1
      }
    }

    SUASSERT (rulelayer1_1->is_base(), "");
    SUASSERT (rulelayer1_2->is_base(), "");

    const sutype::dcoord_t sqr_rulevalue1 = rulevalue1 * rulevalue1;

    unsigned counter = 0;
        
    for (const auto & iter1 : _grids) {

      suGrid * grid1 = iter1;
      if (grid1->get_generators_of_base_layer(rulelayer1_1).empty()) continue; // grid has no vias
    for (const auto & iter2 : _grids) {
      suGrid * grid2 = iter2;
      if (grid2->get_generators_of_base_layer(rulelayer1_2).empty()) continue; // grid has no vias
      
      sutype::svi_t gxperiod = std::max(grid1->get_x_gperiod (), grid2->get_x_gperiod ()) * 4;
      sutype::svi_t gyperiod = std::max(grid1->get_y_gperiod (), grid2->get_y_gperiod ()) * 2;
      
      //SUINFO(1) << "gxperiod=" << gxperiod << "; gyperiod=" << gyperiod << std::endl;

      for (sutype::svi_t gxli1 = 0; gxli1 < gxperiod; ++gxli1) {
        
        for (sutype::svi_t gxli2 = 0; gxli2 < gxperiod; ++gxli2) {

          if (rulegd == (int)sutype::go_ver && gxli2 != gxli1) continue;
          
          for (sutype::svi_t gyli1 = 0; gyli1 < gyperiod; ++gyli1) {
            
            for (sutype::svi_t gyli2 = 0; gyli2 < gyperiod; ++gyli2) {
              
              if (rulegd == (int)sutype::go_hor && gyli2 != gyli1) continue;
              
              const sutype::generators_tc & generators1 = grid1->get_generators_at_gpoint (gxli1, gyli1);
              const sutype::generators_tc & generators2 = grid2->get_generators_at_gpoint (gxli2, gyli2);
              
              for (sutype::uvi_t i = 0; i < generators1.size(); ++i) {
                
                const suGenerator * generator1 = generators1[i];
                const suLayer * layer1 = generator1->get_cut_layer();
                if (layer1->base() != rulelayer1_1) continue;
                
                suRectangle shape1 = grid1->get_generator_shape (generator1, gxli1, gyli1);

                // variations of the rule
                if (!rulename2.empty() && shape1.h() != rulevalue2) continue;
                //

                for (sutype::uvi_t k = 0; k < generators2.size(); ++k) {

                  const suGenerator * generator2 = generators2[k];
                  const suLayer * layer2 = generator2->get_cut_layer();
                  if (layer2->base() != rulelayer1_2) continue;

                  // check colors
                  if (ruleWorksForTheSameColors) {
                    if (layer1 != layer2) continue;
                  }
                  else {
                    if (layer1 == layer2) continue;
                  }
                  //
                  
                  suRectangle shape2 = grid2->get_generator_shape (generator2, gxli2, gyli2);

                  // variations of the rule
                  if (!rulename2.empty() && shape2.h() != rulevalue2) continue;
                  //
                  
                  // distance_to returns >= 0 only
                  sutype::dcoord_t space = 0;

                  // ver or hor
                  if (rulegd == (int)sutype::go_ver || rulegd == (int)sutype::go_hor) {
                    
                    space = shape2.distance_to (shape1, (sutype::grid_orientation_t)rulegd);
                    if (space == 0 || space >= rulevalue1) continue;
                  }

                  // diagonal
                  else {

                    // "2" is used to model diagonal spacing
                    SUASSERT (rulegd == 2, "");
                    
                    sutype::dcoord_t xspace = shape2.distance_to (shape1, sutype::go_hor);
                    sutype::dcoord_t yspace = shape2.distance_to (shape1, sutype::go_ver);
                    if (xspace >= rulevalue1) continue;
                    if (yspace >= rulevalue1) continue;
                    if (xspace == 0) continue;
                    if (yspace == 0) continue;
                    if (xspace * xspace + yspace * yspace >= sqr_rulevalue1) continue;
                  }

                  suPattern * pattern = new suPattern ();
                  
                  pattern->name (
                                 rulename1
                                 + "_"
                                 + layer1->name() + "_" + suStatic::dcoord_to_str(shape1.w()) + "x" + suStatic::dcoord_to_str(shape1.h())
                                 + "_"
                                 + layer2->name() + "_" + suStatic::dcoord_to_str(shape2.w()) + "x" + suStatic::dcoord_to_str(shape2.h())
                                 + "_space_" + suStatic::dcoord_to_str(space)
                                 + "_" + suStatic::dcoord_to_str(counter)
                                 );

                  if (!rulename2.empty()) {
                    pattern->shortname (rulename2);
                  }
                  else {
                    pattern->shortname (rulename1);
                  }

                  std::string comment = "Min spacing between " + layer1->name() + " " + layer2->name() + " vias";

                  // variations of the rule
                  if (!rulename2.empty()) {
                    comment += (" if both are " + rulename2);
                  }
                  //
                  
                  pattern->comment (comment);
                  
                  suLayoutFunc * layoutfunc0 = suLayoutNode::create_func (sutype::logic_func_and, sutype::unary_sign_just);

                  pattern->layoutFunc (layoutfunc0);
                  
                  layoutfunc0->add_leaf (suPatternManager::instance()->create_wire_index_for_via_cut (grid1, generator1, gxli1, gyli1));
                  layoutfunc0->add_leaf (suPatternManager::instance()->create_wire_index_for_via_cut (grid2, generator2, gxli2, gyli2));
                  
                  bool added = add_pattern_if_unique_ (pattern);
                  if (added)
                    ++counter;
                }
              }
            }
          } // for(gyli2)
        } // for(gyli1)
      } // for(gxli2)
    }
    } // for(gxli1)

    
  } // end of suPatternGenerator::dump_patterns_VX_spacing_

  //
  void suPatternGenerator::dump_patterns_VX_25_ (const sutype::tokens_t & tokens,
                                                 const std::string & rulename,
                                                 const bool ruleWorksForTheSameColors)
  {
    int xskiptrack = 1;
    int yskiptrack = 1;

    sutype::dcoords_t activemodes;
    activemodes.push_back (1);
    activemodes.push_back (2);
    activemodes.push_back (3);
    activemodes.push_back (4);
    //activemodes.push_back (5);
    activemodes.push_back (6);
    
    sutype::dcoord_t rulevalue = 0;
    const suLayer * rulelayer = 0;
    bool rulefound = find_the_rule_ (tokens, rulename, rulevalue, rulelayer);
    SUASSERT (rulefound, "");
    SUASSERT (rulelayer, "");
    SUASSERT (rulelayer->is_base(), "");

    sutype::layers_tc colors;

    if (ruleWorksForTheSameColors) {
      colors = suLayerManager::instance()->get_colors (rulelayer);
      SUASSERT (!colors.empty(), "Found no colors for layer " << rulelayer->to_str());
    }
    else {
      colors.push_back (rulelayer);
    }
    
    sutype::uvi_t counter = 0;
    
    for (const auto & iter1 : _grids) {

      suGrid * grid = iter1;
      if (grid->get_generators_of_base_layer(rulelayer).empty()) continue; // grid has no vias
      
      sutype::svi_t gxperiod = grid->get_x_gperiod () * 2 * xskiptrack;
      sutype::svi_t gyperiod = grid->get_y_gperiod () * 2 * yskiptrack;

      // collect skip-track triples of vias
      for (sutype::svi_t x1 = 0; x1 < gxperiod; ++x1) {
        for (sutype::svi_t y1 = 0; y1 < gyperiod; ++y1) {
          
          for (const auto & iter2 : activemodes) {

            int mode = iter2;
            
            sutype::dcoordpairs_t gpoints;
 
            if (mode == 1) {

              sutype::svi_t x2 = x1 + 1 * xskiptrack;
              sutype::svi_t y2 = y1 + 1 * yskiptrack;
              sutype::svi_t y3 = y2 + 1 * yskiptrack;
              
              // 3   o
              // 2 o
              // 1   o
              //   1 2
              gpoints.push_back (std::make_pair (x1, y2));
              gpoints.push_back (std::make_pair (x2, y1));
              gpoints.push_back (std::make_pair (x2, y3));
            }
            else if (mode == 2) {

              sutype::svi_t x2 = x1 + 1 * xskiptrack;
              sutype::svi_t y2 = y1 + 1 * yskiptrack;
              sutype::svi_t y3 = y1 + 2 * yskiptrack;
              
              // 3 o
              // 2   o
              // 1 o
              //   1 2
              gpoints.push_back (std::make_pair (x1, y1));
              gpoints.push_back (std::make_pair (x2, y2));
              gpoints.push_back (std::make_pair (x1, y3));
            }
            else if (mode == 3) {

              sutype::svi_t x2 = x1 + 1 * xskiptrack;
              sutype::svi_t x3 = x1 + 2 * xskiptrack;
              sutype::svi_t y2 = y1 + 1 * yskiptrack;

              // 2   o
              // 1 o   o 
              //   1 2 3
              gpoints.push_back (std::make_pair (x1, y1));
              gpoints.push_back (std::make_pair (x2, y2));
              gpoints.push_back (std::make_pair (x3, y1));
            }
            else if (mode == 4) {

              sutype::svi_t x2 = x1 + 1 * xskiptrack;
              sutype::svi_t x3 = x1 + 2 * xskiptrack;
              sutype::svi_t y2 = y1 + 1 * yskiptrack;

              // 2 o   o
              // 1   o
              //   1 2 3
              gpoints.push_back (std::make_pair (x1, y2));
              gpoints.push_back (std::make_pair (x2, y1));
              gpoints.push_back (std::make_pair (x3, y2));
            }
            else if (mode == 5) {

              SUASSERT (false, "need review");

              sutype::svi_t x2 = x1 + 1;
              sutype::svi_t x3 = x1 + 2;
              sutype::svi_t y2 = y1 + 2;
              sutype::svi_t y3 = y1 + 3;
              
              // 3 o
              // 2   o
              //
              // 1  o
              //   123 
              gpoints.push_back (std::make_pair (x2, y1));
              gpoints.push_back (std::make_pair (x1, y3));
              gpoints.push_back (std::make_pair (x3, y2));
            }
            else if (mode == 6) {

              sutype::svi_t x2 = x1 + 1 * xskiptrack;
              sutype::svi_t y2 = y1 + 1 * yskiptrack;
              sutype::svi_t y3 = y2 + 1 * yskiptrack;
              
              // 3 o
              // 2 o
              // 1   o
              //   1 2
              gpoints.push_back (std::make_pair (x1, y2));
              gpoints.push_back (std::make_pair (x1, y3));
              gpoints.push_back (std::make_pair (x2, y1));
            }
            else {
              SUASSERT (false, "");
            }
          
            for (const auto & iter1 : colors) {

              const suLayer * layer1 = iter1;

              suPattern * pattern = new suPattern ();
              pattern->name (
                             rulename
                             + "_" + suStatic::dcoord_to_str(counter)
                             + "_mode_" + suStatic::dcoord_to_str(mode)
                             );

              pattern->shortname (rulename);
              
              std::string comment = "The isolated pair must be spaced from other " + layer1->name() + " >= " +  suStatic::dcoord_to_str(rulevalue);
              pattern->comment (comment);
              
              suLayoutFunc * layoutfunc0 = suLayoutNode::create_func (sutype::logic_func_and, sutype::unary_sign_just);
              pattern->layoutFunc (layoutfunc0);

              for (const auto & iter2 : gpoints) {
                
                sutype::gcoord_t gx = iter2.first;
                sutype::gcoord_t gy = iter2.second;

                const sutype::generators_tc & generators = grid->get_generators_at_gpoint (gx, gy);
                SUASSERT (!generators.empty(), "");
              
                suLayoutFunc * layoutfunc1 = layoutfunc0->add_func (sutype::logic_func_or, sutype::unary_sign_just);

                for (const auto & iter3 : generators) {
                
                  const suGenerator * generator = iter3;
                  if (generator->get_cut_layer() != layer1) continue;
                
                  layoutfunc1->add_leaf (suPatternManager::instance()->create_wire_index_for_via_cut (grid, generator, gx, gy));
                }

                SUASSERT (!layoutfunc1->nodes().empty(), "");
              }
            
              bool added = add_pattern_if_unique_ (pattern);
              if (added) ++counter;
            }
          }
        }
      }
    }
    
  } // end of suPatternGenerator::dump_patterns_VX_25_
    
  // \return true if added
  bool suPatternGenerator::add_pattern_if_unique_ (suPattern * pattern1)
  {
    const bool np = false;

    SUASSERT (pattern1, "");
    SUASSERT (pattern1->layoutFunc(), "");

    pattern1->layoutFunc()->make_canonical ();
    
    for (const auto & iter1 : _patterns) {
      
      suPattern * pattern0 = iter1;
      
      sutype::trs_t feasibletrs;
      if (!patterns_are_identical_ (pattern0, pattern1, feasibletrs)) continue;

      if (np) {
        SUINFO(1) << "Found identical patterns: pattern0=" << pattern0->name() << ": pattern1=" << pattern1->name() << std::endl;
        SUINFO(1) << "Feasible transformations:" << std::endl;
        for (const auto & iter2 : feasibletrs) {
          SUINFO(1) << "  " << suStatic::tr_2_str (iter2) << std::endl;
        }
        pattern0->dump (std::cout);
        std::cout << std::endl;
        pattern1->dump (std::cout);
        std::cout << std::endl;
      }
      
      delete pattern1;
      return false;
    }

    if (np) {
      SUINFO(1) << "Added pattern " << pattern1->name() << std::endl;
      pattern1->dump (std::cout);
      std::cout << std::endl;
    }
    
    _patterns.push_back (pattern1);

    // it does not affect anyhow default functionality anywhere
    // just it's a bit easier to read patterns in files and in log messages
    pattern1->shift_to_0x0 ();
    
    return true;
    
  } // end of add_pattern_if_unique_

  //
  bool suPatternGenerator::patterns_are_identical_ (suPattern * pattern0,
                                                    suPattern * pattern1,
                                                    sutype::trs_t & feasibletrs)
    const
  {
    SUASSERT (pattern0, "");
    SUASSERT (pattern1, "");
    SUASSERT (feasibletrs.empty(), "");

    suLayoutFunc * func0 = pattern0->layoutFunc();
    suLayoutFunc * func1 = pattern1->layoutFunc();

    SUASSERT (func0, "");
    SUASSERT (func1, "");

    bool identical = funcs_are_identical_ (func0, func1, feasibletrs);
    SUASSERT (feasibletrs.empty() != identical, "");
    
    return identical;
    
  } // end of suPatternGenerator::patterns_are_identical_

  //
  bool suPatternGenerator::funcs_are_identical_ (suLayoutFunc * func0,
                                                 suLayoutFunc * func1,
                                                 sutype::trs_t & feasibletrs)
    const
  {
    // quick check
    if (func0->func() != func1->func()) return false;
    if (func0->sign() != func1->sign()) return false;
    if (func0->nodes().size() != func1->nodes().size()) return false;
    
    const int N = 2;
    const int T = 4;
    
    sutype::layoutfuncs_t funcs;
    funcs.push_back (func0);
    funcs.push_back (func1);
    
    sutype::layoutnodes_t nodesPerType[T][N];
    
    for (int n = 0; n < N; ++n) {

      suLayoutFunc * func = funcs[n];
      const sutype::layoutnodes_t & funcnodes = func->nodes();

      for (const auto & iter : funcnodes) {

        suLayoutNode * node = iter;
        
        if      (node->is_leaf() && node->to_leaf()->satindex() > 0)                                                                         nodesPerType[0][n].push_back (node);
        else if (node->is_leaf() && node->to_leaf()->satindex() < 0)                                                                         nodesPerType[1][n].push_back (node);
        else if (node->is_func() && node->to_func()->func() == sutype::logic_func_and && node->to_func()->sign() == sutype::unary_sign_just) nodesPerType[2][n].push_back (node);
        else if (node->is_func() && node->to_func()->func() == sutype::logic_func_or  && node->to_func()->sign() == sutype::unary_sign_just) nodesPerType[3][n].push_back (node);
        else {
          SUASSERT (false, "unexpected node");
        }
      }
    }

    // quick check
    for (int t = 0; t < T; ++t) {
      
      const sutype::layoutnodes_t & nodes0 = nodesPerType[t][0];
      const sutype::layoutnodes_t & nodes1 = nodesPerType[t][1];
      
      if (nodes0.size() != nodes1.size()) return false;
    }

    // accurate check
    for (int t = 0; t < T; ++t) {
      
      sutype::layoutnodes_t & nodes0 = nodesPerType[t][0];
      sutype::layoutnodes_t & nodes1 = nodesPerType[t][1];

      if (nodes0.empty()) continue;

      bool ret = nodes_are_identical_ (nodes0, nodes1, feasibletrs);
      if (!ret) return false;
    }
    
    return true;
    
  } // end of suPatternGenerator::funcs_are_identical_

  // collect possible transformations from wire1 to wire0
  // wire0 = wire1 + transformation
  bool suPatternGenerator::leaves_are_identical_ (suLayoutLeaf * leaf0,
                                                  suLayoutLeaf * leaf1,
                                                  sutype::trs_t & trs)
    const
  {
    SUASSERT (leaf0, "");
    SUASSERT (leaf1, "");

    sutype::satindex_t wireindex0 = leaf0->satindex();
    sutype::satindex_t wireindex1 = leaf1->satindex();

    SUASSERT (wireindex0 != 0, "");
    SUASSERT (wireindex1 != 0, "");
    SUASSERT ((wireindex0 > 0) == (wireindex1 > 0), "");

    wireindex0 = abs (wireindex0);
    wireindex1 = abs (wireindex1);

    suWire * wire0 = suPatternManager::instance()->get_wire (wireindex0);
    suWire * wire1 = suPatternManager::instance()->get_wire (wireindex1);

    if (wire0->layer() != wire1->layer()) return false;
    if (wire0->rect().w() != wire1->rect().w()) return false;
    if (wire0->rect().h() != wire1->rect().h()) return false;

    // I support following transformations:
    
    //  ref_0  ref_y
    //  +---   ---+
    //  |         |
    //  +--     --+
    //  |         |
    //  |         |
    //
    //  ref_x  ref_xy
    //  |         |
    //  |         |
    //  +--     --+
    //  |         |
    //  +---   ---+

    for (int i = 0; i < (int)sutype::ref_num_types; ++i) {
      
      const sutype::ref_t ref = (sutype::ref_t)i;
      
      sutype::dcoord_t dx  = 0;
      sutype::dcoord_t dy  = 0;
      
      // calculate dx & dy got given ref
      // wire0 = wire1 + transformation
      wire1->rect().calculate_transformation (wire0->rect(), dx, dy, ref);

      trs.push_back (sutype::tr_t (dx, dy, ref));
    }
    
    return true;
    
  } // end of suPatternGenerator::leaves_are_identical_

  //
  bool suPatternGenerator::nodes_are_identical_ (sutype::layoutnodes_t & nodes0,
                                                 sutype::layoutnodes_t & nodes1,
                                                 sutype::trs_t & feasibletrs)
    const
  {
    SUASSERT (nodes0.size() == nodes1.size(), "");

    std::map<sutype::tr_t,sutype::dcoordpairs_t> possibleTransformations;
    
    for (sutype::uvi_t i=0; i < nodes0.size(); ++i) {

      suLayoutNode * node0 = nodes0[i];
      
      for (sutype::uvi_t k=0; k < nodes1.size(); ++k) {

        suLayoutNode * node1 = nodes1[k];

        sutype::trs_t possibletrs;
        get_possible_transformations_ (node0, node1, possibletrs);

        // store
        for (const auto & iter : possibletrs) {
          const sutype::tr_t & tr = iter;
          if (!feasibletrs.empty() && std::find (feasibletrs.begin(), feasibletrs.end(), tr) == feasibletrs.end()) continue;
          possibleTransformations[tr].push_back (std::make_pair (i, k));
        }
      }
    }

    if (possibleTransformations.empty()) return false;
    
    // populate feasibletrs from scratch
    feasibletrs.clear ();

    // a transformation is possible if there's a complete chain of nodes, e.g. 0-1, 1-0, 2-2
    for (const auto & iter1 : possibleTransformations) {
      
      const sutype::tr_t & tr = iter1.first;
      const sutype::dcoordpairs_t & pairsOfNodes = iter1.second;
      
      std::vector<bool> used0 (nodes0.size(), false);
      std::vector<bool> used1 (nodes1.size(), false);
      sutype::uvi_t chainLength = 0;
      
      bool ok = find_chain_of_nodes_ (pairsOfNodes, 0, used0, used1, chainLength);
      if (!ok) continue;
      
      feasibletrs.push_back (tr);
    }
    
    if (feasibletrs.empty()) return false;
    
    return true;
    
  } // end of suPatternGenerator::nodes_are_identical_

  //
  bool suPatternGenerator::find_chain_of_nodes_ (const sutype::dcoordpairs_t & pairsOfNodes,
                                                 const sutype::uvi_t startIndex,
                                                 std::vector<bool> & used0,
                                                 std::vector<bool> & used1,
                                                 sutype::uvi_t & chainLength)
    const
  {
    SUASSERT (!used0.empty(), "");
    SUASSERT (startIndex <= pairsOfNodes.size(), "");
    SUASSERT (chainLength <= used0.size(), "");

    // found a complete chain
    if (chainLength == used0.size()) return true;
    
    for (sutype::uvi_t i = startIndex; i < pairsOfNodes.size(); ++i) {

      sutype::svi_t nodeindex0 = pairsOfNodes[i].first;
      sutype::svi_t nodeindex1 = pairsOfNodes[i].second;

      if (used0[nodeindex0]) continue;
      if (used1[nodeindex1]) continue;

      used0 [nodeindex0] = true;
      used1 [nodeindex1] = true;
      ++chainLength;
      
      bool ok = find_chain_of_nodes_ (pairsOfNodes, i+1, used0, used1, chainLength);
      if (ok) return ok;

      // restore
      --chainLength;
      used0 [nodeindex0] = false;
      used1 [nodeindex1] = false;
    }

    return false;
    
  } // end of suPatternGenerator::find_chain_of_nodes_

  // collect possible transformations from node1 to node0
  // wire0 = wire1 + transformation
  void suPatternGenerator::get_possible_transformations_ (suLayoutNode * node0,
                                                          suLayoutNode * node1,
                                                          sutype::trs_t & possibletrs)
    const
  {
    SUASSERT (node0->is_leaf() == node1->is_leaf(), "");
    
    if (node0->is_leaf()) {
      leaves_are_identical_ (node0->to_leaf(), node1->to_leaf(), possibletrs);
    }
    else {
      funcs_are_identical_  (node0->to_func(), node1->to_func(), possibletrs);
    }
    
  } // end of suPatternGenerator::get_possible_transformations_
  
} // end of namespace amsr

// end of suPatternGenerator.cpp

