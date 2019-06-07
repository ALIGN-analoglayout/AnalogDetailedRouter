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

//! \file   suWire.cpp
//! \brief  A collection of methods of the class suWire.

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
#include <suLayer.h>
#include <suNet.h>
#include <suRectangle.h>
#include <suStatic.h>
#include <suWireManager.h>

// module include
#include <suWire.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  //
  sutype::id_t suWire::_uniqueId = 0;
  
  
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
  sutype::wire_type_t suWire::get_wire_type ()
    const
  {
    sutype::wire_type_t wiretype = sutype::wt_undefined;
    
    for (int i=1; i < (int)sutype::wt_num_types; i *= 2) {

      sutype::wire_type_t singlewiretype = (sutype::wire_type_t)i;
      sutype::bitid_t bitid = (sutype::bitid_t)singlewiretype;

      if (!this->is (bitid)) continue;

      if (wiretype == sutype::wt_undefined) {
        wiretype = singlewiretype;
        continue;
      }
      
      SUASSERT (false, "wire has several types: " << to_str());
    }
    
    return wiretype;
    
  } // end of suWire::get_wire_type

  //
  sutype::dcoord_t suWire::sidel () const { SUASSERT (_layer->has_pgd(), to_str()); return _rect.sidel (_layer->pgd()); }
  sutype::dcoord_t suWire::sideh () const { SUASSERT (_layer->has_pgd(), to_str()); return _rect.sideh (_layer->pgd()); }
  sutype::dcoord_t suWire::edgel () const { SUASSERT (_layer->has_pgd(), to_str()); return _rect.edgel (_layer->pgd()); }
  sutype::dcoord_t suWire::edgeh () const { SUASSERT (_layer->has_pgd(), to_str()); return _rect.edgeh (_layer->pgd()); }
  
  //
  std::string suWire::to_str ()
    const
  {
    std::ostringstream oss;
    
    oss << "{";

    oss << _rect.to_str (":");

    if (_layer && _layer->has_pgd()) {

      SUASSERT (edgel() <= edgeh(), "");
      SUASSERT (sidel() <= sideh(), "");
      SUASSERT (width() >= 0, "");
      SUASSERT (length() >= 0, "");
      
      oss
        << "; w=" << width();
      oss
        << "; l=" << length();
      oss
        << "; sidec=" << sidec();
    }
    
    if (net())
      oss << "; net=" << net()->name() << ";" << net()->id();
    else
      oss << "; net=<null>";
    
    if (_layer) {
      oss << "; layer=" << _layer->name() << ";" << _layer->level();
      if (_layer->is_base())
        oss << " (base)";
      else
        oss << " (color)";
    }
    else {
      oss << "; layer=<null>";
    }
    
    oss << "; satindex=" << satindex();

    oss << "; gid=" << gid();

    oss << "; id=" << _id;

    oss << "; usage=" << suWireManager::instance()->get_wire_usage (this);

    if (suWireManager::instance()->wire_is_illegal (this)) {
      oss << "; illegal";
    }

    if (suWireManager::instance()->wire_is_obsolete (this)) {
      oss << "; obsolete";
    }
    
    if (_type != 0)
      oss << "; type=" << suStatic::wire_type_2_str (_type);
    
    oss << "}";

    SUASSERT (_rect.xl() <= _rect.xh(), oss.str());
    SUASSERT (_rect.yl() <= _rect.yh(), oss.str());
    
    return oss.str();
    
  } // end of suWire::to_str 

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

// end of suWire.cpp
