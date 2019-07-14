// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Mon Oct 16 13:06:32 2017

//! \file   suLayoutFunc.cpp
//! \brief  A collection of methods of the class suLayoutFunc.

// std includes
#include <algorithm>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suClauseBank.h>
#include <suLayoutLeaf.h>
#include <suLayoutNode.h>
#include <suSatRouter.h>
#include <suSatSolverWrapper.h>
#include <suStatic.h>
#include <suWire.h>

// module include
#include <suLayoutFunc.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  
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


  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  //
  sutype::bool_t suLayoutFunc::get_modeled_value ()
    const
  {
    sutype::uvi_t num0 = 0;
    sutype::uvi_t num1 = 0;

    for (sutype::uvi_t i=0; i < _nodes.size(); ++i) {

      suLayoutNode * node = _nodes[i];

      sutype::bool_t value = node->is_func() ? node->to_func()->get_modeled_value() : suSatSolverWrapper::instance()->get_modeled_value (node->to_leaf()->satindex());
      
      if      (value == sutype::bool_false) ++num0;
      else if (value == sutype::bool_true)  ++num1;
      else {
        SUASSERT (false, "");
      }
      
      if        (func() == sutype::logic_func_or  && value == sutype::bool_true)  { return ((sign() == sutype::unary_sign_just) ? value : suStatic::invert_bool_value (value));
      } else if (func() == sutype::logic_func_and && value == sutype::bool_false) { return ((sign() == sutype::unary_sign_just) ? value : suStatic::invert_bool_value (value));
      }
    }

    SUASSERT (num0 || num1, "");

    if        (func() == sutype::logic_func_or  && num1 == 0) { return ((sign() == sutype::unary_sign_just) ? sutype::bool_false : sutype::bool_true);
    } else if (func() == sutype::logic_func_and && num0 == 0) { return ((sign() == sutype::unary_sign_just) ? sutype::bool_true  : sutype::bool_false);
    } else {
      SUASSERT (false, "");
    }

    return sutype::bool_undefined;
    
  } // end of suLayoutFunc::get_modeled_value

  //
  suLayoutLeaf * suLayoutFunc::add_leaf (sutype::satindex_t satindex,
                                         const sutype::clause_t & depth)
  {
    if (depth.empty()) {
      return add_leaf (satindex);
    }
    
    return add_leaf_ (satindex, depth, 0);
    
  } // end of suLayoutFunc::add_leaf

  //
  void suLayoutFunc::delete_nodes ()
  {
    for (sutype::uvi_t i=0; i < _nodes.size(); ++i) {

      suLayoutNode * node = _nodes[i];
      
      if (node->is_func()) {
        node->to_func()->delete_nodes ();
        suLayoutNode::delete_func (node->to_func());
      }
    }
    
    _nodes.clear();
    
  } // end of suLayoutFunc::delete_nodes

  //
  suLayoutFunc * suLayoutFunc::create_clone_and_replace_leaves_by_empty_or_functions ()
    const
  {
    suLayoutFunc * clonedfunc = suLayoutNode::create_func (func(), sign());

    for (sutype::uvi_t i=0; i < _nodes.size(); ++i) {

      suLayoutNode * node = _nodes[i];
      
      if (node->is_func()) {
        
        clonedfunc->add_node (node->to_func()->create_clone_and_replace_leaves_by_empty_or_functions());
      }
     
      // replace a leaf by an empty OR
      else if (node->is_leaf()) {
        
        suLayoutLeaf * leaf = node->to_leaf();
        SUASSERT (leaf->satindex() != sutype::UNDEFINED_SAT_INDEX, "");
        sutype::unary_sign_t s = (leaf->satindex() > 0) ? sutype::unary_sign_just : sutype::unary_sign_not;
        
        suLayoutFunc * orfunc = suLayoutNode::create_func (sutype::logic_func_or, s);
        
        // this constant0 will make this node constant0 if there're no wires that can be put into this func
        orfunc->add_leaf (suSatSolverWrapper::instance()->get_constant (0));
        
        clonedfunc->add_node (orfunc);
      }
      
      else {
        SUASSERT (false, "");
      }
    }
    
    return clonedfunc;
    
  } // end of suLayoutFunc::create_clone_and_replace_leaves_by_empty_or_functions

  // virtual
  suLayoutNode * suLayoutFunc::create_clone ()
    const
  {
    suLayoutFunc * clonedfunc = suLayoutNode::create_func (func(), sign());
    
    clonedfunc->satindex (satindex());
    
    for (sutype::uvi_t i=0; i < _nodes.size(); ++i) {
      
      suLayoutNode * node = _nodes[i];
      clonedfunc->_nodes.push_back (node->create_clone());
    }
    
    return clonedfunc;
    
  } // end of suLayoutFunc::create_clone

  // virtual
  sutype::satindex_t suLayoutFunc::model_satindex (sutype::satindex_t satindexToCheck,
                                                   sutype::satindex_t modeledValue)
    const
  {
    const bool np = false;

    sutype::clause_t & clause = suClauseBank::loan_clause ();

    SUASSERT (!_nodes.empty(), "");

    for (sutype::uvi_t i=0; i < _nodes.size(); ++i) {

      suLayoutNode * node = _nodes[i];

      sutype::satindex_t satindex = node->model_satindex (satindexToCheck, modeledValue);
      
      SUINFO(np)
        << "satindex=" << satindex
        << "; satindexToCheck=" << satindexToCheck
        << "; modeledValue=" << modeledValue
        << "; node_satindex=" << node->satindex()
        << "; isleaf=" << node->is_leaf()
        << std::endl;
      
      if (satindex == sutype::UNDEFINED_SAT_INDEX) {
        clause.push_back (satindex);
        continue;
      }
      
      if ((func() == sutype::logic_func_and && suSatSolverWrapper::instance()->is_constant (1, satindex)) ||
          (func() == sutype::logic_func_or  && suSatSolverWrapper::instance()->is_constant (0, satindex))) {
        
        continue;
      }

      if ((func() == sutype::logic_func_and && suSatSolverWrapper::instance()->is_constant (0, satindex)) ||
          (func() == sutype::logic_func_or  && suSatSolverWrapper::instance()->is_constant (1, satindex))) {
        
        suClauseBank::return_clause (clause);
        SUINFO(np) << "return satindex=" << satindex << std::endl;
        return satindex;
      }

      clause.push_back (satindex);
    }
    
    if (clause.empty()) {

      sutype::satindex_t satindex = sutype::UNDEFINED_SAT_INDEX;

      if      (func() == sutype::logic_func_and) satindex = suSatSolverWrapper::instance()->get_constant (1);
      else if (func() == sutype::logic_func_or)  satindex = suSatSolverWrapper::instance()->get_constant (0);
      else {
        SUASSERT (false, "");
      }

      suClauseBank::return_clause (clause);
      SUINFO(np) << "return satindex=" << satindex << std::endl;
      return satindex;
    }

    SUASSERT (!clause.empty(), "");
    
    if (clause.size() == 1) {
      
      sutype::satindex_t satindex = clause.front();
      suClauseBank::return_clause (clause);
      SUINFO(np) << "return satindex=" << satindex << std::endl;
      return satindex;
    }

    for (sutype::uvi_t i=0; i < clause.size(); ++i) {

      sutype::satindex_t satindex1 = clause[i];
      if (satindex1 == sutype::UNDEFINED_SAT_INDEX) continue;

      for (sutype::uvi_t k=i+1; k < clause.size(); ++k) {

        sutype::satindex_t satindex2 = clause[k];
        if (satindex2 == sutype::UNDEFINED_SAT_INDEX) continue;

        // this unique case makes AND constant0, and it makes OR constant1
        if (satindex1 == -satindex2) {
          
          sutype::satindex_t satindex = sutype::UNDEFINED_SAT_INDEX;
          
          if      (func() == sutype::logic_func_and) satindex = suSatSolverWrapper::instance()->get_constant (0);
          else if (func() == sutype::logic_func_or)  satindex = suSatSolverWrapper::instance()->get_constant (1);
          else {
            SUASSERT (false, "");
          }
          
          suClauseBank::return_clause (clause);
          SUINFO(np) << "return satindex=" << satindex << std::endl;
          return satindex;
        }
      }
    }
    
    suClauseBank::return_clause (clause);
    
    sutype::satindex_t satindex = sutype::UNDEFINED_SAT_INDEX;
    SUINFO(np) << "return satindex=" << satindex << std::endl;
    return satindex;
    
  } // end of suLayoutFunc::model_satindex
  
  // virtual
  sutype::satindex_t suLayoutFunc::calculate_satindex (bool rootcall)
  {
    if (_satindex != sutype::UNDEFINED_SAT_INDEX) return _satindex;

    if (rootcall) {
      make_canonical ();
    }

    SUASSERT (!_nodes.empty(), to_str());
    
    SUASSERT (sign() == sutype::unary_sign_just, "");

    // After simplify, I expect only AND and OR
    // NAND and NOR are not expected
    
    sutype::clause_t & clause = suClauseBank::loan_clause ();
    
    // mode 1: leaves
    // mode 2: functions
    for (int mode = 1; mode <= 2; ++mode) {
    
      for (sutype::uvi_t i=0; i < _nodes.size(); ++i) {

        suLayoutNode * node = _nodes[i];
        
        if (mode == 1 && !node->is_leaf()) continue;
        if (mode == 2 && !node->is_func()) continue;

        sutype::satindex_t satindex = node->calculate_satindex (false);

        if ((func() == sutype::logic_func_and && suSatSolverWrapper::instance()->is_constant (1, satindex)) ||
            (func() == sutype::logic_func_or  && suSatSolverWrapper::instance()->is_constant (0, satindex))) {
        
          continue;
        }

        if ((func() == sutype::logic_func_and && suSatSolverWrapper::instance()->is_constant (0, satindex)) ||
            (func() == sutype::logic_func_or  && suSatSolverWrapper::instance()->is_constant (1, satindex))) {
        
          _satindex = satindex;
          suClauseBank::return_clause (clause);
          return _satindex;
        }
      
        clause.push_back (satindex);
      }
    }
    
    if (clause.empty()) {

      if      (func() == sutype::logic_func_and) _satindex = suSatSolverWrapper::instance()->get_constant (1);
      else if (func() == sutype::logic_func_or)  _satindex = suSatSolverWrapper::instance()->get_constant (0);
      else {
        SUASSERT (false, "");
      }

      suClauseBank::return_clause (clause);
      return _satindex;
    }
    
    SUASSERT (!clause.empty(), "");
    
    if      (func() == sutype::logic_func_and) _satindex = suSatSolverWrapper::instance()->emit_AND_or_return_constant (clause);
    else if (func() == sutype::logic_func_or)  _satindex = suSatSolverWrapper::instance()->emit_OR_or_return_constant  (clause);
    else {
      SUASSERT (false, "");
    }

    suClauseBank::return_clause (clause);
    
    return _satindex;
    
  } // end of suLayoutFunc::calculate_satindex

  //
  bool suLayoutFunc::is_valid ()
    const
  {
    if (_nodes.empty()) return false;

    for (sutype::uvi_t i=0; i < _nodes.size(); ++i) {

      suLayoutNode * node = _nodes[i];
      
      if (node->is_leaf()) {
        if (node->to_leaf()->satindex() == sutype::UNDEFINED_SAT_INDEX) return false;
      }

      if (node->is_func()) {
        bool ok = node->to_func()->is_valid();
        if (!ok) return false;
      }
    }

    return true;
    
  } // end of suLayoutFunc::is_valid

  //
  bool suLayoutFunc::has_dangling_funcs ()
    const
  {
    if (_nodes.empty()) return true;
    
    for (const auto & iter : _nodes) {
      
      suLayoutNode * node = iter;
      
      if (node->is_leaf()) continue;
      
      if (node->to_func()->has_dangling_funcs())
        return true;
    }

    return false;
    
  } // end of suLayoutFunc::has_dangling_funcs

  //
  bool suLayoutFunc::is_identical (const suLayoutFunc * func)
    const
  {
    bool v12 = suStatic::compare_layout_nodes (this->to_node(), func->to_node());
    bool v21 = suStatic::compare_layout_nodes (func->to_node(), this->to_node());
    
    return (v12 == v21);
    
  } // end of suLayoutFunc::is_identical
  
  // make sign sutype::unary_sign_just everywhere
  void suLayoutFunc::make_canonical ()
  {
    if (sign() != sutype::unary_sign_just) {
      
      invert_func ();
      invert_sign ();
      
      for (sutype::uvi_t i = 0; i < _nodes.size(); ++i) {
        
        suLayoutNode * node = _nodes[i];
        
        if (node->is_func()) {
          node->to_func()->invert_sign ();
          node->to_func()->invert_satindex ();
        }
        else {
          _nodes[i] = suLayoutNode::create_leaf (-node->to_leaf()->satindex());
        }
      }
    }
    
    for (sutype::uvi_t i = 0; i < _nodes.size(); ++i) {

      suLayoutNode * node = _nodes[i];
      
      if (node->is_func()) {
        node->to_func()->make_canonical ();
      }
    }

    std::sort (_nodes.begin(), _nodes.end(), suStatic::compare_layout_nodes);
    
  } // end of suLayoutFunc::make_canonical

  //
  bool suLayoutFunc::is_constant (int index)
    const
  {
    SUASSERT (!_nodes.empty(), "");

     for (sutype::uvi_t i = 0; i < _nodes.size(); ++i) {

       suLayoutNode * layoutnode = _nodes[i];

       if      (func() == sutype::logic_func_and && sign() == sutype::unary_sign_just && index == 0 &&  layoutnode->is_constant (index)) return true;
       else if (func() == sutype::logic_func_and && sign() == sutype::unary_sign_just && index == 1 && !layoutnode->is_constant (index)) return false;
       else if (func() == sutype::logic_func_or  && sign() == sutype::unary_sign_just && index == 0 && !layoutnode->is_constant (index)) return false;
       else if (func() == sutype::logic_func_or  && sign() == sutype::unary_sign_just && index == 1 &&  layoutnode->is_constant (index)) return true;
       
       else if (func() == sutype::logic_func_and && sign() == sutype::unary_sign_not  && index == 0 &&  !layoutnode->is_constant (1 - index)) return false;
       else if (func() == sutype::logic_func_and && sign() == sutype::unary_sign_not  && index == 1 &&   layoutnode->is_constant (1 - index)) return true;
       else if (func() == sutype::logic_func_or  && sign() == sutype::unary_sign_not  && index == 0 &&   layoutnode->is_constant (1 - index)) return true;
       else if (func() == sutype::logic_func_or  && sign() == sutype::unary_sign_not  && index == 1 &&  !layoutnode->is_constant (1 - index)) return false;

       else {
         // do nothing
       }
     }

     if      (func() == sutype::logic_func_and && sign() == sutype::unary_sign_just && index == 1) return true;
     else if (func() == sutype::logic_func_or  && sign() == sutype::unary_sign_just && index == 0) return true;
     else if (func() == sutype::logic_func_and && sign() == sutype::unary_sign_not  && index == 0) return true;
     else if (func() == sutype::logic_func_or  && sign() == sutype::unary_sign_not  && index == 1) return true;
     else {
       //SUINFO(1) <<  "index=" << index << std::endl;
       //suSatRouter::print_layout_node (std::cout, this, "");
       //SUASSERT (false, "");
       return false;
     }
    
  } // end of suLayoutFunc::is_constant

  //
  void suLayoutFunc::simplify (bool rootcall)
  {
    SUASSERT (!_nodes.empty(), "");

    const bool checkEmittedConstants = false;

    // make_canonical is heavy and it makes canonical a full subtree
    if (rootcall) {
      make_canonical ();
      //return; // tofix
    }

    SUASSERT (sign() == sutype::unary_sign_just, "");

    // simplify the tree
    for (sutype::uvi_t i = 0; i < _nodes.size(); ++i) {
      
      suLayoutNode * layoutnode = _nodes[i];
      if (layoutnode->is_leaf()) continue;

      suLayoutFunc * layoutfunc = layoutnode->to_func();
      layoutfunc->simplify (false);

      SUASSERT (layoutfunc->sign() == sutype::unary_sign_just, "");
      
      const sutype::layoutnodes_t & nodes = layoutfunc->nodes();
      SUASSERT (!nodes.empty(), "");
      
      // replace func by its single node
      if (nodes.size() == 1) {
        _nodes[i] = layoutfunc->nodes().front();
        layoutfunc->clear_nodes();
        suLayoutNode::delete_func (layoutfunc);        
        continue;
      }

      // replace func by its first node
      // put other nodes to the end
      if (layoutfunc->func() == func()) {
        
        _nodes[i] = layoutfunc->nodes().front();
        
        for (sutype::uvi_t k=1; k < nodes.size(); ++k) {
          _nodes.push_back (nodes[k]);
        }

        layoutfunc->clear_nodes();
        suLayoutNode::delete_func (layoutfunc);
        continue;
      }
    }

    sutype::svi_t survidedIndex = -1;

    for (sutype::svi_t i = 0; i < (int)_nodes.size(); ++i) {

      suLayoutNode * layoutnode = _nodes[i];
      
      // check leaf
      if (layoutnode->is_leaf()) {
        
        sutype::satindex_t satindex = layoutnode->to_leaf()->satindex();
        
        bool isconstant0 = false;
        bool isconstant1 = false;
        
        if      (suSatSolverWrapper::instance()->is_constant (0, satindex, checkEmittedConstants)) isconstant0 = true;
        else if (suSatSolverWrapper::instance()->is_constant (1, satindex, checkEmittedConstants)) isconstant1 = true;

        if ((func() == sutype::logic_func_and && isconstant0) ||
            (func() == sutype::logic_func_or  && isconstant1)) {

          survidedIndex = i;
          break;
        }

        if ((func() == sutype::logic_func_and && isconstant1) ||
            (func() == sutype::logic_func_or  && isconstant0)) {

          _nodes[i] = _nodes.back();
          _nodes.pop_back();
          --i;
          continue;
        }
      }
    }
    
    if (_nodes.empty()) {

      SUASSERT (survidedIndex < 0, "");
      
      if        (func() == sutype::logic_func_and) { add_leaf (suSatSolverWrapper::instance()->get_constant (1));
      } else if (func() == sutype::logic_func_or)  { add_leaf (suSatSolverWrapper::instance()->get_constant (0));
      }
      
      SUASSERT (_nodes.size() == 1, "");
    }

    if (survidedIndex >= 0) {

      suLayoutNode * layoutnode = _nodes [survidedIndex];
      SUASSERT (layoutnode->is_leaf(), "");

      for (sutype::svi_t i=0; i < (sutype::svi_t)_nodes.size(); ++i) {

        if (i == survidedIndex) continue;

        suLayoutNode * node = _nodes[i];
      
        if (node->is_func()) {
          node->to_func()->delete_nodes ();
          suLayoutNode::delete_func (node->to_func());
        }
      }
    
      _nodes.clear();

      if        (suSatSolverWrapper::instance()->is_constant (0, layoutnode->to_leaf()->satindex(), checkEmittedConstants)) { add_leaf (suSatSolverWrapper::instance()->get_constant (0));
      } else if (suSatSolverWrapper::instance()->is_constant (1, layoutnode->to_leaf()->satindex(), checkEmittedConstants)) { add_leaf (suSatSolverWrapper::instance()->get_constant (1));
      } else {
        _nodes.push_back (layoutnode);
      }
    }

    SUASSERT (sign() == sutype::unary_sign_just, "");

    // move nodes one level up
    if (_nodes.size() == 1 && _nodes.front()->is_func()) {

      suLayoutFunc * singlefunc = _nodes.front()->to_func();
      SUASSERT (singlefunc->sign() == sutype::unary_sign_just, "");
      SUASSERT (singlefunc->sign() == sign(), "");
      
      // copy nodes & func; sign must be the same
      _nodes.clear();
      _nodes.insert (_nodes.end(), singlefunc->nodes().begin(), singlefunc->nodes().end());
      //_nodes = singlefunc->nodes();
      func (singlefunc->func());
      
      singlefunc->_nodes.clear ();
      suLayoutNode::delete_func (singlefunc);
    }

    SUASSERT (sign() == sutype::unary_sign_just, "");
    
    // AND(a) == OR(a), when a is the only node
    // let's make it AND(a) if it was OR(a)
    if (_nodes.size() == 1 && func() != sutype::logic_func_and) {
      invert_func ();
    }

    SUASSERT (sign() == sutype::unary_sign_just, "");

    // update sat index
    if (_nodes.size() == 1) {
      
      SUASSERT (_nodes.front()->is_leaf(), "");
      SUASSERT (sign() == sutype::unary_sign_just, "");
      SUASSERT (func() == sutype::logic_func_and, "");
      
      _satindex = _nodes.front()->to_leaf()->satindex();
    }
    
    if (rootcall) {
      make_canonical ();
      //std::sort (_nodes.begin(), _nodes.end(), suStatic::compare_layout_nodes);
    }
    
  } // end of suLayoutFunc::simplify

  //
  void suLayoutFunc::collect_satindices (sutype::clause_t & satindices)
    const
  {
    for (sutype::uvi_t i=0; i < _nodes.size(); ++i) {

      suLayoutNode * node = _nodes[i];

      if (node->is_leaf()) {

        sutype::satindex_t leafsatindex = abs (node->to_leaf()->satindex());
        satindices.push_back (leafsatindex);
      }

      else if (node->is_func()) {

        node->to_func()->collect_satindices (satindices);
      }

      else {
        SUASSERT (false, "");
      }
    }
    
  } // end of suLayoutFunc::collect_satindices
  
  //
  bool suLayoutFunc::has_satindex (sutype::satindex_t satindex)
    const
  {
    for (sutype::uvi_t i=0; i < _nodes.size(); ++i) {
      
      suLayoutNode * node = _nodes[i];

      if (node->is_leaf()) {

        sutype::satindex_t leafsatindex = node->to_leaf()->satindex();
        if (leafsatindex == satindex || leafsatindex == -satindex) return true;
      }

      else if (node->is_func()) {

        bool ret = node->to_func()->has_satindex (satindex);
        if (ret) return ret;
      }

      else {
        SUASSERT (false, "");
      }
    }

    return false;
    
  } // suLayoutFunc::has_satindex

  //
  bool suLayoutFunc::replace_satindex (sutype::satindex_t satindexOldValue,
                                       sutype::satindex_t satindexNewValue)
  {
    bool modified = false;

    if (satindexOldValue == satindexNewValue) return modified;
    
    for (sutype::uvi_t i=0; i < _nodes.size(); ++i) {
      
      suLayoutNode * node = _nodes[i];

      if (node->is_leaf()) {

        sutype::satindex_t leafsatindex = node->to_leaf()->satindex();

        if (leafsatindex == satindexOldValue) {
          _nodes[i] = suLayoutNode::create_leaf (satindexNewValue);
          modified = true;
          continue;
        }
       
        if (leafsatindex == -satindexOldValue) {
          _nodes[i] = suLayoutNode::create_leaf (-satindexNewValue);
          modified = true;
          continue;
        }
      }

      else if (node->is_func()) {

        bool ret = node->to_func()->replace_satindex (satindexOldValue, satindexNewValue);
        if (ret) modified = true;
      }
      
      else {
        SUASSERT (false, "");
      }
    }

    return modified;
    
  } // end of suLayoutFunc::replace_satindex

  //
  std::string suLayoutFunc::to_str ()
    const
  {
    std::ostringstream oss;

    if (!fileName().empty()) {
      oss << "origin: " << fileName() << ":" << fileLine();
    }
    
    return oss.str();

  } // end of suLayoutFunc::to_str
  
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
  suLayoutLeaf * suLayoutFunc::add_leaf_ (sutype::satindex_t satindex,
                                          const sutype::clause_t & depth,
                                          sutype::uvi_t depthindex)
  {
    SUASSERT (depthindex >= 0 && depthindex < depth.size(), "");
    
    sutype::satindex_t position = depth [depthindex];
    
    SUASSERT (position >= 0, "");
    SUASSERT (position < (sutype::satindex_t)_nodes.size(), "");
    SUASSERT (_nodes[position]->is_func(), "");
    
    if (depthindex + 1 == depth.size()) {
      
      return _nodes[position]->to_func()->add_leaf (satindex);
    }
    
    else {

      return _nodes[position]->to_func()->add_leaf_ (satindex, depth, depthindex+1);
    }
    
  } // end of suLayoutFunc::add_leaf_


} // end of namespace amsr

// end of suLayoutFunc.cpp
