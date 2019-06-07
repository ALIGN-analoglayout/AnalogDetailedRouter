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
//! \date   Fri Sep 29 08:58:07 2017

//! \file   suClauseBank.h
//! \brief  A header of the class suClauseBank.

#ifndef _suClauseBank_h_
#define _suClauseBank_h_

// system includes

// std includes
#include <vector>

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
  class suClauseBank
  {
  private:

    //
    static std::vector<sutype::clause_t*> _clausePool;

    // it's very important to release every loaned clause to avoid an explosion of created objects
    // at the end of the flow, the number of loaned clauses must be exactly zero
    static sutype::uvi_t _numCreatedClauses;
    
  private:

    //! default constructor
    suClauseBank ()
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();

    } // end of suClauseBank

    //! copy constructor
    suClauseBank (const suClauseBank & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suClauseBank

    //! assignment operator
    suClauseBank & operator = (const suClauseBank & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suClauseBank ()
    {
      SUASSERT (false, "The method is not expected to be called.");
      
    } // end of ~suClauseBank
    
  private:

    //! init all class members
    inline void init_ ()
    {
    } // end of init_

    //! copy all class members
    inline void copy_ (const suClauseBank & rs)
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
    static inline sutype::clause_t & loan_clause ()
    {      
      if (suClauseBank::_clausePool.empty()) {
        
        suClauseBank::_numCreatedClauses += 128;
        
        for (sutype::uvi_t i = 0; i < 128; ++i) {
          suClauseBank::_clausePool.push_back (new sutype::clause_t (2048, 0));
        }
      }
      
      sutype::clause_t * clause = suClauseBank::_clausePool.back();
      
      clause->clear();
      
      _clausePool.pop_back();
      
      return *clause;
      
    } // end of loan_clause
    
    //
    static inline void return_clause (sutype::clause_t & clause)
    {
      //for (auto iter : suClauseBank::_clausePool) { SUASSERT (iter != &clause, ""); }
      
      suClauseBank::_clausePool.push_back (&clause);
      
    } // end of return_clause
    
    //
    static void delete_clause_pool ();

  private:
        
  }; // end of class suClauseBank

} // end of namespace amsr

#endif // _suClauseBank_h_

// end of suClauseBank.h

