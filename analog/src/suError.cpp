// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Aug 21 14:56:13 2018

//! \file   suError.cpp
//! \brief  A collection of methods of the class suError.

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
#include <suRectangle.h>

// module include
#include <suError.h>

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

  //
  suError::~suError ()
  {
    for (const auto & iter : _rectangles) {
      suRectangle * rectangle = iter;
      delete rectangle;
    }
    
  } // end of suError::~suError


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
  void suError::add_rectangle (sutype::dcoord_t x1,
                               sutype::dcoord_t y1,
                               sutype::dcoord_t x2,
                               sutype::dcoord_t y2)
  {
    _rectangles.push_back (new suRectangle (x1, y1, x2, y2));
    
  } // end of suRectangle::add_rectangle
  

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

// end of suError.cpp
