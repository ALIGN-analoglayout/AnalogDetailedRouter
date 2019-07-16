// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Oct  3 13:18:06 2017

//! \file   suSatSolver.h
//! \brief  A header of the class suSatSolver.

#ifndef _suSatSolver_h_
#define _suSatSolver_h_

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
  class suSatSolver
  {
  private:

  protected:

    //
    sutype::bools_t _model;

    //
    bool _modelIsValid;

    //
    bool _keepModelUnchanged;
    
  public:
    
    //! default constructor
    suSatSolver ()
    {
      init_ ();

    } // end of suSatSolver

    //! copy constructor
    suSatSolver (const suSatSolver & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);
      
    } // end of suSatSolver

    //! assignment operator
    suSatSolver & operator = (const suSatSolver & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suSatSolver ()
    {
    } // end of ~suSatSolver

  private:

    //! init all class members
    inline void init_ ()
    {
      _modelIsValid = false;
      _keepModelUnchanged = false;

    } // end of init_
    
    //! copy all class members
    inline void copy_ (const suSatSolver & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

    //
    inline bool model_is_valid () const { return _modelIsValid; }

    //
    inline void keep_model_unchanged (bool v) { SUASSERT (_keepModelUnchanged == !v, ""); _keepModelUnchanged = v; }
    
  public:

    //
    virtual void set_default_solver_params ()
    {
      
    } // end of set_default_solver_params

    //
    virtual void emit_clause (const sutype::clause_t & clause)
    {
      SUASSERT (false, "");
      
    } // end of emit_clause

    //
    virtual bool simplify ()
    {
      return true;
      
    } // end of simplify

    //
    virtual bool solve ()
    {
      SUASSERT (false, "");

      return true;
      
    } // end of solve

    //! satindices are to be evaluated as constant-1
    virtual bool solve (const sutype::clause_t & assumptions)
    {
      SUASSERT (false, "");

      return true;
      
    } // end of solve

    //
    virtual sutype::bool_t get_modeled_value (sutype::satindex_t satindex)
    {
      SUASSERT (false, "");

      return sutype::bool_undefined;
      
    } // end of get_modeled_value
    
  public:
    
    //
    const sutype::bools_t & get_model ()
    {
      SUASSERT (_modelIsValid, "");
      
      if (_model.empty()) {
        populate_model_ ();
      }
      
      return _model;
      
    } // end of get_model

  protected:

    //
    virtual void populate_model_ ()
    {
      SUASSERT (false, "");
      
    } // end of populate_model_
    
    //
    inline void clear_model_ ()
    {
      if (_keepModelUnchanged) {
        SUINFO(0) << "Model left unchanged." << std::endl;
        return;
      }
      
      if (!_model.empty()) {
        SUINFO(0) << "Clear model." << std::endl;
      }
      
      _model.clear();
      _modelIsValid = false;
      
    } // end of clear_model_
    
  }; // end of class suSatSolver

} // end of namespace amsr

#endif // _suSatSolver_h_

// end of suSatSolver.h

