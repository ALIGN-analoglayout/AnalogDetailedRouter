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
//! \date   Fri Sep 29 13:48:05 2017

//! \file   suClauseManager.h
//! \brief  A header of the class suClauseManager.

#ifndef _suClauseManager_h_
#define _suClauseManager_h_

// system includes

// std includes
#include <map>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes

namespace amsr
{
  // classes

  //! 
  class suClauseManager
  {

    // used to run static compare_clauses_
    static std::map<sutype::clauseid_t,sutype::clause_t> _temporaryStaticIdToStoredClause;

    static sutype::clauseid_t _idMaxFixedCluaseId;       // a little hardcode for fixed clauses

  private:
    
    //
    sutype::clauseid_t _idOfTheLastStoredClause;  // used to create an unique id for every new unique clause
    sutype::clauseid_t _idOfTheLastEmittedClause; // used not to emit same clauses twice
    
    //
    std::map<sutype::clauseid_t,sutype::clause_t> _idToStoredClause;
    
    // satindex to clauses
    std::vector<sutype::clauseids_t> _posSatindexToStoredClauseIds;
    std::vector<sutype::clauseids_t> _negSatindexToStoredClauseIds;
    
  public:

    //! default constructor
    suClauseManager ()
    {
      init_ ();

    } // end of suClauseManager

    //! copy constructor
    suClauseManager (const suClauseManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suClauseManager

    //! assignment operator
    suClauseManager & operator = (const suClauseManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suClauseManager ()
    {
    } // end of ~suClauseManager

  private:

    //! init all class members
    inline void init_ ()
    {
      _idOfTheLastStoredClause  = 0;
      _idOfTheLastEmittedClause = 0;
      
    } // end of init_

    //! copy all class members
    inline void copy_ (const suClauseManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

  public:

    //
    void add_clause (const sutype::clause_t & clause);

    //
    sutype::clauses_t get_clauses_to_emit ();

    //
    bool satindex_has_stored_clauses (sutype::satindex_t satindex)
      const;
    
  private:

    //
    inline bool satindex_has_stored_clauses_ (sutype::satindex_t satindex)
      const
    {
      SUASSERT (satindex != 0, "");

      if (satindex > 0) {
        sutype::satindex_t abssatindex = satindex;
        if (abssatindex < (sutype::satindex_t)_posSatindexToStoredClauseIds.size() && !_posSatindexToStoredClauseIds[abssatindex].empty()) {
          return true;
        }
      }
      
      else {
        sutype::satindex_t abssatindex = -satindex;
        if (abssatindex < (sutype::satindex_t)_negSatindexToStoredClauseIds.size() && !_negSatindexToStoredClauseIds[abssatindex].empty()) {
          return true;
        }
      }
      
      return false;
      
    } // end of satindex_has_stored_clauses_
    
    //
    inline const sutype::clauseids_t & get_ids_of_stored_clauses_ (sutype::satindex_t satindex)
      const
    {
      SUASSERT (satindex != 0, "");
      
      if (satindex > 0) {
        sutype::satindex_t abssatindex = satindex;
        SUASSERT (abssatindex < (sutype::satindex_t)_posSatindexToStoredClauseIds.size(), "");
        return _posSatindexToStoredClauseIds [abssatindex];
      }
      else {
        sutype::satindex_t abssatindex = -satindex;
        SUASSERT (abssatindex < (sutype::satindex_t)_negSatindexToStoredClauseIds.size(), "");
        return _negSatindexToStoredClauseIds [abssatindex];
      }
      
    } // get_ids_of_stored_clauses_
    
    //
    inline sutype::clauseids_t & get_ids_of_stored_clauses_ (sutype::satindex_t satindex)
    {
      SUASSERT (satindex != 0, "");
      
      if (satindex > 0) {
        sutype::satindex_t abssatindex = satindex;
        SUASSERT (abssatindex < (sutype::satindex_t)_posSatindexToStoredClauseIds.size(), "");
        return _posSatindexToStoredClauseIds [abssatindex];
      }
      else {
        sutype::satindex_t abssatindex = -satindex;
        SUASSERT (abssatindex < (sutype::satindex_t)_negSatindexToStoredClauseIds.size(), "");
        return _negSatindexToStoredClauseIds [abssatindex];
      }
      
    } // get_ids_of_stored_clauses_
    
    //
    inline const sutype::clause_t & get_stored_clause_by_id_ (sutype::clauseid_t id)
      const
    {
      return _idToStoredClause.at(id);
      
    } // end of get_stored_clause_by_id_

    //
    inline sutype::clauseid_t get_next_clause_id_ ()
    {
      ++_idOfTheLastStoredClause;

      return _idOfTheLastStoredClause;
      
    } // end of get_next_clause_id_

    //
    bool first_clause_is_redundant_ (const sutype::clause_t & clauseToCheck,
                                     const sutype::clause_t & baseClause)
      const;
      
    //
    void store_clause_ (const sutype::clause_t & clause);
    
    //
    void remove_id_from_satindices_ (const sutype::clause_t & clause,
                                     sutype::clauseid_t id,
                                     sutype::satindex_t satindexToSkip);

    //
    static bool compare_clauses_ (sutype::clauseid_t id1,
                                  sutype::clauseid_t id2);
    
  }; // end of class suClauseManager

} // end of namespace amsr

#endif // _suClauseManager_h_

// end of suClauseManager.h

