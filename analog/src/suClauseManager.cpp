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
//! \date   Fri Sep 29 13:48:15 2017

//! \file   suClauseManager.cpp
//! \brief  A collection of methods of the class suClauseManager.

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
#include <suStatic.h>

// module include
#include <suClauseManager.h>

namespace amsr
{

  // used to run static compare_clauses_
  std::map<sutype::clauseid_t,sutype::clause_t> suClauseManager::_temporaryStaticIdToStoredClause;

  // id=1 is used for constant-0; id=2 is used for constant-1 in suSatSolverWrapper::init_constants_
  sutype::clauseid_t suClauseManager::_idMaxFixedCluaseId = 2;

  
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

  // used for debug
  bool suClauseManager::satindex_has_stored_clauses (sutype::satindex_t satindex)
    const
  {
    SUASSERT (!_idToStoredClause.empty(), "No stored clasuses so far. It's very strange.");
    
    bool ret = satindex_has_stored_clauses_ (satindex);

    if (!ret) return ret;

    // print
    if (1) {

      const sutype::clauseids_t & storedClauseIds = get_ids_of_stored_clauses_ (satindex);
      SUASSERT (!storedClauseIds.empty(), "");
      
      SUINFO(1) << "satindex=" << satindex << " has " << storedClauseIds.size() << " stored clauses." << std::endl;
      
      for (sutype::svi_t k=0; k < (sutype::svi_t)storedClauseIds.size(); ++k) {
        
        sutype::clauseid_t id = storedClauseIds[k];
        
        const sutype::clause_t & storedclause = get_stored_clause_by_id_ (id);

        SUINFO(1) << "Clause: " << suStatic::clause_to_str (storedclause) << std::endl;
      }
    }
    
    return ret;
    
  } // end of satindex_has_stored_clauses

  //
  void suClauseManager::add_clause (const sutype::clause_t & clause)
  {
    // check if a new clause is not redundant
    for (sutype::uvi_t i=0; i < clause.size(); ++i) {
      
      sutype::satindex_t satindex = clause[i]; // can be negative
      
      if (!satindex_has_stored_clauses_ (satindex)) continue;

      const sutype::clauseids_t & storedClauseIds = get_ids_of_stored_clauses_ (satindex);
      SUASSERT (!storedClauseIds.empty(), "");
      
      for (sutype::uvi_t k=0; k < storedClauseIds.size(); ++k) {
        
        sutype::clauseid_t id = storedClauseIds[k];
        const sutype::clause_t & storedclause = get_stored_clause_by_id_ (id);
        
        if (first_clause_is_redundant_ (clause, storedclause)) return; // do not store redundant clause
      }
    }

    // find and remove redundant clauses
    for (sutype::uvi_t i=0; i < clause.size(); ++i) {
      
      sutype::satindex_t satindex = clause[i]; // can be negative

      if (!satindex_has_stored_clauses_ (satindex)) continue;

      sutype::clauseids_t & storedClauseIds = get_ids_of_stored_clauses_ (satindex);
      SUASSERT (!storedClauseIds.empty(), "");
      
      for (sutype::svi_t k=0; k < (sutype::svi_t)storedClauseIds.size(); ++k) {
        
        sutype::clauseid_t id = storedClauseIds[k];
        
        const sutype::clause_t & storedclause = get_stored_clause_by_id_ (id);
        
        if (!first_clause_is_redundant_ (storedclause, clause)) continue;
        
        // remove clause's id from the list of clauses of satindex
        storedClauseIds[k] = storedClauseIds.back();
        storedClauseIds.pop_back();
        --k;
        
        const sutype::uvi_t prevsize = storedClauseIds.size();
        
        // can't modify our storedClauseIds
        remove_id_from_satindices_ (storedclause, id, satindex);
        
        SUASSERT (storedClauseIds.size() == prevsize, "Somehow storedClauseIds were affected");

        //SUINFO(1) << "Removed a clause (" << _idToStoredClause.size() << "): " << suStatic::clause_to_str (storedclause) << std::endl;
        
        // now, can erase the clause
        _idToStoredClause.erase (id);
      }
    }
    
    // store a new unique clause
    store_clause_ (clause);
    
  } // suClauseManager::add_clause

  //
  sutype::clauses_t suClauseManager::get_clauses_to_emit ()
  {
    sutype::clauseids_t ids;
    
    sutype::clauseid_t maxid = _idOfTheLastEmittedClause;
    
    suClauseManager::_temporaryStaticIdToStoredClause.clear();

    for (const auto & iter : _idToStoredClause) {
      
      sutype::clauseid_t id = iter.first;
      const sutype::clause_t & clause = iter.second;
      
      if (id <= _idOfTheLastEmittedClause) continue;

      maxid = std::max (id, maxid);
      
      ids.push_back (id);

      suClauseManager::_temporaryStaticIdToStoredClause[id] = clause;

      //SUINFO(1) << "AAA: id=" << id << "; clause=" << suStatic::clause_to_str (clause) << std::endl;
    }
    
    _idOfTheLastEmittedClause = maxid;

    // sort clauses because solvers may be undetemenistic
    std::sort (ids.begin(), ids.end(), compare_clauses_);
    
    sutype::clauses_t clauses;

    for (sutype::uvi_t i=0; i < ids.size(); ++i) {
      
      sutype::clauseid_t id = ids[i];
      
      const sutype::clause_t & clause = suClauseManager::_temporaryStaticIdToStoredClause.at (id);
      
      clauses.push_back (clause);

      //SUINFO(1) << "BBB: id=" << id << "; clause=" << suStatic::clause_to_str (clause) << std::endl;
    }
    
    return clauses;
    
  } // end of suClauseManager::get_clauses_to_emit
  
  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------

  // clauseToCheck (a, b, c) is redundant if, for example, baseClause = (a, b)
  bool suClauseManager::first_clause_is_redundant_ (const sutype::clause_t & clauseToCheck,
                                                    const sutype::clause_t & baseClause)
    const
  {
    SUASSERT (!clauseToCheck.empty(), "");
    SUASSERT (!baseClause.empty(), "");
    
    // quick check
    if (clauseToCheck.size() < baseClause.size()) return false;

    for (sutype::uvi_t i=0; i < baseClause.size(); ++i) {

      sutype::satindex_t satindex = baseClause[i];
      
      if (std::find (clauseToCheck.begin(), clauseToCheck.end(), satindex) == clauseToCheck.end()) return false;
    }
    
    return true;
    
  } // end of suClauseManager::first_clause_is_redundant_

  //
  void suClauseManager::store_clause_ (const sutype::clause_t & clause)
  {
    sutype::clauseid_t id = get_next_clause_id_ ();
    SUASSERT (id > 0, "");
    
    _idToStoredClause [id] = clause;

    //SUINFO(1) << "Stored a clause (" << _idToStoredClause.size() << "): " << suStatic::clause_to_str (clause) << std::endl;
    
    for (sutype::uvi_t i=0; i < clause.size(); ++i) {
      
      sutype::satindex_t satindex = clause[i]; // can be negative
      SUASSERT (satindex != 0, "");

      if (satindex > 0) {
        if (satindex >= (sutype::satindex_t)_posSatindexToStoredClauseIds.size()) {
          _posSatindexToStoredClauseIds.resize (satindex + 1);
        }
        _posSatindexToStoredClauseIds[satindex].push_back (id);
      }
      else {
        if (-satindex >= (sutype::satindex_t)_negSatindexToStoredClauseIds.size()) {
          _negSatindexToStoredClauseIds.resize (-satindex + 1);
        }
        _negSatindexToStoredClauseIds[-satindex].push_back (id);
      }
    }
    
  } // end of suClauseManager::store_clause_

  //
  void suClauseManager::remove_id_from_satindices_ (const sutype::clause_t & clause,
                                                    sutype::clauseid_t id,
                                                    sutype::satindex_t satindexToSkip)
  {
    for (sutype::uvi_t i=0; i < clause.size(); ++i) {

      sutype::satindex_t satindex = clause[i]; // can be negative
      if (satindex == satindexToSkip) continue;

      SUASSERT (satindex_has_stored_clauses_ (satindex), "");
      sutype::clauseids_t & ids = get_ids_of_stored_clauses_ (satindex);
      
      sutype::uvi_t index = std::find (ids.begin(), ids.end(), id) - ids.begin();
      SUASSERT (index >= 0, "");
      SUASSERT (index <= ids.size(), "");
      
      if (index < ids.size()) {
        ids[index] = ids.back();
        ids.pop_back();
      }
    }
    
  } // end of suClauseManager::remove_id_from_satindices_

  // static
  bool suClauseManager::compare_clauses_ (sutype::clauseid_t id1,
                                          sutype::clauseid_t id2)
  {
    SUASSERT (id1 > 0, "");
    SUASSERT (id2 > 0, "");
    
    // first Ids are used for fixed constants
    if (id1 <= suClauseManager::_idMaxFixedCluaseId || id2 <= suClauseManager::_idMaxFixedCluaseId)
      return id1 < id2;
    
    const sutype::clause_t & clause1 = suClauseManager::_temporaryStaticIdToStoredClause.at(id1);
    const sutype::clause_t & clause2 = suClauseManager::_temporaryStaticIdToStoredClause.at(id2);
    
    if (clause1.size() != clause2.size())
      return (clause1.size() < clause2.size());

    for (sutype::uvi_t i=0; i < clause1.size(); ++i) {
      
      sutype::satindex_t satindex1 = clause1[i];
      sutype::satindex_t satindex2 = clause2[i];

      if (satindex1 != satindex2)
        return (satindex1 < satindex2);
    }

    return (id1 < id2);
    
  } // end of suClauseManager::compare_clauses_
  
} // end of namespace amsr

// end of suClauseManager.cpp
