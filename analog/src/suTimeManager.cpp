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
//! \date   Wed Oct  4 11:44:07 2017

//! \file   suTimeManager.cpp
//! \brief  A collection of methods of the class suTimeManager.

// system includes
#include <time.h>

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

// module include
#include <suTimeManager.h>

namespace amsr
{

  //
  suTimeManager * suTimeManager::_instance = 0;
  timespec suTimeManager::_timeSpec;
  
  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  //
  suTimeManager::suTimeManager ()
  {
    init_ ();
    
    clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &suTimeManager::_timeSpec);
    
    _startCpuTime = ((double)suTimeManager::_timeSpec.tv_sec + (double)suTimeManager::_timeSpec.tv_nsec / (double)1000000000.0);
    
  } // end of suTimeManager::suTimeManager

  // ------------------------------------------------------------
  // -
  // --- Public static methods
  // -
  // ------------------------------------------------------------

  //
  void suTimeManager::create_instance ()
  {
    SUASSERT (suTimeManager::_instance == 0, "");
    
    suTimeManager::_instance = new suTimeManager;
    
  } // end of suTimeManager::create_instance

  //
  void suTimeManager::delete_instance ()
  {
    if (suTimeManager::_instance)
      delete suTimeManager::_instance;
    
    suTimeManager::_instance = 0;
    
  } // end of suTimeManager::delete_instance

  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  double suTimeManager::get_cpu_time ()
    const
  {    
    clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &suTimeManager::_timeSpec);
    
    // high resolution CPU time
    return (((double)suTimeManager::_timeSpec.tv_sec + (double)suTimeManager::_timeSpec.tv_nsec / (double)1000000000.0) - _startCpuTime);
    
  } // end of suTimeManager::get_cpu_time
  
  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------


} // end of namespace amsr

// end of suTimeManager.cpp
