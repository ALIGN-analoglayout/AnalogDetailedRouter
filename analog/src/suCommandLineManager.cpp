// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Thu Oct  5 16:04:48 2017

//! \file   suCommandLineManager.cpp
//! \brief  A collection of methods of the class suCommandLineManager.

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
#include <suCommandLineManager.h>

namespace amsr
{
  //
  suCommandLineManager * suCommandLineManager::_instance = 0;

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
  
  //
  void suCommandLineManager::delete_instance ()
  {
    if (suCommandLineManager::_instance)
      delete suCommandLineManager::_instance;
    
    suCommandLineManager::_instance = 0;
    
  } // end of suCommandLineManager::delete_instance

  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  void suCommandLineManager::parse_options (const sutype::strings_t & strs)
  {
    for (sutype::uvi_t i=0; i < strs.size(); ++i) {
      
      const std::string & str = strs[i];
      
      if (str.compare ("-file") == 0) { ++i; _runFile = get_string_ (strs, i); }
    }
    
  } // end of suCommandLineManager::parse_options

  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------

  //
  const std::string & suCommandLineManager::get_string_ (const sutype::strings_t & strs,
                                                         sutype::uvi_t index)
    const
  {
    SUASSERT (index > 0, "");
    SUASSERT (index < strs.size(), "No value for command line option " << strs[index-1]);
    
    return strs[index];
    
  } // suCommandLineManager::get_string_
  

} // end of namespace amsr

// end of suCommandLineManager.cpp
