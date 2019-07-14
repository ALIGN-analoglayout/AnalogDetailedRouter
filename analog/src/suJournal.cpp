// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Thu Jul 19 10:02:12 2018

//! \file   suJournal.cpp
//! \brief  A collection of methods of the class suJournal.

// std includes
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// application includes
#include <suTypedefs.h>

// other application includes
#include <suStatic.h>

namespace amsr {

  //
  std::string __report_event__ (sutype::log_event_t le,
                                const std::string & filename,
                                int lineindex,
                                const std::string & msgid)
  {
    std::ostringstream oss;
  
    oss << "-" << suStatic::log_event_2_str (le) << "- ";

#ifndef _RELEASE_BUILD_

    oss << "(" << filename << ":" << lineindex << "): ";
    
#endif // _RELEASE_BUILD_

    if (!msgid.empty()) {
      oss << msgid;
    }
    
    if (le != sutype::le_info) {
      suStatic::register_a_log_message (le, oss.str());
    }
  
    return oss.str();
  
  } // end of __report_event__

} // end of namespace amsr


// end of suJournal.cpp
