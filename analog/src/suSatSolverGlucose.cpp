// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Thu Oct 3 09:45:53 2017

//! \file   suSatSolverGlucose.cpp
//! \brief  A collection of methods of the class suSatSolverGlucose.

// std includes
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suStatic.h>

// external includes
#include <core/Solver.h>
#include <core/SolverTypes.h>

// module include
#include <suSatSolverGlucose.h>

using namespace Glucose;

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------
  
  //
  suSatSolverGlucose::suSatSolverGlucose ()
  {
    SUINFO(1)
      << "Created a solver";

#ifdef _GLUCOSE_2_2_0_
    SUOUT(1)
      << " Glucose2.2";
#endif

#ifdef _GLUCOSE_3_0_0_
    SUOUT(1)
      << " Glucose3.0";
#endif

#ifdef _GLUCOSE_4_1_0_
    SUOUT(1)
      << " Glucose4.1";
#endif
    
    SUOUT(1)
      << std::endl;
    
    _solver = new Glucose::Solver ();
    
  } // end of suSatSolverGlucose

  //
  suSatSolverGlucose::~suSatSolverGlucose ()
  {
    if (_solver)
      delete _solver;
    
  } // end of ~suSatSolverGlucose
  
  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------
  
  //
  void suSatSolverGlucose::set_default_solver_params ()
  {
  } // end of suSatSolverGlucose::set_default_solver_params

  //
  void suSatSolverGlucose::emit_clause (const sutype::clause_t & clause)
  {    
    // reset current result
    clear_model_ ();
    
    for (sutype::uvi_t i=0; i < clause.size(); ++i) {
      SUASSERT (clause[i] != 0, i);
    }
    
    Glucose::vec<Glucose::Lit> & lits = _sharedLits;
    lits.clear();
    
    for (sutype::uvi_t i=0; i < clause.size(); ++i) {

      sutype::satindex_t satindex = clause[i]; // can be negative
      sutype::glucosevar_t var = satindex_to_solver_var_ (satindex);
      add_vars_ (var);
      
#ifdef _GLUCOSE_PRO_
      lits.push ((satindex > 0) ? Glucose::mkLit(var) : Glucose::pro_invert_lit(Glucose::mkLit(var)));
#else
      lits.push ((satindex > 0) ? Glucose::mkLit(var) : ~Glucose::mkLit(var));
#endif

    }
    
    _solver->addClause_ (lits);

    //SUINFO(1) << "suSatSolverGlucose::emit_clause: " << suStatic::clause_to_str (clause) << "; Found " << _solver->nClauses() << " clauses." << std::endl;
    
  } // end of suSatSolverGlucose::emit_clause
  
  //
  bool suSatSolverGlucose::simplify ()
  {
    // reset current result
    clear_model_ ();

    //SUINFO(1) << "Simplify." << std::endl;
    //SUINFO(1) << "Found " << _solver->nVars() << " vars." << std::endl;
    //SUINFO(1) << "Found " << _solver->nClauses() << " clauses." << std::endl;
    
    bool ok = _solver->simplify();
    
#ifdef _GLUCOSE_2_2_0_
    _solver->reduceDB();
#endif // _GLUCOSE_2_2_0_
    
    return ok;  
    
  } // end of suSatSolverGlucose::simplify
  
  //
  bool suSatSolverGlucose::solve ()
  {
    Glucose::vec<Glucose::Lit> & assumptions = _sharedLits;
    
    assumptions.clear();
    
    return solve_ (assumptions);
    
  } // end of suSatSolverGlucose::solve

  //
  bool suSatSolverGlucose::solve (const sutype::clause_t & assumptions)
  {    
    Glucose::vec<Glucose::Lit> & assumptions2 = _sharedLits;
    
    assumptions2.clear();
    
    for (sutype::uvi_t i=0; i < assumptions.size(); ++i) {

      sutype::satindex_t satindex = assumptions[i]; // can be negative
      sutype::glucosevar_t var = satindex_to_solver_var_ (satindex);
      add_vars_ (var);

#ifdef _GLUCOSE_PRO_
      assumptions2.push ((satindex > 0) ? Glucose::mkLit(var) : Glucose::pro_invert_lit (Glucose::mkLit(var)));
#else 
      assumptions2.push ((satindex > 0) ? Glucose::mkLit(var) : ~Glucose::mkLit(var));
#endif

    }
    
    return solve_ (assumptions2);
    
  } // end of suSatSolverGlucose::solve

  // virtual
  sutype::bool_t suSatSolverGlucose::get_modeled_value (sutype::satindex_t satindex)
  {
    SUASSERT (_modelIsValid, "Model is not valid");
    SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");
    
    sutype::satindex_t abssatindex = abs (satindex);
    
    lbool value = _solver->modelValue ((Var)(abssatindex - 1));
    
    if        (value == l_True)  { return (satindex > 0) ? sutype::bool_true : sutype::bool_false;
    } else if (value == l_Undef) { return sutype::bool_undefined;
    } else if (value == l_False) { return (satindex > 0) ? sutype::bool_false : sutype::bool_true;
    } else {
      SUASSERT (false, "Unexpected lbool value: satindex=" << satindex);
    }

    return sutype::bool_undefined;
    
  } // end of suSatSolverGlucose::get_modeled_value
  
  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------

  //!
  void suSatSolverGlucose::add_vars_ (sutype::glucosevar_t var)
  {
    // add vars
    while (var >= _solver->nVars()) {
        
      // used to create a new default var
      const bool polarity = true;
      const bool dvar     = true;

      bool newVarCreated = false;

#ifdef _GLUCOSE_2_2_0_
      SUASSERT (!newVarCreated, "");
      const bool important = false;        
      _solver->newVar (polarity, dvar, important);
      newVarCreated = true;
#endif // _GLUCOSE_2_2_0_

#ifdef _GLUCOSE_3_0_0_
      SUASSERT (!newVarCreated, "");
      _solver->newVar (polarity, dvar);
      newVarCreated = true;
#endif // _GLUCOSE_3_0_0_

#ifdef _GLUCOSE_4_1_0_
      SUASSERT (!newVarCreated, "");
      _solver->newVar (polarity, dvar);
      newVarCreated = true;
#endif // _GLUCOSE_4_1_0_

      SUASSERT (newVarCreated, "");
    } 
    
  } // end of suSatSolverGlucose::add_vars_

  //!
  bool suSatSolverGlucose::solve_ (const Glucose::vec<Glucose::Lit> & assumptions)
  {
    // reset current result
    clear_model_ ();
        
    if (l_True == _solver->solveLimited (assumptions)) {
      _modelIsValid = true;
    }
    
    return _modelIsValid;
    
  } // end of suSatSolverGlucose::solve_
  
  //!
  void suSatSolverGlucose::populate_model_ ()
  {
    SUASSERT (_modelIsValid, "");
    SUASSERT (_model.empty(), "");
        
    _model.push_back (sutype::bool_undefined); // zero index is not used
    
    for (sutype::glucosevar_t i=0; i < _solver->nVars(); ++i) {

      lbool value = _solver->modelValue ((Var)i);
      
      if        (value == l_True)  { _model.push_back (sutype::bool_true);
      } else if (value == l_Undef) { _model.push_back (sutype::bool_undefined);
      } else if (value == l_False) { _model.push_back (sutype::bool_false);
      } else {
        SUASSERT (false, "");
      }
    }
    
  } // end of suSatSolverGlucose::getModel
  
} // end of namespace amsr

// end of suSatSolverGlucose.cpp
