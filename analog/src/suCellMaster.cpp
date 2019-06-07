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
//! \date   Mon Oct  9 15:16:11 2017

//! \file   suCellMaster.cpp
//! \brief  A collection of methods of the class suCellMaster.

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
#include <suCellManager.h>
#include <suNet.h>

// module include
#include <suCellMaster.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  suCellMaster::~suCellMaster ()
  {
    for (sutype::uvi_t i=0; i < _nets.size(); ++i) {
      delete _nets[i];
    }
    
  } // suCellMaster::~suCellMaster

  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  //
  suNet * suCellMaster::get_net_by_name (const std::string & netname)
    const
  {
    for (sutype::uvi_t i=0; i < _nets.size(); ++i) {

      suNet * net = _nets[i];
      if (net->name().compare (netname) == 0) return net;
    }
    
    return 0;
    
  } // end of suCellMaster::get_net_by_name

  //
  suNet * suCellMaster::create_net (const std::string & netname)
  {
    suNet * net = new suNet (netname);
    
    _nets.push_back (net);

    suCellManager::instance()->register_a_net (net);
    
    return net;
    
  } // end of suCellMaster::create_net

  //
  suNet * suCellMaster::create_net_if_needed (const std::string & netname)
  {
    suNet * net = get_net_by_name (netname);

    if (net) return net;

    net = create_net (netname);

    SUASSERT (net, "");

    return net;
    
  } // end of suCellMaster::create_net_if_needed
  

  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------


} // end of namespace amsr

// end of suCellMaster.cpp
