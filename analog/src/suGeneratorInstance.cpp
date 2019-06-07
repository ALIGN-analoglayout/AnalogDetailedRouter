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
//! \date   Fri Nov  3 14:23:16 2017

//! \file   suGeneratorInstance.cpp
//! \brief  A collection of methods of the class suGeneratorInstance.

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
#include <suGenerator.h>
#include <suLayoutFunc.h>
#include <suNet.h>
#include <suWire.h>

// module include
#include <suGeneratorInstance.h>

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

  //! custom constructor
  suGeneratorInstance::suGeneratorInstance (const suGenerator * g,
                                            const suNet * n,
                                            sutype::dcoord_t dx,
                                            sutype::dcoord_t dy,
                                            sutype::wire_type_t cutwiretype)
  {
    init_ ();
    
    _net = n;
    _generator = g;
    
    _point.x (dx);
    _point.y (dy);

    SUASSERT (_generator, "");
    SUASSERT (_wires.empty(), "");
    
    _generator->clone_wires (_wires, _net, dx, dy, cutwiretype);
    
    SUASSERT (!_wires.empty(), "");
    
    if (_wires.size() != 3) {
      for (const auto & iter : _wires) {
        SUINFO(1) << iter->to_str() << std::endl;
      }
      SUASSERT (false, "other via generators are not supported yet");
    }
    
  } // end of suGeneratorInstance::suGeneratorInstance
  
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
  suWire * suGeneratorInstance::get_wire (sutype::via_generator_layer_t vgl)
    const
  {
    const suLayer * layer = _generator->get_layer (vgl);

    for (sutype::uvi_t i=0; i < _wires.size(); ++i) {

      suWire * wire = _wires[i];
      if (wire->layer() == layer) return wire;
    }

    return 0;
    
  } // end of suGeneratorInstance::get_wire

  //
  suGeneratorInstance * suGeneratorInstance::create_clone (sutype::wire_type_t cutwiretype)
    const
  {
    return (new suGeneratorInstance (_generator, _net, _point.x(), _point.y(), cutwiretype));
    
  } // end of suGeneratorInstance::create_clone
      
  //
  std::string suGeneratorInstance::to_str ()
    const
  {
    std::ostringstream oss;
    
    oss << "{";

    oss
      << _point.x()
      << ":"
      << _point.y();

    if (_generator)
      oss << "; " << _generator->name();
    
    if (_net)
      oss << "; net=" << _net->name();

    if (_layoutFunc && _layoutFunc->satindex() != sutype::UNDEFINED_SAT_INDEX) {
      oss << "; satindex=" << _layoutFunc->satindex();
    }
    
    if (!_legal) {
      oss << "; illegal";
    }
    
    oss << "}";

    return oss.str();
    
  } // end of suGeneratorInstance::to_str
  
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

// end of suGeneratorInstance.cpp
