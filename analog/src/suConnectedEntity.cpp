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
//! \date   Mon Oct 16 11:20:03 2017

//! \file   suConnectedEntity.cpp
//! \brief  A collection of methods of the class suConnectedEntity.

// std includes
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suGlobalRoute.h>
#include <suLayer.h>
#include <suLayoutFunc.h>
#include <suLayoutLeaf.h>
#include <suLayoutNode.h>
#include <suNet.h>
#include <suSatSolverWrapper.h>
#include <suStatic.h>
#include <suWire.h>

// module include
#include <suConnectedEntity.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------
  
  sutype::id_t suConnectedEntity::_uniqueId = 0;
  
  
  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  suConnectedEntity::~suConnectedEntity ()
  {
    // _openSatindex can be either suSatSolverWrapper::instance()->get_constant(0) or suSatSolverWrapper::instance()->get_next_sat_index()
    // anyway, I don't create too much connected entities to worry about available satindices
    // moreover, I don't want to check here what kind of _openSatindex I used here
    
    //if (_openSatindex != sutype::UNDEFINED_SAT_INDEX) {
    //  suSatSolverWrapper::instance()->return_sat_index (_openSatindex);
    //}
    
  } // end of suConnectedEntity::~suConnectedEntity


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
  std::string suConnectedEntity::to_str ()
    const
  {
    std::ostringstream oss;

    oss
      << "{"
      << "net=" << _net->name()
      << "; ceid=" << _id;

    if (_type != sutype::wt_undefined) {
      oss
        << "; type=" << suStatic::wire_type_2_str (_type);
    }

    if (get_interface_layer()) {
      oss
        << "; interface=" << get_interface_layer()->name();
    }
    else {
      oss
        << "; interface=<null>";
    }
    
    if (_trunkGlobalRoute) {
      oss
        << "; trunk=" << _trunkGlobalRoute->to_str();
    }

    oss
      << "; iwires=" << _interfaceWires.size();
    
    oss
      << "}";

    return oss.str();
    
  } // end of suConnectedEntity::to_str

  //
  bool suConnectedEntity::layout_function_is_legal ()
    const
  {
    suLayoutFunc * layoutfuncCE = layoutFunc();

    if (layoutfuncCE == 0) return false;
    if (layoutfuncCE->func() != sutype::logic_func_or) return false;
    if (layoutfuncCE->sign() != sutype::unary_sign_just) return false;
    if (layoutfuncCE->nodes().size() != 2) return false;

    // expected to be 2-node function to support opens
    suLayoutNode * node0 = layoutfuncCE->nodes().front();
    suLayoutNode * node1 = layoutfuncCE->nodes().back();
    if (!((node0->is_leaf() && node1->is_func()) || (node0->is_func() && node1->is_leaf()))) return false;
    
    suLayoutLeaf * leaf        = node0->is_leaf() ? node0->to_leaf() : node1->to_leaf();
    suLayoutFunc * layoutfunc0 = node0->is_func() ? node0->to_func() : node1->to_func();
    
    if (leaf->satindex() != openSatindex()) return false;
    if (layoutfunc0->func() != sutype::logic_func_and) return false;
    if (layoutfunc0->sign() != sutype::unary_sign_just) return false;
    
    return true;
    
  } // end of suConnectedEntity::layout_function_is_legal

  //
  suLayoutFunc * suConnectedEntity::get_main_layout_function ()
    const
  {
    suLayoutFunc * layoutfuncCE = layoutFunc();
    
    SUASSERT (layoutfuncCE, "");
    SUASSERT (layoutfuncCE->func() == sutype::logic_func_or, "");
    SUASSERT (layoutfuncCE->sign() == sutype::unary_sign_just, "");
    SUASSERT (layoutfuncCE->nodes().size() == 2, "");

    suLayoutNode * node0 = layoutfuncCE->nodes().front();
    suLayoutNode * node1 = layoutfuncCE->nodes().back();
    SUASSERT ((node0->is_leaf() && node1->is_func()) || (node0->is_func() && node1->is_leaf()), "");
    suLayoutLeaf * leaf        = node0->is_leaf() ? node0->to_leaf() : node1->to_leaf();
    suLayoutFunc * layoutfunc0 = node0->is_func() ? node0->to_func() : node1->to_func();
    SUASSERT (leaf->satindex() == openSatindex(), "");
    SUASSERT (layoutfunc0->func() == sutype::logic_func_and, "Can be only AND to support incrementally added constraints");
    SUASSERT (layoutfunc0->sign() == sutype::unary_sign_just, "Can be only AND to support incrementally added constraints");
    
    return layoutfunc0;
    
  } // end of suConnectedEntity::get_main_layout_function

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


} // end of namespace amsr

// end of suConnectedEntity.cpp
