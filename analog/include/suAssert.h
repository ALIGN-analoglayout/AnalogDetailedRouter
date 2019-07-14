// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Wed Sep 27 13:49:14 2017

//! \file   suAssert.h
//! \brief  Custom asserts

#ifndef _suAssert_h_
#define _suAssert_h_

#include <stdlib.h>
#include <assert.h>

// std includes
#include <string>
#include <iostream>

// application includes
#include <suJournal.h>

extern void __do_something_before_assert__ ();

extern void __final_assert_action__ ();

#define SUABORT if(1) { \
    SUFATAL("") << "*" << std::endl;                                    \
    SUFATAL("") << "*" << std::endl;                                    \
    SUFATAL("") << " No panic. Program intentionally aborted by Nikolai." << std::endl; \
    SUFATAL("") << "*" << std::endl;                                    \
    SUFATAL("") << "*" << std::endl;                                    \
    exit(1);                                                            \
  }

#ifdef _RELEASE_BUILD_

#define SUASSERT(value,message) if (!(value)) { \
                                  SUFATAL("PROGRAM ASSERTED") << ": " << message << std::endl; \
                                  __do_something_before_assert__ (); \
                                  SUFATAL("PROGRAM ASSERTED") << ": " << message << std::endl; \
                                  exit(1); \
                                }
#else // _RELEASE_BUILD_

#define SUASSERT(value,message) if (!(value)) { \
                                  SUFATAL("PROGRAM ASSERTED") << ": " << message << std::endl; \
                                  __do_something_before_assert__ (); \
                                  SUFATAL("PROGRAM ASSERTED") << ": " << message << std::endl; \
                                  assert (false); \
                                }

#endif // _RELEASE_BUILD_

#endif // _suAssert_h_

// end of suAssert.h
