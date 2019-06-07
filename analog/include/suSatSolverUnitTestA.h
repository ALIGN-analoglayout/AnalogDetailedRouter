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
//! \date   Thu Sep 28 10:16:42 2017

//! \file   suSatSolverUnitTestA.h
//! \brief  Classical Einstein's problem. It has the only correct answer if SAT coded correctly.

#ifndef _suSatSolverUnitTestA_h_
#define _suSatSolverUnitTestA_h_

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
  class suSatSolverUnitTestA
  {
  private:

  private:

    //! default constructor
    suSatSolverUnitTestA ()
    {
      SUASSERT (false, "The method is not expected to be called.");
      init_ ();

    } // end of suSatSolverUnitTestA

    //! copy constructor
    suSatSolverUnitTestA (const suSatSolverUnitTestA & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suSatSolverUnitTestA

    //! assignment operator
    suSatSolverUnitTestA & operator = (const suSatSolverUnitTestA & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suSatSolverUnitTestA ()
    {
      SUASSERT (false, "The method is not expected to be called.");
      
    } // end of ~suSatSolverUnitTestA

  private:

    //! init all class members
    inline void init_ ()
    {
    } // end of init_

    //! copy all class members
    inline void copy_ (const suSatSolverUnitTestA & rs)
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
    static void run_unit_test ();

  private:

    //
    static void unit_test_basic_rules (const std::vector<int> & vars,
                                       const std::map<std::pair<int,int>, sutype::satindex_t> & itIsInPosition);
    
    //
    static void unit_test_bind_two_vars (int var1, 
                                         int var2,
                                         const std::map<std::pair<int,int>, sutype::satindex_t> & itIsInPosition);
    
    //
    static void unit_test_two_vars_are_in_conflict (int var1, 
                                                    int var2,
                                                    const std::map<std::pair<int,int>, sutype::satindex_t> & itIsInPosition);
    
    //
    static void unit_test_two_vars_are_next_to_each_other (int var1, // Portland
                                                           int var2, // New York
                                                           const std::map<std::pair<int,int>, sutype::satindex_t> & itIsInPosition,
                                                           sutype::side_t side); // New York is on the east
    
  }; // end of class suSatSolverUnitTestA

} // end of namespace amsr

#endif // _suSatSolverUnitTestA_h_

// end of suSatSolverUnitTestA.h

