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
//! \date   Fri Sep 29 10:15:32 2017

//! \file   suSatSolverUnitTestB.h
//! \brief  Classical chess problem. Here, I check SAT and Boolean counters.

#ifndef _suSatSolverUnitTestB_h_
#define _suSatSolverUnitTestB_h_

// system includes

// std includes

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
  class suSatSolverUnitTestB
  {
  private:

  private:

    //! default constructor
    suSatSolverUnitTestB ()
    {
      SUASSERT (false, "The method is not expected to be called.");
      init_ ();

    } // end of suSatSolverUnitTestB

    //! copy constructor
    suSatSolverUnitTestB (const suSatSolverUnitTestB & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suSatSolverUnitTestB

    //! assignment operator
    suSatSolverUnitTestB & operator = (const suSatSolverUnitTestB & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suSatSolverUnitTestB ()
    {
      SUASSERT (false, "The method is not expected to be called.");
      
    } // end of ~suSatSolverUnitTestB

  private:

    //! init all class members
    inline void init_ ()
    {
    } // end of init_

    //! copy all class members
    inline void copy_ (const suSatSolverUnitTestB & rs)
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
    static void run_unit_test (const unsigned problemsize,
                               const int implementationid);
    
  private:

    //
    static void check_diagonal_sum_ (int x1,
                                     int y1,
                                     const sutype::clauses_t & board);
    
  }; // end of class suSatSolverUnitTestB

} // end of namespace amsr

#endif // _suSatSolverUnitTestB_h_

// end of suSatSolverUnitTestB.h

