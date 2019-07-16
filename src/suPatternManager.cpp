// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Nov 14 12:04:34 2017

//! \file   suPatternManager.cpp
//! \brief  A collection of methods of the class suPatternManager.

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
#include <suClauseBank.h>
#include <suErrorManager.h>
#include <suGenerator.h>
#include <suGrid.h>
#include <suLayerManager.h>
#include <suLayerManager.h>
#include <suLayoutFunc.h>
#include <suPattern.h>
#include <suPatternInstance.h>
#include <suSatRouter.h>
#include <suStatic.h>
#include <suToken.h>
#include <suTokenParser.h>
#include <suWire.h>
#include <suWireManager.h>

// module include
#include <suPatternManager.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  suPatternManager * suPatternManager::_instance = 0;


  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  suPatternManager::~suPatternManager ()
  {
    for (const auto & iter : _patterns)
      delete iter;

    for (const auto & iter1 : _patternIdToInstances) {
      const sutype::patterninstances_tc & pis = iter1;
      for (const auto & iter2 : pis) {
        delete iter2;
      }
    }
    
    SUASSERT (!_wires.empty(), "");
    SUASSERT (_wires.front() == 0, "");
    
    for (sutype::uvi_t i=1; i < _wires.size(); ++i) {
      suWire * wire = _wires[i];
      suWireManager::instance()->release_wire (wire);
    }
    
  } // end of suPatternManager::~suPatternManager


  // ------------------------------------------------------------
  // -
  // --- Public static methods
  // -
  // ------------------------------------------------------------

  // static
  void suPatternManager::delete_instance ()
  {
    if (suPatternManager::_instance)
      delete suPatternManager::_instance;
  
    suPatternManager::_instance = 0;
    
  } // end of suPatternManager::delete_instance


  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  //
  void suPatternManager::create_raw_pattern_instances (const suWire * wire)
  {
    SUASSERT (wire, "");
    
    const suLayer * layer = wire->layer();
    SUASSERT (layer, "");

    if (_patternsPerLayer.count (layer) == 0) return;

    const sutype::patterns_tc & patterns = _patternsPerLayer.at (layer);

    for (const auto & iter : patterns) {
      
      const suPattern * pattern = iter;
      
      pattern->create_pattern_instances (wire, _patternIdToInstances [pattern->id()]);
    }
    
  } // end of suPatternManager::create_raw_pattern_instances

  //
  void suPatternManager::delete_incomplete_pattern_instances ()
  {
    sutype::uvi_t num = 0;

    for (sutype::uvi_t k = 0; k < _patternIdToInstances.size(); ++k) {

      sutype::patterninstances_tc & pis = _patternIdToInstances[k];
      
      for (sutype::svi_t i = 0; i < (sutype::svi_t)pis.size(); ++i) {
        
        const suPatternInstance * pi = pis[i];

//         // debug
//         if (pi->pattern()->id() == 101 &&
//             pi->matches (13680, 1760, sutype::ref_xy)) {
//           SUINFO(1) << "Found pi: " << pi->to_str() << std::endl;
//           pi->print_pattern_instance (std::cout);
//           SUABORT;
//         }
//         //

        if (!pi->layoutFunc()->has_dangling_funcs()) continue;
        
        pis[i] = pis.back();
        pis.pop_back();
        delete pi;
        --i;
        ++num;
      }

      std::sort (pis.begin(), pis.end(), suStatic::compare_pattern_instances_by_id);
    }

    if (num != 0) {
      SUINFO(1) << "Deleted " << num << " incomplete pattern instances." << std::endl;
    }
    
  } // end of suPatternManager::delete_incomplete_pattern_instances

  //
  void suPatternManager::delete_redundant_pattern_instances ()
  {
    sutype::uvi_t num0 = 0;
    sutype::uvi_t num1 = 0;

    // simplify & delete constant-0 as useless
    for (sutype::uvi_t p = 0; p < _patternIdToInstances.size(); ++p) {

      sutype::patterninstances_tc & pis = _patternIdToInstances[p];

      // simplify
      for (sutype::svi_t i = 0; i < (sutype::svi_t)pis.size(); ++i) {

        const suPatternInstance * pi = pis[i];
                
        // get pattern bbox; to print error only
        sutype::dcoord_t bboxxl = 0;
        sutype::dcoord_t bboxyl = 0;
        sutype::dcoord_t bboxxh = 0;
        sutype::dcoord_t bboxyh = 0;
        bool needinit = true;
        sutype::clause_t & satindices = suClauseBank::loan_clause ();
        pi->layoutFunc()->collect_satindices (satindices);
        suStatic::sort_and_leave_unique (satindices);
        for (const auto & iter : satindices) {
          sutype::satindex_t satindex = iter;
          suWire * wire = suSatRouter::instance()->get_wire_by_satindex (satindex);
          if (!wire) continue;
          const suRectangle & rect = wire->rect();
          if (needinit || rect.xl() < bboxxl) bboxxl = rect.xl();
          if (needinit || rect.yl() < bboxyl) bboxyl = rect.yl();
          if (needinit || rect.xh() > bboxxh) bboxxh = rect.xh();
          if (needinit || rect.yh() > bboxyh) bboxyh = rect.yh();
          needinit = false;
        }
        suClauseBank::return_clause (satindices);
        //
        
        pi->layoutFunc()->simplify();
        
        // delete constant-0 as useless
        if (pi->layoutFunc()->is_constant (0)) {
          delete pi;
          pis[i] = 0;
          pis[i] = pis.back();
          pis.pop_back();
          --i;
          ++num0;
          continue;
        }

        // delete constant-1 for a while
        if (pi->layoutFunc()->is_constant (1)) {
          
          SUISSUE("Found unsolvable DRV (written an error)") << ": pattern: " << pi->to_str() << std::endl;

          suErrorManager::instance()->add_error (pi->pattern()->shortname(),
                                                 bboxxl,
                                                 bboxyl,
                                                 bboxxh,
                                                 bboxyh);
          
          delete pi;
          pis[i] = 0;
          pis[i] = pis.back();
          pis.pop_back();
          --i;
          ++num1;
          continue;
        }
      }

      std::sort (pis.begin(), pis.end(), suStatic::compare_pattern_instances_by_id);
    }

    if (num0 != 0) {
      SUINFO(1) << "Deleted " << num0 << " redundant and useless constant-0 pattern instances." << std::endl;
    }
    if (num1 != 0) {
      SUINFO(1) << "Deleted " << num1 << " redundant and useless constant-1 pattern instances." << std::endl;
    }

    sutype::uvi_t num = 0;   
    
    // same pattern
    for (sutype::uvi_t p = 0; p < _patternIdToInstances.size(); ++p) {

      sutype::patterninstances_tc & pis = _patternIdToInstances[p];
      
      // compare
      for (sutype::svi_t i=0; i < (sutype::svi_t)pis.size(); ++i) {
        
        const suPatternInstance * pi1 = pis[i];
        
        for (sutype::svi_t k=i+1; k < (sutype::svi_t)pis.size(); ++k) {

          const suPatternInstance * pi2 = pis[k];

          if (!pi1->layoutFunc()->is_identical (pi2->layoutFunc())) continue;

//           if (1 || (pi2->id() == 9519 || pi2->id() == 8040)) {
//             SUINFO(1) << "Found pi1: " << pi1->to_str() << std::endl;
//             pi1->print_pattern_instance (std::cout);
//             SUINFO(1) << "Found pi2: " << pi2->to_str() << std::endl;
//             pi2->print_pattern_instance (std::cout);
//             SUINFO(1) << "Deleted pi=" << pi2->id() << " because of pi=" << pi1->id() << "." << std::endl;
//           }

          delete pi2;
          pis[k] = 0;
          pis[k] = pis.back();
          pis.pop_back();
          --k;
          ++num;
        }
      }
    }

    SUINFO(1) << "Deleted " << num << " redundant pattern instances of same patterns." << std::endl;
    num = 0;

    // between different patterns
    // slow but has no effect in my test cases
    if (0) {
      
      for (sutype::uvi_t p1 = 0; p1 < _patternIdToInstances.size(); ++p1) {

        const sutype::patterninstances_tc & pis1 = _patternIdToInstances[p1];
        if (pis1.empty()) continue;
      
        for (sutype::uvi_t p2 = p1+1; p2 < _patternIdToInstances.size(); ++p2) {
        
          sutype::patterninstances_tc & pis2 = _patternIdToInstances[p2];
          if (pis2.empty()) continue;
        
          // compare
          for (sutype::uvi_t i=0; i < pis1.size(); ++i) {
          
            const suPatternInstance * pi1 = pis1[i];
          
            for (sutype::svi_t k=0; k < (sutype::svi_t)pis2.size(); ++k) {

              const suPatternInstance * pi2 = pis2[k];

              if (!pi1->layoutFunc()->is_identical (pi2->layoutFunc())) continue;

              //             if (1 || (pi2->id() == 9519 || pi2->id() == 8040)) {
              //               SUINFO(1) << "Found pi1: " << pi1->to_str() << std::endl;
              //               pi1->print_pattern_instance (std::cout);
              //               SUINFO(1) << "Found pi2: " << pi2->to_str() << std::endl;
              //               pi2->print_pattern_instance (std::cout);
              //               SUINFO(1) << "Deleted pi=" << pi2->id() << " because of pi=" << pi1->id() << "." << std::endl;
              //             }

              delete pis2[k];
              pis2[k] = 0;
              pis2[k] = pis2.back();
              pis2.pop_back();
              --k;
              ++num;
            }
          }
        }
      }

      SUINFO(1) << "Deleted " << num << " redundant pattern instances of different patterns." << std::endl;
    }

    // final sort
    for (sutype::uvi_t p = 0; p < _patternIdToInstances.size(); ++p) {

      sutype::patterninstances_tc & pis = _patternIdToInstances[p];
      std::sort (pis.begin(), pis.end(), suStatic::compare_pattern_instances_by_id);
    }
    
  } // end of suPatternManager::delete_redundant_pattern_instances

  //
  void suPatternManager::read_pattern_file (const std::string & filename)
  {
    suTokenParser tokenparser;

    bool ok = tokenparser.read_file (filename);
    SUASSERT (ok, "");

    const std::string emptystr ("");

    sutype::tokens_t tokens = tokenparser.rootToken()->get_tokens ("Pattern");

    for (sutype::uvi_t i=0; i < tokens.size(); ++i) {
      
      const suToken * token = tokens[i];

      const std::string & name      = token->is_defined ("name")      ? token->get_string_value ("name")      : emptystr;
      const std::string & comment   = token->is_defined ("comment")   ? token->get_string_value ("comment")   : emptystr;
      const std::string & shortname = token->is_defined ("shortname") ? token->get_string_value ("shortname") : emptystr;

      suPattern * pattern = new suPattern ();
      SUASSERT (pattern->id() == (sutype::id_t)_patterns.size(), "");

      _patterns.push_back (pattern);
      _patternIdToInstances.push_back (sutype::patterninstances_tc());

      pattern->name (name);
      pattern->comment (comment);

      if (!shortname.empty()) {
        pattern->shortname (shortname);
      }
      else {
        pattern->shortname (name);
      }
      
      sutype::tokens_t tokens2 = token->get_tokens ("Node");
      if (tokens2.size() != 1) {
        SUASSERT (false, "Unexpected format of a pattern " << name << "; file " << filename << "; tokens2.size()=" << tokens2.size());
      }

      parse_token_as_pattern_node_ (pattern, tokens2.front(), 0);

      //pattern->dump_as_flat_lgf_file ("out");
      
      pattern->collect_unique_layers (_patternsPerLayer);
    }
    
  } // end of suPatternManager::read_pattern_file
  
  //
  sutype::satindex_t suPatternManager::create_wire_index_for_via_cut (const suGrid * grid,
                                                                      const suGenerator * generator,
                                                                      sutype::gcoord_t gx,
                                                                      sutype::gcoord_t gy)
  {
    sutype::dcoord_t dx = grid->get_dcoord_by_gcoord (sutype::go_ver, gx);
    sutype::dcoord_t dy = grid->get_dcoord_by_gcoord (sutype::go_hor, gy);
    
    return (create_wire_index_for_via_cut (generator, dx, dy));
    
  } // end of suPatternManager::create_wire_index_for_via_cut
  
  //
  sutype::satindex_t suPatternManager::create_wire_index_for_via_cut (const suGenerator * generator,
                                                                      sutype::dcoord_t dx,
                                                                      sutype::dcoord_t dy)
  {    
    const suLayer * layer = generator->get_cut_layer ();
    
    suRectangle rect = generator->get_shape (dx, dy);

    return create_wire_index_for_dcoords (layer, rect.xl(), rect.yl(), rect.xh(), rect.yh());
    
  } // end of suPatternManager::create_wire_index_for_via_cut

  //
  sutype::satindex_t suPatternManager::create_wire_index_for_transformation (const suWire * wire,
                                                                             const sutype::tr_t & tr)
  {
    const suLayer * layer = wire->layer();

    suRectangle rect;
    rect.copy (wire->rect());
    rect.apply_transformation (tr);

    return create_wire_index_for_dcoords (layer, rect.xl(), rect.yl(), rect.xh(), rect.yh());
    
  } // end of suPatternManager::create_wire_index_for_transformation

  //
  sutype::satindex_t suPatternManager::create_wire_index_for_dcoords (const suLayer * layer,
                                                                      sutype::dcoord_t xl,
                                                                      sutype::dcoord_t yl,
                                                                      sutype::dcoord_t xh,
                                                                      sutype::dcoord_t yh)
  {
    suWire * wire = suWireManager::instance()->create_wire_from_dcoords ((const suNet *)0, layer, xl, yl, xh, yh, sutype::wt_pattern);
    
    sutype::satindex_t wireindex = add_a_wire_ (wire);
    
    return wireindex;
    
  } // end of suPatternManager::create_wire_index_for_dcoords
  
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
  sutype::satindex_t suPatternManager::add_a_wire_ (suWire * wire)
  {
    SUASSERT (wire->satindex() == sutype::UNDEFINED_SAT_INDEX, "");

    for (sutype::uvi_t i=0; i < _wires.size(); ++i) {
      if (_wires[i] == wire) {
        SUASSERT (suWireManager::instance()->get_wire_usage (wire) >= 2, "");
        suWireManager::instance()->release_wire (wire);
        return i;
      }
    }
    
    sutype::satindex_t wireindex = _wires.size();
    SUASSERT (wireindex > 0, "");

    _wires.push_back (wire);

    return wireindex;
    
  } // end of suPatternManager::add_a_wire_

  // \return func 
  void suPatternManager::parse_token_as_pattern_node_ (suPattern * pattern,
                                                       const suToken * token,
                                                       suLayoutFunc * parentFunc)
  {
    SUASSERT (token, "");
    const std::string & type = token->get_string_value ("type");

    if (type.compare ("leaf") == 0) {

      SUASSERT (parentFunc, "Parent can't be null" << "; pattern: " << pattern->name() << "; " << pattern->comment());
      
      const std::string & presence = token->get_string_value ("pr");
      
      int s = 1;
      if        (presence.compare ("mbp") == 0) { s =  1;
      } else if (presence.compare ("mba") == 0) { s = -1;
      } else {
        SUASSERT (false, "Unexpected presence: " << presence << "; pattern: " << pattern->name() << "; " << pattern->comment());
      }

      const std::string & layername = token->get_string_value ("layer");
      const suLayer * layer = suLayerManager::instance()->get_colored_layer_by_name (layername);
      SUASSERT (layer, "Can't get layer: " << layername << "; pattern: " << pattern->name() << "; " << pattern->comment());
      
      std::vector<int> rectcoords = token->get_list_of_integers ("rect", ':');
      SUASSERT (rectcoords.size() >= 4, "Bad rectangle: " << token->get_string_value("rect") << "; pattern: " << pattern->name() << "; " << pattern->comment());

      unsigned i = 0;
      sutype::dcoord_t xl = rectcoords[i]; ++i;
      sutype::dcoord_t yl = rectcoords[i]; ++i;
      sutype::dcoord_t xh = rectcoords[i]; ++i;
      sutype::dcoord_t yh = rectcoords[i]; ++i;

      sutype::satindex_t wireindex = create_wire_index_for_dcoords (layer, xl, yl, xh, yh);
           
      parentFunc->add_leaf (s * wireindex);

      return;
    }

    sutype::logic_func_t func = sutype::logic_func_and;
    sutype::unary_sign_t sign = sutype::unary_sign_just;
    
    if        (type.compare ("AND")  == 0) { func = sutype::logic_func_and; sign = sutype::unary_sign_just;
    } else if (type.compare ("NAND") == 0) { func = sutype::logic_func_and; sign = sutype::unary_sign_not;
    } else if (type.compare ("OR")   == 0) { func = sutype::logic_func_or;  sign = sutype::unary_sign_just;
    } else if (type.compare ("NOR")  == 0) { func = sutype::logic_func_or;  sign = sutype::unary_sign_not;
    } else {
      SUASSERT (false, "Unexpected node type: " << type << "; pattern: " << pattern->name() << "; " << pattern->comment());
    }

    suLayoutFunc * layoutfunc = suLayoutNode::create_func (func, sign);

    // set as a root function node
    if (parentFunc == 0) {
      SUASSERT (pattern->layoutFunc() == 0, "");
      pattern->layoutFunc (layoutfunc);
    }
    else {
      parentFunc->add_node (layoutfunc);
    }
    
    sutype::tokens_t tokens2 = token->get_tokens ("Node");
    SUASSERT (!tokens2.empty(), "Found a dangling node" << "; pattern: " << pattern->name() << "; " << pattern->comment());
    
    for (const auto & iter : tokens2) {
      parse_token_as_pattern_node_ (pattern, iter, layoutfunc);
    }
    
  } // end of suPatternManager::parse_token_as_pattern_node_

} // end of namespace amsr

// end of suPatternManager.cpp
