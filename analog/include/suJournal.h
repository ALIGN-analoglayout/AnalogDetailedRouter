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
//! \date   Wed Sep 27 13:56:18 2017

//! \file   suJournal.h
//! \brief  A header of the class suJournal.

#ifndef _suJournal_h_
#define _suJournal_h_

// system includes
#include <assert.h>

// std includes
#include <string>
#include <iostream>

//
#include <suTypedefs.h>

namespace amsr {

  extern std::string __report_event__ (sutype::log_event_t le,
                                       const std::string & filename,
                                       int lineindex,
                                       const std::string & msgid);

} // end of namespace amsr

#ifdef _RELEASE_BUILD_
#define _ENABLE_LOG_ 0
#else
#define _ENABLE_LOG_ 1
#endif // _RELEASE_BUILD_

#define SUISSUE(msgid) if (1) std::cout << amsr::__report_event__ (amsr::sutype::le_issue, __FILE__, __LINE__, msgid)

#define SUFATAL(msgid) if (1) std::cout << amsr::__report_event__ (amsr::sutype::le_fatal, __FILE__, __LINE__, msgid)

#define SUERROR(msgid) if (1) std::cout << amsr::__report_event__ (amsr::sutype::le_error, __FILE__, __LINE__, msgid)

#define SULABEL(msgid) if (1) std::cout << amsr::__report_event__ (amsr::sutype::le_label, __FILE__, __LINE__, msgid) << std::endl;

// RUINFO/RUOUT is used for most important messages which are printed even all other other logs are disabled.
// It helps to localize the problem quickly.
// In the code, value for RUINFO is uses as a Boolean switch rather than as an integer level.
// Usually it's 1 - "print this message" or 0 - "don't print this message.

#define SUINFO(value) if (int(value) > 1 || (_ENABLE_LOG_ && (value))) std::cout << amsr::__report_event__ (amsr::sutype::le_info, __FILE__, __LINE__, "")

#define SUOUT(value)  if (int(value) > 1 || (_ENABLE_LOG_ && (value))) std::cout

#endif // _suJournal_h_

// end of suJournal.h

