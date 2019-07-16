// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Thu Sep 28 09:45:43 2017

//! \file   suSatSolverFiver.h
//! \brief  A header of the class suSatSolverFiver.

#ifndef _suSatSolverFiver_h_
#define _suSatSolverFiver_h_

// system includes

// std includes
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>
#include <suSatSolver.h>

#ifdef _ENABLE_FIVER_

namespace fiver
{
  class CFiver;
  
} // end of namespace fiver

#endif // _ENABLE_FIVER_

namespace amsr
{
  //! 
  class suSatSolverFiver : public suSatSolver
  {
  private:

#ifdef _ENABLE_FIVER_

    //
    fiver::CFiver * _solver;

#endif

    // pre-created object to avoid multiple creations and deletions of clauses
    sutype::clause_t _sharedClause;    
    
  public:
    
    //! default constructor
    suSatSolverFiver ();
    
    //! copy constructor
    suSatSolverFiver (const suSatSolverFiver & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suSatSolverFiver

    //! assignment operator
    suSatSolverFiver & operator = (const suSatSolverFiver & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suSatSolverFiver ();

  private:

    //! init all class members
    inline void init_ ()
    {

#ifdef _ENABLE_FIVER_

      _solver = 0;

#endif // _ENABLE_FIVER_

      _sharedClause.resize (2048, 0);
      _sharedClause.clear();
      
    } // end of init_
    
    //! copy all class members
    inline void copy_ (const suSatSolverFiver & rs)
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

    //! satindices are to be evaluated as constant-1
    bool solve_ (const sutype::clause_t & assumptions);
    
    // virtual
    void populate_model_ ();
    
  }; // end of class suSatSolverFiver

} // end of namespace amsr

#endif // _suSatSolverFiver_h_

// end of suSatSolverFiver.h

