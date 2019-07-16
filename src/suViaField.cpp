// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Apr 10 13:18:24 2018

//! \file   suViaField.cpp
//! \brief  A collection of methods of the class suViaField.

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

// module include
#include <suViaField.h>

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

  //
  void suViaField::add_via_option (const sutype::viaoption_t & vo)
  {
    sutype::dcoord_t dx0 = std::get<0>(vo);
    sutype::dcoord_t dy0 = std::get<1>(vo);
    const sutype::generatorinstances_t & gis0 = std::get<2>(vo);
    
    for (auto & iter : _viaOptions) {
      
      sutype::viaoption_t & prevvo = iter;
      
      sutype::dcoord_t dx1 = std::get<0>(prevvo);
      sutype::dcoord_t dy1 = std::get<1>(prevvo);

      if (dx1 != dx0 || dy1 != dy0) continue;
      
      sutype::generatorinstances_t & gis1 = std::get<2>(prevvo);

      for (const auto & iter2 : gis0) {
        
        suGeneratorInstance * gi = iter2;
        gis1.push_back (gi);
      }

      return;
    }
    
    // store a new (dx0,dy0) point
    _viaOptions.push_back (vo);
    
  } // end of add_via_option

  //
  std::string suViaField::to_str ()
    const
  {
    std::ostringstream oss;

    oss << "{";

    oss << _layer->name();

    for (const auto & iter : _viaOptions) {

      const sutype::viaoption_t & vo = iter;

      sutype::dcoord_t dx1 = std::get<0>(vo);
      sutype::dcoord_t dy1 = std::get<1>(vo);
      const sutype::generatorinstances_t & gis1 = std::get<2>(vo);

      oss
        << "; "
        << "(" << dx1
        << "," << dy1
        << ",-" << gis1.size() << "-"
        << ")";
    }
    
    oss << "}";

    return oss.str();
    
  } // end of suViaField::to_str

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

// end of suViaField.cpp
