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

//! \file   suSatSolverUnitTestD.h
//! \brief  Checks incrementally added constraints.

#ifndef _suSatSolverUnitTestD_h_
#define _suSatSolverUnitTestD_h_

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
  class suSatSolverUnitTestD
  {
  private:

  private:

    //! default constructor
    suSatSolverUnitTestD ()
    {
      SUASSERT (false, "The method is not expected to be called.");
      init_ ();

    } // end of suSatSolverUnitTestD

    //! copy constructor
    suSatSolverUnitTestD (const suSatSolverUnitTestD & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suSatSolverUnitTestD

    //! assignment operator
    suSatSolverUnitTestD & operator = (const suSatSolverUnitTestD & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suSatSolverUnitTestD ()
    {
      SUASSERT (false, "The method is not expected to be called.");
      
    } // end of ~suSatSolverUnitTestD

  private:

    //! init all class members
    inline void init_ ()
    {
    } // end of init_

    //! copy all class members
    inline void copy_ (const suSatSolverUnitTestD & rs)
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
    static void run_unit_test (const unsigned problemsize);

  private:

    //
    static void check_diagonal_sum_ (int x1,
                                     int y1,
                                     const sutype::clauses_t & board);
    
  }; // end of class suSatSolverUnitTestD

} // end of namespace amsr

#endif // _suSatSolverUnitTestD_h_

// end of suSatSolverUnitTestD.h

