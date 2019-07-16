// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Oct 17 12:14:00 2017

//! \file   suRoute.cpp
//! \brief  A collection of methods of the class suRoute.

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
#include <suLayoutFunc.h>
#include <suLayoutNode.h>
#include <suSatSolverWrapper.h>
#include <suStatic.h>
#include <suWire.h>
#include <suWireManager.h>

// module include
#include <suRoute.h>

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

  //! default constructor
  suRoute::suRoute ()
  {
    init_ ();

    _layoutFunc = suLayoutNode::create_func (sutype::logic_func_and, sutype::unary_sign_just);
    
  } // end of suRoute

  //! destructor
  suRoute::~suRoute ()
  {
    for (const auto & iter : _wires) {
      suWireManager::instance()->release_wire (iter);
    }

    _wires.clear();
    
    if (_layoutFunc) {
      _layoutFunc->delete_nodes ();
      suLayoutNode::delete_func (_layoutFunc);
    }
    
  } // end of suRoute::~suRoute
  
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
  bool suRoute::add_wire (suWire * wire)
  {
    SUASSERT (wire, "");
    
    for (const auto & iter1 : _wires) {

      suWire * wire1 = iter1;
      if (wire1 != wire) continue;

//       for (const auto & iter2 : _wires) {
//         suWire * wire2 = iter2;
//         SUISSUE("Saved wire") << ": " << wire2->to_str() << std::endl;
//       }
//       SUASSERT (false, "saved twice: " << wire->to_str());

      return false;
    }
    
    _wires.push_back (wire);

    return true;
    
  } // end of suRoute::add_wire

  // a wire is useless is the layoutfunc's doesn't use this wire
  bool suRoute::release_useless_wires ()
  {
    const bool np = false;
    
    sutype::clause_t & satindices = suClauseBank::loan_clause ();
    _layoutFunc->collect_satindices (satindices);
    suStatic::sort_and_leave_unique (satindices);
    
    sutype::uvi_t counter = 2;
    SUASSERT (_wires.size() >= counter, "First " << counter << " wires are fixed: the wires are terminal wires this route connects");
    bool modified = false;
    
    for (sutype::uvi_t i = counter; i < _wires.size(); ++i) {
      
      suWire * wire = _wires[i];
      
      SUASSERT (suWireManager::instance()->get_wire_usage (wire) > 0, "");
      
      sutype::satindex_t satindex = wire->satindex();
      SUASSERT (satindex > 0, "");
      
      _wires[counter] = wire;
      ++counter;
      
      // wire is in use; we can't release it
      if (suSatSolverWrapper::instance()->is_constant (1, satindex)) continue;

      //bool ret1 = _layoutFunc->has_satindex (satindex);
      bool ret2 = suStatic::sorted_clause_has_satindex (satindices, satindex);
      //SUASSERT (ret1 == ret2, "");

      // wire is in use
      if (ret2) continue;
      
      if (!suSatSolverWrapper::instance()->is_constant (satindex)) {
        SUINFO(np) << "Found a useless non-constant wire: " << wire->to_str() << std::endl;
      }
      
      suWireManager::instance()->release_wire (wire);
      --counter;
      
      modified = true;
    }

    suClauseBank::return_clause (satindices);
    
    SUASSERT (counter >= 2, "");
    SUASSERT (counter <= _wires.size(), "");
    
    if (counter != _wires.size()) {
      _wires.resize (counter);
    }

    return modified;
    
  } // end of suRoute::release_useless_wires

  //
  bool suRoute::remove_wire_and_update_layout_function (suWire * wire,
                                                        sutype::svi_t index)
  {
    SUASSERT (wire, "");
    SUASSERT (wire->satindex() != sutype::UNDEFINED_SAT_INDEX, wire->to_str());
    SUASSERT (_wires.size() >= 3, "");
    SUASSERT (_wires[0] != wire, "");
    SUASSERT (_wires[1] != wire, "");
    SUASSERT (_layoutFunc, "");
    SUASSERT (wire->satindex() > 0, "");
    SUASSERT (wire->is (sutype::wt_route) || wire->is (sutype::wt_shunt), "");

    if (index >= 0) {

      SUASSERT (index >= 2 && index < (sutype::svi_t)_wires.size(), "");
      SUASSERT (_wires[index] == wire, "");

      _wires[index] = _wires.back();
      _wires.pop_back ();
    }

    else {
    
      sutype::uvi_t counter = 0;

      for (sutype::svi_t i=0; i < (sutype::svi_t)_wires.size(); ++i) {
      
        suWire * wire2 = _wires[i];
        if (wire2 != wire) continue;

        SUASSERT (i != 0, "");
        SUASSERT (i != 1, "");
      
        ++counter;
      
        _wires[i] = _wires.back();
        _wires.pop_back ();
        --i;
      }

      SUASSERT (counter == 1, "");
    }

    bool modified = _layoutFunc->replace_satindex (wire->satindex(), suSatSolverWrapper::instance()->get_constant(0));
    
    suWireManager::instance()->release_wire (wire);

    return modified;
    
  } // end of suRoute::remove_wire_and_update_layout_function

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

// end of suRoute.cpp
