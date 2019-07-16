// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Mon Mar 19 10:02:11 2018

//! \file   suLayoutLeaf.cpp
//! \brief  A collection of methods of the class suLayoutLeaf.

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
#include <suSatSolverWrapper.h>

// module include
#include <suLayoutLeaf.h>

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

  // virtual
  bool suLayoutLeaf::is_constant (int index)
    const
  {
    return suSatSolverWrapper::instance()->is_constant (index, _satindex);
    
  } // end of suLayoutLeaf::is_constant
  
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

// end of suLayoutLeaf.cpp
