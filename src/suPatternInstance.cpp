// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Nov 14 16:28:01 2017

//! \file   suPatternInstance.cpp
//! \brief  A collection of methods of the class suPatternInstance.

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
#include <suLayoutFunc.h>
#include <suPattern.h>
#include <suSatRouter.h>
#include <suStatic.h>
#include <suWire.h>

// module include
#include <suPatternInstance.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  //
  sutype::id_t suPatternInstance::_uniqueId = 0;


  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  //! custom constructor
  suPatternInstance::suPatternInstance (const suPattern * pattern,
                                        const sutype::tr_t & tr)
  {
    init_ ();
    
    _pattern = pattern;
    
    _tr = tr;    

    SUASSERT (_pattern, "");
    
    _layoutFunc = _pattern->layoutFunc()->create_clone_and_replace_leaves_by_empty_or_functions ();
    
  } // end of suPatternInstance

  //
  suPatternInstance::~suPatternInstance ()
  {
    if (_layoutFunc) {
      _layoutFunc->delete_nodes ();
      suLayoutNode::delete_func (_layoutFunc);
    }
    
  } // end of suPatternInstance::~suPatternInstance

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
  std::string suPatternInstance::to_str ()
    const
  {
    std::ostringstream oss;

    oss
      << "{"
      << "pattern=" << _pattern->name()
      << "; pid=" << _pattern->id()
      << "; tr=" << suStatic::tr_2_str (_tr)
      << "; id=" << id()
      << "}";
      
    return oss.str();
    
  } // end of suPatternInstance::to_str

  //
  void suPatternInstance::print_pattern_instance (std::ostream & oss)
    const
  {
    oss
      << "Pattern name=" << _pattern->name()
      << " comment=\"" << _pattern->comment() << "\""
      << " tr=" << suStatic::tr_2_str (_tr)
      << " {"
      << std::endl;

    suSatRouter::print_layout_node (oss, _layoutFunc, "  ");

    oss << "}";

    oss << std::endl;
    
  } // end of suPatternInstance::print_pattern_instance

  //
  suPatternInstance * suPatternInstance::create_clone ()
    const
  {
    suPatternInstance * pi = new suPatternInstance (_pattern, _tr);
    
    return pi;
    
  } // end of suPatternInstance::create_clone
  
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

// end of suPatternInstance.cpp
