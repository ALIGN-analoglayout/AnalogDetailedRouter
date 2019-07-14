// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Oct 17 10:29:19 2017

//! \file   suTie.cpp
//! \brief  A collection of methods of the class suTie.

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
#include <suConnectedEntity.h>
#include <suNet.h>
#include <suRoute.h>
#include <suSatSolverWrapper.h>

// module include
#include <suTie.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  sutype::id_t suTie::_uniqueId = 0;


  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  //! custom constructor
  suTie::suTie (const suConnectedEntity * ce1,
                const suConnectedEntity * ce2)
  {
    init_ ();

    SUASSERT (ce1, "");
    SUASSERT (ce2, "");
    SUASSERT (ce1->net(), "");
    SUASSERT (ce1->net() == ce2->net(), "");
    SUASSERT (ce1 != ce2, "");
    
    _net = ce1->net();
    
    _connectedEntities [0] = ce1;
    _connectedEntities [1] = ce2;
    
  } // end of suTie

  //
  suTie::~suTie ()
  {
    delete_routes ();

    // Can't return sat index because it was not taken by get_next_sat_index
    
    //if (_satindex != sutype::UNDEFINED_SAT_INDEX) {
    //  suSatSolverWrapper::instance()->return_sat_index (_satindex);
    //}
    
    // _openSatindex can be either suSatSolverWrapper::instance()->get_constant(0) or suSatSolverWrapper::instance()->get_next_sat_index()
    // anyway, I don't create too much ties to worry about available satindices
    // moreover, I don't want to check here what kind of _openSatindex I used here
    
    //if (_openSatindex != sutype::UNDEFINED_SAT_INDEX) {
    //  suSatSolverWrapper::instance()->return_sat_index (_openSatindex);
    //}
    
  } // end of suTie::~suTie
  
  
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
  void suTie::delete_routes ()
  {
    for (sutype::uvi_t i=0; i < _routes.size(); ++i) {
      suRoute * route = _routes[i];
      delete route;
    }

    _routes.clear();
    
  } // end of suTie::delete_routes

  //
  void suTie::delete_route (suRoute * route)
  {
    sutype::uvi_t counter = 0;

    for (sutype::uvi_t i=0; i < _routes.size(); ++i) {

      suRoute * tieroute = _routes[i];
      
      if (tieroute != route) {
        _routes[counter] = tieroute;
        ++counter;
        continue;
      }

      delete tieroute;
    }

    SUASSERT (counter + 1 == _routes.size(), "");

    if (counter != _routes.size())
      _routes.resize (counter);
    
  } // end of suTie::delete_route

  //
  const suConnectedEntity * suTie::get_another_entity (const suConnectedEntity * entity)
    const
  {
    if        (entity == entity0()) { return entity1();
    } else if (entity == entity1()) { return entity0();
    } else {
      SUASSERT (false, "entity=" << entity->to_str() << "; tie=" << to_str());
    }

    return 0;
    
  } // end of suTie::get_another_entity

  //
  unsigned suTie::calculate_the_number_of_connected_entities_of_a_particular_type (sutype::wire_type_t wt)
    const
  {
    unsigned num = 0;

    if (entity0()->is (wt)) ++num;
    if (entity1()->is (wt)) ++num;

    return num;
    
  } // end of suTie::calculate_the_number_of_connected_entities_of_a_particular_type

  //
  std::string suTie::to_str ()
    const
  {
    std::ostringstream oss;

    oss
      << "{";

    oss
      << "net=" << _net->name()
      << "; tieid=" << _id
      << "; " << entity0()->id() << "-" << entity1()->id();

    oss
      << "; routes=" << _routes.size();
    
    oss
      << "; e0=" << entity0()->to_str()
      << "; e1=" << entity1()->to_str();
    
    oss
      << "}";
    
    return oss.str();
    
  } // end of suTie::to_str


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

// end of suTie.cpp
