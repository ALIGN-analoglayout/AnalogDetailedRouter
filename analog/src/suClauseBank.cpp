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
//! \date   Fri Sep 29 08:58:15 2017

//! \file   suClauseBank.cpp
//! \brief  A collection of methods of the class suClauseBank.

// std includes
#include <string>
#include <vector>
#include <iostream>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes

// module include
#include <suClauseBank.h>

namespace amsr
{
  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------
  
  std::vector<sutype::clause_t*> suClauseBank::_clausePool;
  sutype::uvi_t suClauseBank::_numCreatedClauses = 0;
  
  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------


  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------
  
  // static
  void suClauseBank::delete_clause_pool ()
  {
    SUINFO(1) << "Delete " << _clausePool.size() << " clauses from the pool." << std::endl;
    
    SUASSERT (suClauseBank::_numCreatedClauses == _clausePool.size(), "There's a leak of clauses somewhere. Some clauses were loaned and not returned. Debug and fix it.");
    
    for (sutype::uvi_t i=0; i < suClauseBank::_clausePool.size(); ++i) {
      sutype::clause_t * clause = suClauseBank::_clausePool[i];
      //SUINFO(1) << "clause capacity = " << clause->capacity() << std::endl;
      delete clause;
    }
    
    suClauseBank::_clausePool.clear();
    suClauseBank::_numCreatedClauses = 0;
    
  } // end of suClauseBank::delete_clause_pool
  
  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------
  
} // end of namespace amsr

// end of suClauseBank.cpp
