// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Apr 17 10:55:24 2018

//! \file   suSatSolverUnitTestE.h
//! \brief  A header of the class suSatSolverUnitTestE.

#ifndef _suSatSolverUnitTestE_h_
#define _suSatSolverUnitTestE_h_

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
  class suSatSolverUnitTestE
  {
  private:

  public:

    //! default constructor
    suSatSolverUnitTestE ()
    {
      init_ ();

    } // end of suSatSolverUnitTestE

  private:

    //! copy constructor
    suSatSolverUnitTestE (const suSatSolverUnitTestE & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suSatSolverUnitTestE

    //! assignment operator
    suSatSolverUnitTestE & operator = (const suSatSolverUnitTestE & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suSatSolverUnitTestE ()
    {
    } // end of ~suSatSolverUnitTestE

  private:

    //! init all class members
    inline void init_ ()
    {
    } // end of init_

    //! copy all class members
    inline void copy_ (const suSatSolverUnitTestE & rs)
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
    static void optimize_round_ (std::vector <std::vector<int> > & round);
    
    //
    static void enumerate_round_options_ (std::vector <std::vector<int> > & roundOptions, // to store
                                          const std::vector <std::vector <std::vector<int> > > & roundGameOptions, // to enumerate
                                          const unsigned index);

    //
    static void prune_round_options_ (std::vector <std::vector<int> > & roundOptions);

    //
    static int round_option_is_legal_ (const std::vector<int> & roundOption);
    
  }; // end of class suSatSolverUnitTestE

} // end of namespace amsr

#endif // _suSatSolverUnitTestE_h_

// end of suSatSolverUnitTestE.h

