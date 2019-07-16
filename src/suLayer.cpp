// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Fri Oct  6 14:29:57 2017

//! \file   suLayer.cpp
//! \brief  A collection of methods of the class suLayer.

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
#include <suColoredLayer.h>

// module include
#include <suLayer.h>

namespace amsr
{
  //
  sutype::id_t suLayer::_uniqueId = 0;

  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  //! destructor
  suLayer::~suLayer ()
  {
    for (const auto & iter : _coloredLayers) {
      delete iter;
    }
    
  } // end of suLayer::~suLayer


  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  //
  bool suLayer::has_name (const std::string & str)
    const
  {
    if (name().compare (str) == 0) return true;

    for (const auto & iter : _names) {
      
      const std::string & regexp = iter;
      if (regexp.compare (str) == 0) return true;
    }

    return false;
    
  } // end of suLayer::has_name

  //
  void suLayer::add_name (const std::string & str)
  {
    if (std::find (_names.begin(), _names.end(), str) != _names.end()) return;

    _names.push_back (str);

    std::sort (_names.begin(), _names.end());
    
  } // end of suLayer::add_name

  //
  const suLayer * suLayer::get_colored_layer_by_type (const std::string & colortype)
    const
  {
    SUASSERT (is_base(), "");

    if (_coloredLayers.empty() && colortype.empty()) return this;
    
    for (const auto & iter : _coloredLayers) {
      
      const suColoredLayer * coloredlayer = iter;
      if (coloredlayer->color_type().compare (colortype) == 0) return coloredlayer;
    }
    
    //SUISSUE ("Could not get colored layer by type") << ": \"" << colortype << "\" for layer " << name() << std::endl;
    
    return 0;
    
  } // suLayer::get_colored_layer_by_type

  //
  const suLayer * suLayer::get_colored_layer_by_name (const std::string & colorname)
    const
  {
    SUASSERT (is_base(), "");
    
    for (const auto & iter : _coloredLayers) {
      
      const suColoredLayer * coloredlayer = iter;
      if (coloredlayer->name().compare (colorname) == 0) return coloredlayer;
    }

    return 0;
    
  } // suLayer::get_colored_layer_by_name
  
  //
  void suLayer::add_colored_layer (const suColoredLayer * coloredlayer1)
  {
    SUASSERT (coloredlayer1, "");
    SUASSERT (coloredlayer1->base() == this, "");

    for (const auto & iter : _coloredLayers) {

      const suColoredLayer * coloredlayer2 = iter;
      SUASSERT (coloredlayer2->name().compare (coloredlayer1->name()) != 0, "Colored layer name is defined twice " << coloredlayer1->name());
      SUASSERT (coloredlayer2->color_type().compare (coloredlayer1->color_type()) != 0, "Colored layer type is defined twice " << coloredlayer1->name());
    }
    
    _coloredLayers.push_back (coloredlayer1);
    
  } // end of suLayer::add_colored_layer

  //
  std::string suLayer::to_str ()
    const
  {
    std::ostringstream oss;

    oss << "{";

    oss << name();

    if (is_base()) {
      oss << "; base";
    }
    else {
      oss << "; color";
    }

    oss << "; pers_id=" << pers_id();
    oss << "; base_id=" << base_id();

    oss << "; level=" << level();
    
    oss << "}";

    return oss.str();
    
  } // end of suLayer::to_str

  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------


} // end of namespace amsr

// end of suLayer.cpp
