// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Mon Oct  2 09:46:25 2017

//! \file   suSatSolverUnitTestC.h
//! \brief  Checks boolean counters -- both hard implementation and assumptions.

#ifndef _suSatSolverUnitTestC_h_
#define _suSatSolverUnitTestC_h_

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
  class suSatSolverUnitTestC
  {
  private:

  public:

    //! default constructor
    suSatSolverUnitTestC ()
    {
      init_ ();

    } // end of suSatSolverUnitTestC

    //! copy constructor
    suSatSolverUnitTestC (const suSatSolverUnitTestC & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suSatSolverUnitTestC

    //! assignment operator
    suSatSolverUnitTestC & operator = (const suSatSolverUnitTestC & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suSatSolverUnitTestC ()
    {
    } // end of ~suSatSolverUnitTestC

  private:

    //! init all class members
    inline void init_ ()
    {
    } // end of init_

    //! copy all class members
    inline void copy_ (const suSatSolverUnitTestC & rs)
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
    
  }; // end of class suSatSolverUnitTestC

} // end of namespace amsr

#endif // _suSatSolverUnitTestC_h_

// end of suSatSolverUnitTestC.h

