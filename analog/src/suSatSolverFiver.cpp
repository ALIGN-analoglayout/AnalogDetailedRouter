// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Thu Sep 28 09:45:53 2017

//! \file   suSatSolverFiver.cpp
//! \brief  A collection of methods of the class suSatSolverFiver.

// system includes
#include <time.h>

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

#ifdef _ENABLE_FIVER_

// external includes
#include <Fiver.h>
#include <FiverTypes.h>
#include <StrippedFiver.h>

#endif // _ENABLE_FIVER_

// module include
#include <suSatSolverFiver.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  //
  suSatSolverFiver::suSatSolverFiver ()
  {

#ifdef _ENABLE_FIVER_
    
    SUINFO(1) << "Created a solver Fiver" << std::endl;

    _solver = new fiver::CFiver ();

#else // _ENABLE_FIVER_

    SUASSERT (false, "Fiver is not supported in this build.");

#endif // _ENABLE_FIVER_
    
  } // end of suSatSolverFiver

  //
  suSatSolverFiver::~suSatSolverFiver ()
  {

#ifdef _ENABLE_FIVER_

    if (_solver) {
      delete _solver;
    }

#endif // _ENABLE_FIVER_
    
  } // end of ~suSatSolverFiver
  
  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------
  
  //
  void suSatSolverFiver::set_default_solver_params ()
  {

#ifdef _ENABLE_FIVER_

    _solver->SetParam ("shrinking", "shrink", "2");
    _solver->SetParam ("inprocessing", "inproc", "0");
    //_solver->SetMode("glucose"); // should behave as close to glucose as possible
    _solver->SetMode("laygen"); // laygen oriented optimizations

#endif // _ENABLE_FIVER_
    
  } // end of suSatSolverFiver::set_default_solver_params

  //
  void suSatSolverFiver::emit_clause (const sutype::clause_t & clause)
  {

#ifdef _ENABLE_FIVER_

    // reset current result
    clear_model_ ();
    
    for (sutype::uvi_t i=0; i < clause.size(); ++i) {
      SUASSERT (clause[i] != 0, i);
    }
        
    _sharedClause.clear ();
    _sharedClause.insert (_sharedClause.end(), clause.begin(), clause.end());
    _sharedClause.push_back (0);
    
    // 2^20 - 1
    const sutype::uvi_t maxNumLiteralsPerClause = 1048575;
    
    if (_sharedClause.size() > maxNumLiteralsPerClause) {
      SUERROR("SAT solver limitation")
        << ": Clause size=" << _sharedClause.size() << " exceeds the known limit of " << maxNumLiteralsPerClause << " literals in a clause. Nothing will be done, Fiver may throw an exception."
        << std::endl;
    }
    
    _solver->AddClause (_sharedClause);

#else // _ENABLE_FIVER_

    SUASSERT (false, "");

#endif // _ENABLE_FIVER_
        
  } // end of suSatSolverFiver::emit_clause
  
  //
  bool suSatSolverFiver::simplify ()
  {

#ifdef _ENABLE_FIVER_

    // reset current result
    clear_model_ ();
    
    _solver->Simplify();
    
    bool ok = _solver->Okay();
    
    return ok;

#else // _ENABLE_FIVER_

    SUASSERT (false, "");

    return true;

#endif // _ENABLE_FIVER_
    
  } // end of suSatSolverFiver::simplify
  
  //
  bool suSatSolverFiver::solve ()
  {

#ifdef _ENABLE_FIVER_

    sutype::clause_t & assumptions = _sharedClause;
    
    assumptions.clear();
    assumptions.push_back (0);
    
    return solve_ (assumptions);

#else // _ENABLE_FIVER_

    SUASSERT (false, "");

    return false;

#endif // _ENABLE_FIVER_
    
  } // end of suSatSolverFiver::solve

  //
  bool suSatSolverFiver::solve (const sutype::clause_t & assumptions)
  {

#ifdef _ENABLE_FIVER_

    sutype::clause_t & assumptions2 = _sharedClause;
    
    assumptions2.clear();
    assumptions2.resize (assumptions.size(), 0);
    assumptions2.clear();
    
    assumptions2.insert (assumptions2.end(), assumptions.begin(), assumptions.end());
    assumptions2.push_back (0);
    
    return solve_ (assumptions2);

#else // _ENABLE_FIVER_

    SUASSERT (false, "");

    return false;

#endif // _ENABLE_FIVER_
    
  } // end of suSatSolverFiver::solve

  sutype::bool_t suSatSolverFiver::get_modeled_value (sutype::satindex_t satindex)
  {

#ifdef _ENABLE_FIVER_

    if (_model.empty()) {
      populate_model_ ();
    }

    SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");
   
    sutype::satindex_t abssatindex = abs (satindex);
    SUASSERT (abssatindex  < (sutype::satindex_t)_model.size(), "abssatindex=" << abssatindex << "; _model.size()=" << _model.size());
    
    sutype::bool_t value = _model [abssatindex];
    SUASSERT (value == sutype::bool_true || value == sutype::bool_false, "");
    
    return ((satindex > 0) ? value : suStatic::invert_bool_value (value));

#else // _ENABLE_FIVER_

    SUASSERT (false, "");

    return sutype::bool_undefined;

#endif // _ENABLE_FIVER_
    
  } // end of suSatSolverFiver::get_modeled_value
  
  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------

  //!
  bool suSatSolverFiver::solve_ (const sutype::clause_t & assumptions)
  {

#ifdef _ENABLE_FIVER_

    // reset current result
    clear_model_ ();
    
    fiver::TResult status = _solver->Solve (assumptions);
    
    if (status == fiver::SAT) {
      _modelIsValid = true;
    }

    return _modelIsValid;

#else // _ENABLE_FIVER_

    SUASSERT (false, "");

    return false;

#endif // _ENABLE_FIVER_
    
  } // end of suSatSolverFiver::solve_

  //!
  void suSatSolverFiver::populate_model_ ()
  {

#ifdef _ENABLE_FIVER_

    SUASSERT (_modelIsValid, "");
    SUASSERT (_model.empty(), "");
    
    std::vector<fiver::TVarValue> model;    
    _solver->GetModel (model);
    
    for (sutype::uvi_t i=0; i < model.size(); ++i) {
      
      if        (model[i] == fiver::True)  { _model.push_back (sutype::bool_true);
      } else if (model[i] == fiver::Undef) { _model.push_back (sutype::bool_undefined);
      } else if (model[i] == fiver::False) { _model.push_back (sutype::bool_false);
      } else {
        SUASSERT (false, "");
      }
    }

#endif // _ENABLE_FIVER_
    
  } // end of suSatSolverFiver::getModel
  
} // end of namespace amsr

// end of suSatSolverFiver.cpp
