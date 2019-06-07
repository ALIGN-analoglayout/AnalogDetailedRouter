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
//! \date   Mon Oct 16 11:25:34 2017

//! \file   suLayoutNode.cpp
//! \brief  A collection of methods of the class suLayoutNode.

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
#include <suLayoutFunc.h>
#include <suLayoutLeaf.h>
#include <suSatSolverWrapper.h>
#include <suStatic.h>

// module include
#include <suLayoutNode.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  // used for debug purposes only
  std::string suLayoutNode::_emptyString;
  
  sutype::layoutfuncs_t suLayoutNode::_layoutFuncs;
  sutype::layoutleaves_t suLayoutNode::_layoutLeavesPosSign;
  sutype::layoutleaves_t suLayoutNode::_layoutLeavesNegSign;
  
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

  // static
  suLayoutFunc * suLayoutNode::create_counter (sutype::boolean_counter_t ct,
                                               sutype::unary_sign_t sign,
                                               sutype::layoutnodes_t & nodes,
                                               sutype::svi_t number)
  {
    //SUINFO(1) << "suLayoutNode::create_counter: nodes=" << nodes.size() << "; number=" << number << std::endl;
    
    SUASSERT (!nodes.empty(), "");

    if      (ct == sutype::bc_greater) return suLayoutNode::create_counter (sutype::bc_greater_or_equal, sign, nodes, number+1);
    else if (ct == sutype::bc_less)    return suLayoutNode::create_counter (sutype::bc_less_or_equal,    sign, nodes, number-1);

    SUASSERT (ct == sutype::bc_greater_or_equal || ct == sutype::bc_less_or_equal, "");

    // #nodes is greater_or_equal than number
    if (ct == sutype::bc_greater_or_equal) {

      if (number <= 0) {
        suLayoutFunc * func = suLayoutNode::create_func (sutype::logic_func_and, sign);
        func->add_leaf (suSatSolverWrapper::instance()->get_constant(1));
        return func;
      }

      else if (number == 1) {
        suLayoutFunc * func = suLayoutNode::create_func (sutype::logic_func_or, sign);
        func->add_nodes (nodes);
        return func;
      }
      
      else if (number == (sutype::svi_t)nodes.size()) {
        suLayoutFunc * func = suLayoutNode::create_func (sutype::logic_func_and, sign);
        func->add_nodes (nodes);
        return func;
      }

      else if (number > (sutype::svi_t)nodes.size()) {
        suLayoutFunc * func = suLayoutNode::create_func (sutype::logic_func_and, sign);
        func->add_leaf (suSatSolverWrapper::instance()->get_constant(0));
        return func;
      }

      suLayoutFunc * func = suLayoutNode::build_default_gte_counter_ (nodes, number);
      func->sign (sign);
      return func;
    }
    
    // #nodes is less_or_equal than number
    else {

      sutype::unary_sign_t opsign = suStatic::invert_unary_sign (sign);

      if (number >= (sutype::svi_t)nodes.size()) {
        suLayoutFunc * func = suLayoutNode::create_func (sutype::logic_func_and, sign);
        func->add_leaf (suSatSolverWrapper::instance()->get_constant(1));
        return func;
      }

      else if (number+1 == (sutype::svi_t)nodes.size()) {
        suLayoutFunc * func = suLayoutNode::create_func (sutype::logic_func_and, opsign); // nand(nodes)
        func->add_nodes (nodes);
        return func;
      }

      else if (number == 0) {
        suLayoutFunc * func = suLayoutNode::create_func (sutype::logic_func_or, opsign); // nor(nodes)
        func->add_nodes (nodes);
        return func;
      }

      else if (number < 0) {
        suLayoutFunc * func = suLayoutNode::create_func (sutype::logic_func_and, sign);
        func->add_leaf (suSatSolverWrapper::instance()->get_constant(0));
        return func;
      }

      // invert nodes
      //
      for (sutype::uvi_t i=0; i < nodes.size(); ++i) {
                
        if (nodes[i]->is_leaf()) {
          nodes[i] = suLayoutNode::create_leaf (- nodes[i]->to_leaf()->satindex());
        }
        else {
          nodes[i]->to_func()->sign (suStatic::invert_unary_sign (nodes[i]->to_func()->sign()));
        }
      }

      // invert target number {num of Trues} <= N --> {num of Falses} >= nodes.size() - N
      //
      number = (int)nodes.size() - number;
      SUASSERT (number >= 2 && number <= (int)nodes.size() - 1, "number=" << number << "; #nodes=" << nodes.size());
      
      suLayoutFunc * func = suLayoutNode::build_default_gte_counter_ (nodes, number);
      func->sign (sign);
      return func;
    }

    SUASSERT (false, "");
    return 0;
    
  } // end of suLayoutNode::create_counter
  
  // static
  suLayoutFunc * suLayoutNode::create_func (sutype::logic_func_t lf,
                                            sutype::unary_sign_t us,
                                            const std::string & filename,
                                            const int fileline)
  {
    if (suLayoutNode::_layoutFuncs.empty()) {

      suLayoutNode::_layoutFuncs.resize (1024, 0);

      for (unsigned i=0; i < 1024; ++i) {
        suLayoutNode::_layoutFuncs[i] = new suLayoutFunc ();
      }
    }

    suLayoutFunc * func = suLayoutNode::_layoutFuncs.back();
    suLayoutNode::_layoutFuncs.pop_back ();

    SUASSERT (func->nodes().empty(), "");
    
    func->func     (lf);
    func->sign     (us);
    func->satindex (0);
    func->fileName (filename);
    func->fileLine (fileline);

    return func;
    
  } // end of suLayoutNode::create_func
  
  // static
  void suLayoutNode::delete_func (suLayoutFunc * func)
  {
    SUASSERT (func->nodes().empty(), "");
    
    suLayoutNode::_layoutFuncs.push_back (func);
    
  } // end of suLayoutNode::delete_func

  // static
  suLayoutLeaf * suLayoutNode::create_leaf (sutype::satindex_t v)
  {
    SUASSERT (v != sutype::UNDEFINED_SAT_INDEX, "");
    
    sutype::satindex_t absv = (v < 0) ? -v : v;

    if (absv >= (sutype::satindex_t)suLayoutNode::_layoutLeavesPosSign.size()) {
      suLayoutNode::_layoutLeavesPosSign.resize (absv+1, (suLayoutLeaf*)0);
      suLayoutNode::_layoutLeavesNegSign.resize (absv+1, (suLayoutLeaf*)0);
    }

    if (v > 0) {
      if (suLayoutNode::_layoutLeavesPosSign[absv] == 0) {
        suLayoutNode::_layoutLeavesPosSign[absv] = new suLayoutLeaf (v);
      }
      return suLayoutNode::_layoutLeavesPosSign[absv];  
    }
    else {
      if (suLayoutNode::_layoutLeavesNegSign[absv] == 0) {
        suLayoutNode::_layoutLeavesNegSign[absv] = new suLayoutLeaf (v);
      }
      return suLayoutNode::_layoutLeavesNegSign[absv];
    }
    
  } // end of suLayoutNode::create_leaf

  // static
  void suLayoutNode::delete_leaves ()
  {
    for (sutype::uvi_t i=0; i < suLayoutNode::_layoutLeavesPosSign.size(); ++i) {
      if (suLayoutNode::_layoutLeavesPosSign[i])
        delete suLayoutNode::_layoutLeavesPosSign[i];
    }

    for (sutype::uvi_t i=0; i < suLayoutNode::_layoutLeavesNegSign.size(); ++i) {
      if (suLayoutNode::_layoutLeavesNegSign[i])
        delete suLayoutNode::_layoutLeavesNegSign[i];
    }

    suLayoutNode::_layoutLeavesPosSign.clear();
    suLayoutNode::_layoutLeavesNegSign.clear();
    
  } // end of suLayoutNode::delete_leaves
    
  // static
  void suLayoutNode::delete_funcs () 
  {
    for (sutype::uvi_t i=0; i < suLayoutNode::_layoutFuncs.size(); ++i) {
      delete suLayoutNode::_layoutFuncs[i];
    }

    suLayoutNode::_layoutFuncs.clear();
    
  } // end of suLayoutNode::delete_funcs
  
  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------


  // ------------------------------------------------------------
  // -
  // --- Private static methods
  // -
  // ------------------------------------------------------------

  // static
  suLayoutFunc * suLayoutNode::build_default_gte_counter_ (const sutype::layoutnodes_t & nodes,
                                                           const sutype::svi_t number)
  {
    SUASSERT (nodes.size() >= 3, "");
    SUASSERT (number > 1 && number < (int)nodes.size(), "");
    
    // get max stage & level
    const size_t numlevels = number;
    const size_t numstages = nodes.size();
    
    SUASSERT (numlevels < numstages, "");
    
    suLayoutFunc * rootfunc = suLayoutNode::create_func (sutype::logic_func_or, sutype::unary_sign_just);
    
    int stage = (int)numstages - 1;
    int level = (int)numlevels - 1;
    
    SUASSERT (stage >= 1, "");
    SUASSERT (level >= 0, "");

    sutype::uvi_t numfuncs = 0;
        
    suLayoutNode::build_unary_counter_ (rootfunc,
                                        stage,
                                        level,
                                        nodes,
                                        numfuncs);
    
    SUASSERT (rootfunc->nodes().size() == 2, "");

    SUINFO(0)
      << "build_default_gte_counter_"
      << ": nodes=" << nodes.size()
      << "; number=" << number
      << "; numfuncs=" << numfuncs
      << std::endl;
    
    //SUABORT;
    
    return rootfunc;
    
  } // end of suLayoutNode::build_default_gte_counter_

  // static
  // Almost every AND/OR drives two other nodes
  // here, I cannot reuse created functions; because I have to build a tree
  // suLayoutFunc is a tree; it's not an electrical circuit of ANDs and ORs as I show in the paper
  // So that, it creates O(numstages*numlevels) new functions; it can be used only for small counters
  // When I emit counters to SAT directly; I don't have such problems; because the numbers of clauses is O(N)
  void suLayoutNode::build_unary_counter_ (suLayoutFunc * lnfunc,
                                           int stage,
                                           int level,
                                           const std::vector <suLayoutNode *> & nodes,
                                           sutype::uvi_t & numfuncs)
    
  {
    SUASSERT (level >= 0 && stage >= 1, "");
    SUASSERT (stage < (int)nodes.size(), "");
    SUASSERT (lnfunc->nodes().size() == 0, "");

//     SUINFO(1)
//       << "  stage=" << stage
//       << ";\t level=" << level
//       << ";\t sldiff=" << (stage-level)
//       << ";\t func=" << ((lnfunc->func() == sutype::logic_func_or) ? "OR" : "AND")
//       << std::endl;
    
    // OR
    if (lnfunc->func() == sutype::logic_func_or) {
      
      // fisrt input
      if (level == 0) {
        suLayoutNode * node = nodes[stage];
        SUASSERT (node, "");
        lnfunc->add_node (node->create_clone());
      }
      else {
        int s = stage;
        int l = level;

        suLayoutFunc * funcAnd = suLayoutNode::create_func (sutype::logic_func_and, sutype::unary_sign_just);
        ++numfuncs;

        lnfunc->add_node (funcAnd);
        build_unary_counter_ (funcAnd, s, l, nodes, numfuncs);
      }

      SUASSERT (lnfunc->nodes().size() == 1, "");
      
      // second input
      if (level == 0 && stage == 1) {
        suLayoutNode * node = nodes[stage-1];
        SUASSERT (node, "");
        lnfunc->add_node (node->create_clone());
      }
      else if (stage == level+1) {
        int s = stage-1;
        int l = level;

        suLayoutFunc * funcAnd = suLayoutNode::create_func (sutype::logic_func_and, sutype::unary_sign_just);
        ++numfuncs;
        
        lnfunc->add_node (funcAnd);
        build_unary_counter_ (funcAnd, s, l, nodes, numfuncs);
      }
      else {
        int s = stage-1;
        int l = level;
 
        suLayoutFunc * funcOr = suLayoutNode::create_func (sutype::logic_func_or, sutype::unary_sign_just);
        ++numfuncs;
        
        lnfunc->add_node (funcOr);
        build_unary_counter_ (funcOr, s, l, nodes, numfuncs);
      }

      SUASSERT (lnfunc->nodes().size() == 2, "");
    }

    // AND
    else if (lnfunc->func() == sutype::logic_func_and) {

      // first input
      if (1) {
        suLayoutNode * node = nodes[stage];
        SUASSERT (node, "");
        lnfunc->add_node (node->create_clone());
      }

      SUASSERT (lnfunc->nodes().size() == 1, "");

      // second input
      if (stage == 1) {
        suLayoutNode * node = nodes[stage-1];
        SUASSERT (node, "");
        lnfunc->add_node (node->create_clone());
      }
      else if (stage == level) {
        int s = stage-1;
        int l = level-1;

        suLayoutFunc * funcAnd = suLayoutNode::create_func (sutype::logic_func_and, sutype::unary_sign_just);
        ++numfuncs;
        
        lnfunc->add_node (funcAnd);
        build_unary_counter_ (funcAnd, s, l, nodes, numfuncs);
      }
      else {
        int s = stage-1;
        int l = level-1;

        suLayoutFunc * funcOr = suLayoutNode::create_func (sutype::logic_func_or, sutype::unary_sign_just);
        ++numfuncs;
        
        lnfunc->add_node (funcOr);
        build_unary_counter_ (funcOr, s, l, nodes, numfuncs);
      }
      
      SUASSERT (lnfunc->nodes().size() == 2, "");
    }
    
    else {
      SUASSERT (false, "");
    }
    
  } // end of suLayoutNode::build_unary_counter_
  
  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------


} // end of namespace amsr

// end of suLayoutNode.cpp
