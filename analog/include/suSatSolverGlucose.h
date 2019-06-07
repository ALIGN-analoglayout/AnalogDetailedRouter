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
//! \date   Thu Oct 3 09:45:43 2017

//! \file   suSatSolverGlucose.h
//! \brief  A header of the class suSatSolverGlucose.

#ifndef _suSatSolverGlucose_h_
#define _suSatSolverGlucose_h_

// system includes

// std includes
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>
#include <suSatSolver.h>

// external includes
#include <core/SolverTypes.h>

namespace Glucose
{
  class Solver;
   
} // end of namespace Glucose

namespace amsr
{
  //! 
  class suSatSolverGlucose : public suSatSolver
  {
  private:

    //
    Glucose::Solver * _solver;

    //
    Glucose::vec<Glucose::Lit> _sharedLits;
    
  public:
    
    //! default constructor
    suSatSolverGlucose ();
    
    //! copy constructor
    suSatSolverGlucose (const suSatSolverGlucose & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suSatSolverGlucose

    //! assignment operator
    suSatSolverGlucose & operator = (const suSatSolverGlucose & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suSatSolverGlucose ();

  private:

    //! init all class members
    inline void init_ ()
    {
      _solver = 0;
            
    } // end of init_
    
    //! copy all class members
    inline void copy_ (const suSatSolverGlucose & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");
      
    } // end of copy_
    
  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

  public:

    // virtual
    void set_default_solver_params ();

    // virtual
    void emit_clause (const sutype::clause_t & clause);

    // virtual
    bool simplify ();

    // virtual
    bool solve ();

    // virtual
    bool solve (const sutype::clause_t & assumptions);

    // virtual
    sutype::bool_t get_modeled_value (sutype::satindex_t satindex);
    
  private:

    //
    inline sutype::glucosevar_t satindex_to_solver_var_ (sutype::satindex_t satindex)
      const
    {
      return ((satindex > 0) ? (satindex - 1) : (-satindex - 1));
      
    } // end of satindex_to_solver_var_

    //!
    void add_vars_ (sutype::glucosevar_t var);
    
    //
    bool solve_ (const Glucose::vec<Glucose::Lit> & assumptions);
    
    // virtual
    void populate_model_ ();
    
  }; // end of class suSatSolverGlucose

} // end of namespace amsr

#endif // _suSatSolverGlucose_h_

// end of suSatSolverGlucose.h

