// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Thu Oct 12 12:51:46 2017

//! \file   suMetalTemplate.cpp
//! \brief  A collection of methods of the class suMetalTemplate.

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

// module include
#include <suMetalTemplate.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  sutype::id_t suMetalTemplate::_uniqueId = 0;

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
  sutype::dcoord_t suMetalTemplate::convert_to_canonical_offset (sutype::grid_direction_t gd,
                                                                 sutype::dcoord_t shift)
    const
  {
    sutype::dcoord_t dperiod = _dperiod [gd];
    if (dperiod == 0) return 0;

    sutype::dcoord_t value = shift % dperiod;

    if (value < 0)
      value = dperiod + value;

    SUASSERT (value >= 0, "");
    
    return value;
    
  } // suMetalTemplate::convert_to_canonical_offset


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

// end of suMetalTemplate.cpp
