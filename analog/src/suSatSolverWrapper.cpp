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
//! \date   Wed Sep 27 14:31:04 2017

//! \file   suSatSolverWrapper.cpp
//! \brief  A collection of methods of the class suSatSolverWrapper.

// std includes
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suClauseBank.h>
#include <suClauseManager.h>
#include <suOptionManager.h>
#include <suSatSolver.h>
#include <suSatSolverFiver.h>
#include <suSatSolverGlucose.h>
#include <suSatSolverUnitTestA.h>
#include <suSatSolverUnitTestB.h>
#include <suSatSolverUnitTestC.h>
#include <suSatSolverUnitTestD.h>
#include <suStatic.h>
#include <suTimeManager.h>

// module include
#include <suSatSolverWrapper.h>

namespace amsr
{
  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------
  
  suSatSolverWrapper * suSatSolverWrapper::_instance = 0;
  
  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  //
  suSatSolverWrapper::suSatSolverWrapper (bool emitConstants)
  {
    init_ ();

    if (use_clause_manager()) {
      _clauseManager = new suClauseManager ();
    }
    
    create_solvers_ ();
    
    set_default_solver_params_ ();
    
    if (emitConstants) {
      init_constants_ ();
    }
    
  } // end of suSatSolverWrapper::suSatSolverWrapper
  
  // destructor
  suSatSolverWrapper::~suSatSolverWrapper ()
  {
    if (_clauseManager) delete _clauseManager;

    for (sutype::uvi_t i=0; i < _solvers.size(); ++i)
      delete _solvers[i];
    
  } // end of suSatSolverWrapper::~suSatSolverWrapper
  
  // ------------------------------------------------------------
  // -
  // --- Static public methods
  // -
  // ------------------------------------------------------------

  // static
  void suSatSolverWrapper::create_instance (bool emitConstants)
  {
    for (int i = 100; i <= 2000; i += 10) {
      
      break;
      if (i == 2) continue;
      if (i == 3) continue;
      //if (i != 800) continue;
      
      SUASSERT (suSatSolverWrapper::_instance == 0, "");
      
      const double sec1 = suTimeManager::instance()->get_cpu_time ();
      
      suSatSolverWrapper::_instance = new suSatSolverWrapper (emitConstants);
      suSatSolverUnitTestB::run_unit_test (i, 3);
      suSatSolverWrapper::delete_instance ();
      
      const double runtime = suTimeManager::instance()->get_cpu_time () - sec1;
      
      SUOUT(1)
        << "AAA"
        << "\t" << i
        << "\t" << runtime
        << std::endl;
    }
    
    // a few unit tests + one alive empty solver
    for (int i = 0; true; ++i) {
      
      SUASSERT (suSatSolverWrapper::_instance == 0, "");
      suSatSolverWrapper::_instance = new suSatSolverWrapper (emitConstants);

      break; // skip unit tests
      
      // suSatSolverUnitTestA: Classical Einstein's problem. It has the only correct answer if SAT coded correctly.
      // suSatSolverUnitTestB: Classical chess problem. Here, I check seeveral implementaions of SAT and Boolean counters.
      // suSatSolverUnitTestC: Checks boolean counters -- both hard implementation and assumptions.
      // suSatSolverUnitTestD: Checks incrementally added constraints.
      
      if        (i ==  0) { suSatSolverUnitTestA::run_unit_test ();
      } else if (i ==  1) { suSatSolverUnitTestB::run_unit_test (8,  0);
      } else if (i ==  2) { suSatSolverUnitTestB::run_unit_test (8,  1);
      } else if (i ==  3) { suSatSolverUnitTestB::run_unit_test (8,  2);
      } else if (i ==  4) { suSatSolverUnitTestB::run_unit_test (8,  3);
      } else if (i ==  5) { suSatSolverUnitTestB::run_unit_test (16, 0);
      } else if (i ==  6) { suSatSolverUnitTestB::run_unit_test (16, 1);
      } else if (i ==  7) { suSatSolverUnitTestB::run_unit_test (16, 2);
      } else if (i ==  8) { suSatSolverUnitTestB::run_unit_test (16, 3);
      } else if (i ==  9) { suSatSolverUnitTestC::run_unit_test (1);
      } else if (i == 10) { suSatSolverUnitTestC::run_unit_test (2);
      } else if (i == 11) { suSatSolverUnitTestC::run_unit_test (8);
      } else if (i == 12) { suSatSolverUnitTestD::run_unit_test (8);
      } else if (i == 13) { suSatSolverUnitTestD::run_unit_test (16);
      } else {
        break;
      }
      
      suSatSolverWrapper::delete_instance ();
    }
    
  } // end of suSatSolverWrapper::create_instance

  // static
  void suSatSolverWrapper::delete_instance ()
  {
    if (suSatSolverWrapper::_instance)
      delete suSatSolverWrapper::_instance;

    suSatSolverWrapper::_instance = 0;
    
  } // end of suSatSolverWrapper::delete_instance

  // ------------------------------------------------------------
  // -
  // --- Static public methods
  // -
  // ------------------------------------------------------------

  //
  sutype::satindex_t suSatSolverWrapper::get_next_sat_index ()
  {
    if (!_availableSatIndices.empty()) {

      sutype::satindex_t satindex = _availableSatIndices.back();
        
      _availableSatIndices.pop_back();

      return satindex;
    }
      
    ++_nextSatIndex;
      
    return _nextSatIndex;
      
  } // end of get_next_sat_index

  //
  bool suSatSolverWrapper::return_sat_index (sutype::satindex_t satindex)
  {
    SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");
    
    if (!is_registered_satindex (satindex)) {
      
      _availableSatIndices.push_back (abs (satindex));
      
      return true;
    }
    
    return false;
    
  } // end of suSatSolverWrapper::return_sat_index
  
  //
  bool suSatSolverWrapper::is_registered_satindex (sutype::satindex_t satindex)
    const
  {
    sutype::satindex_t abssatindex = abs (satindex);
    
    if (abssatindex >= (sutype::satindex_t)_emittedConstants.size()) return false;

    sutype::satindex_t value = _emittedConstants[abssatindex];

    return (value != sutype::UNDEFINED_SAT_INDEX);
    
  } // end of suSatSolverWrapper::is_registered_satindex
  
  //
  bool suSatSolverWrapper::is_registered_constant (int index,
                                                   sutype::satindex_t satindex)
    const
  {
    SUASSERT (index == 0 || index == 1, "");

    sutype::satindex_t abssatindex = abs (satindex);
    
    if (abssatindex >= (sutype::satindex_t)_emittedConstants.size()) return false;
    
    sutype::satindex_t value = _emittedConstants[abssatindex];
    SUASSERT (value == sutype::UNDEFINED_SAT_INDEX ||
              value == get_constant(0) ||
              value == get_constant(1) ||
              value == sutype::NEGATIVE_SAT_INDEX, "");
    
    if (value == sutype::UNDEFINED_SAT_INDEX ||
        value == sutype::NEGATIVE_SAT_INDEX) return false;
    
    if (satindex > 0) {
      return is_constant (index, value, false);
    }
    else {
      return is_constant (1 - index, value, false);
    }
    
  } // end of suSatSolverWrapper::is_registered_constant

  // ------------------------------------------------------------
  // -
  // --- Private wrappers
  // -
  // ------------------------------------------------------------  

  //
  void suSatSolverWrapper::create_solvers_ ()
  {
    std::string defaultsolver ("glucose");
    
    const std::string & solvertype = suOptionManager::instance()->get_string_option ("solver_type", defaultsolver);

    if (solvertype.compare ("fiver") == 0) {
      _solvers.push_back (new suSatSolverFiver   ());
    }
    else if (solvertype.compare ("glucose") == 0) { 
      _solvers.push_back (new suSatSolverGlucose ());
    }
    else if (solvertype.empty()) {
      SUISSUE("Option solver_type is not set. Use a default solver.") << std::endl;
      _solvers.push_back (new suSatSolverGlucose ());
    }
    else {
      SUASSERT (false, "Unexpected solver type: " << solvertype);
    }
    
  } // end of suSatSolverWrapper::create_solvers_
  
  //
  void suSatSolverWrapper::set_default_solver_params_ ()
  {
    for (sutype::uvi_t i=0; i < _solvers.size(); ++i) {
      _solvers[i]->set_default_solver_params ();
    }
    
  } // end of suSatSolverWrapper::set_default_solver_params_

  //
  void suSatSolverWrapper::emit_stored_clauses_to_solvers_ ()
  {
    if (!_clauseManager) return;
    
    sutype::clauses_t clauses = _clauseManager->get_clauses_to_emit ();
    
    for (sutype::uvi_t i=0; i < _solvers.size(); ++i) {
      
      suSatSolver * solver = _solvers[i];

      for (sutype::uvi_t k=0; k < clauses.size(); ++k) {
        solver->emit_clause (clauses[k]);
      }
    }
    
  } // end of suSatSolverWrapper::emit_stored_clauses_to_solvers_
  
  // This procedure emits clauses to a manager. Purposes:
  //   1) Detect an avoid all redundant clauses
  //   2) Sort clauses because some solvers may be non-determenistic
  void suSatSolverWrapper::emit_clause_ (const sutype::clause_t & inputclause)
  {
    SUASSERT (!inputclause.empty(), "");

    //SUINFO(1) << "Input clause: " << suStatic::clause_to_str (inputclause) << std::endl;
    
    if (_unsatisfiable) {
      //SUINFO(1) << "_unsatisfiable" << std::endl;
      return;
    }
    
    sutype::clause_t & clause = suClauseBank::loan_clause ();
    
    for (sutype::uvi_t i=0; i < inputclause.size(); ++i) {
      
      sutype::satindex_t satindex = inputclause[i];
      
      // clause is satisfiable by default
      if (is_constant (1, satindex)) {
        suClauseBank::return_clause (clause);
        return;
      }
      
      if (is_constant (0, satindex)) continue;
      
      clause.push_back (satindex);
    }
    
    // all satindices are constant-0; problem is unsatisfiable
    if (clause.empty()) {
      _unsatisfiable = true;
      clause.push_back (get_constant(0));
      SUISSUE("Problem is unsatisfiable. Clauses will not emitted anymore to the solver.") << std::endl;
      SUASSERT (false, "");
    }

    std::sort (clause.begin(), clause.end());

    // remove identical literals in a sorted list
    sutype::uvi_t counter = 1;
    for (sutype::uvi_t i=1; i < clause.size(); ++i) {
      if (clause[i] != clause[counter-1]) {
        clause[counter] = clause[i];
        ++counter;
      }
    }
    if (counter != clause.size())
      clause.resize (counter);

    ++_nClauses;

    //SUINFO(1) << "Emit clause:  " << suStatic::clause_to_str (inputclause) << std::endl;
    ++_statisticOfEmittedClauses [clause.size()];
    
    // _constant[1] may not be yet defined
    if (clause.size() == 1 && _constant[1] != sutype::UNDEFINED_SAT_INDEX) {
      register_a_constant_one_ (clause.front());
    }

    // register a satindex as emitted
    for (const auto & iter : clause) {
      register_emitted_satindex_ (iter);
    }
    
    // emit to an intermediate manager
    if (use_clause_manager()) {
      SUASSERT (_clauseManager, "");
      _clauseManager->add_clause (clause);
    }
    
    // emit to solvers directly
    else {
      SUASSERT (!_solvers.empty(), "");
      for (sutype::uvi_t i=0; i < _solvers.size(); ++i) {
        _solvers[i]->emit_clause (clause);
      }
    }
    
    suClauseBank::return_clause (clause);
    
  } // end of emit_clause_

  //
  void suSatSolverWrapper::register_emitted_satindex_ (sutype::satindex_t satindex)
  {
    SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");

    sutype::satindex_t abssatindex = abs (satindex);

    if (abssatindex >= (sutype::satindex_t)_emittedConstants.size()) {
      _emittedConstants.resize (abssatindex+1, sutype::UNDEFINED_SAT_INDEX);
    }

    if (_emittedConstants [abssatindex] == sutype::UNDEFINED_SAT_INDEX)
      _emittedConstants [abssatindex] = sutype::NEGATIVE_SAT_INDEX;
    
  } // end of suSatSolverWrapper::register_emitted_satindex_

  //
  void suSatSolverWrapper::register_a_constant_one_ (sutype::satindex_t satindex)
  {
    SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");

    //SUINFO(1) << "Found a constant-1 = " << satindex << std::endl;

    sutype::satindex_t abssatindex = abs (satindex);

    if (abssatindex >= (sutype::satindex_t)_emittedConstants.size()) {
      _emittedConstants.resize (abssatindex+1, sutype::UNDEFINED_SAT_INDEX);
    }

    SUASSERT (_emittedConstants [abssatindex] == sutype::UNDEFINED_SAT_INDEX || _emittedConstants [abssatindex] == sutype::NEGATIVE_SAT_INDEX, "");
    
    if (satindex > 0) {
      _emittedConstants [abssatindex] = get_constant (1);
    }
    else {
      _emittedConstants [abssatindex] = get_constant (0);
    }

    SUASSERT (_emittedConstants [abssatindex] != sutype::UNDEFINED_SAT_INDEX, "");
    SUASSERT (_emittedConstants [abssatindex] != sutype::NEGATIVE_SAT_INDEX, "");
    
  } // end of suSatSolverWrapper::register_a_constant_one_

  // ------------------------------------------------------------
  // -
  // --- Public wrappers
  // -
  // ------------------------------------------------------------  

  //
  bool suSatSolverWrapper::simplify ()
  {
    emit_stored_clauses_to_solvers_ ();

    bool allok = true;
    
    for (sutype::uvi_t i=0; i < _solvers.size(); ++i) {
      
      bool ok = _solvers[i]->simplify ();
      
      SUASSERT (i == 0 || ok == allok, "");
      
      if (!ok)
        allok = false;
    }
    
    return allok;
    
  } // end of suSatSolverWrapper::simplify

  //
  bool suSatSolverWrapper::solve_the_problem ()
  {
    //SUINFO(1) << "Solving without any assumptions..." << std::endl;
    
    //SUINFO(1) << " nLiterals = " << _nextSatIndex << std::endl;
    //SUINFO(1) << " nClauses = " << _nClauses << std::endl;
    
    emit_stored_clauses_to_solvers_ ();

    bool allok = true;
    
    for (sutype::uvi_t i=0; i < _solvers.size(); ++i) {
      
      const double cputime0 = suTimeManager::instance()->get_cpu_time();
      
      bool ok = _solvers[i]->solve ();

      suStatic::increment_elapsed_time (sutype::ts_sat_total_time, suTimeManager::instance()->get_cpu_time() - cputime0);
      
      SUASSERT (i == 0 || ok == allok, "");
      
      if (!ok)
        allok = false;
    }
    
    return allok;
    
  } // end of suSatSolverWrapper::solve_the_problem

  //
  bool suSatSolverWrapper::solve_the_problem (const sutype::clause_t & assumptions)
  {
    //SUINFO(1) << "Solving with " << assumptions.size() << " assumptions..." << std::endl;
    
    emit_stored_clauses_to_solvers_ ();
    
    bool allok = true;
    
    for (sutype::uvi_t i=0; i < _solvers.size(); ++i) {

      const double cputime0 = suTimeManager::instance()->get_cpu_time();
      
      bool ok = _solvers[i]->solve (assumptions);

      suStatic::increment_elapsed_time (sutype::ts_sat_total_time, suTimeManager::instance()->get_cpu_time() - cputime0);
      
      SUASSERT (i == 0 || ok == allok, "");
      
      if (!ok)
        allok = false;
    }
    
    return allok;
    
  } // end of suSatSolverWrapper::solve_the_problem

  //
  bool suSatSolverWrapper::model_is_valid ()
    const
  {
    SUASSERT (_solvers.size() == 1, "");
    const sutype::uvi_t solverindex = 0; // I'll switch it to a parameter if needed
    
    return _solvers[solverindex]->model_is_valid ();
    
  } // end of suSatSolverWrapper::model_is_valid

  //
  void suSatSolverWrapper::keep_model_unchanged (bool v)
  {
    SUASSERT (_solvers.size() == 1, "");
    const sutype::uvi_t solverindex = 0; // I'll switch it to a parameter if needed

    _solvers[solverindex]->keep_model_unchanged (v);
    
  } // end of suSatSolverWrapper::keep_model_unchanged
  
  //
  sutype::bool_t suSatSolverWrapper::get_modeled_value (sutype::satindex_t satindex,
                                                        sutype::bool_t desiredvalue)
    const
  {
    SUASSERT (_solvers.size() == 1, "");
    const sutype::uvi_t solverindex = 0; // I'll switch it to a parameter if needed

    // it may happen:
    //   when you allocated a satindex
    //   this satindex is not constrained anyhow
    //   no other larger satindices emitted after that
    if (!is_registered_satindex (satindex)) {
      
      if (desiredvalue != sutype::bool_undefined)
        return desiredvalue;

      //SUASSERT (false, "this may be OK but review just in case");
      
      SUISSUE("SAT index was not registered. This may be OK but review just in case. Returned FALSE.")
        << std::endl;

      return sutype::bool_false;
    }
    
    // this flow is correct but it populates a whole model
    if (0) {
      SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");
      
      const sutype::bools_t & model = get_model_ ();
      SUASSERT (!model.empty(), "Model can't be empty");
      
      sutype::satindex_t abssatindex = abs (satindex);
      SUASSERT (abssatindex > 0 && abssatindex < (sutype::satindex_t)model.size(), "");
      
      sutype::bool_t value = model [abssatindex];
      SUASSERT (value == sutype::bool_true || value == sutype::bool_false, "");
      
      return ((satindex > 0) ? value : suStatic::invert_bool_value (value));
    }
    //
    
    // get_modeled_value has no benefits for Fiver; I have to populate a whole model
    return _solvers[solverindex]->get_modeled_value (satindex);
    
  } // end of suSatSolverWrapper::get_modeled_value
  
  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  //!
  sutype::satindex_t suSatSolverWrapper::emit_constraint (sutype::logic_func_t func,
                                                          sutype::bool_t value,
                                                          sutype::satindex_t satindex)
  {
    sutype::clause_t & satindices = suClauseBank::loan_clause ();
    
    satindices.push_back (satindex);
    
    sutype::satindex_t out = emit_constraint (func, value, satindices);

    suClauseBank::return_clause (satindices);

    return out;
    
  } // end of suSatSolverWrapper::emit_constraint
  
  //!
  sutype::satindex_t suSatSolverWrapper::emit_constraint (sutype::logic_func_t func,
                                                          sutype::bool_t value,
                                                          sutype::satindex_t satindex1,
                                                          sutype::satindex_t satindex2)
  {
    sutype::clause_t & satindices = suClauseBank::loan_clause ();
    
    satindices.push_back (satindex1);
    satindices.push_back (satindex2);
    
    sutype::satindex_t out = emit_constraint (func, value, satindices);
    
    suClauseBank::return_clause (satindices);

    return out; 
    
  } // end of suSatSolverWrapper::emit_constraint
  
  //!
  sutype::satindex_t suSatSolverWrapper::emit_constraint (sutype::logic_func_t func,
                                                          sutype::bool_t value,
                                                          const sutype::clause_t & satindices)
  {
    if (func == sutype::logic_func_and) {

      if (value == sutype::bool_true) {
        emit_AND_ALWAYS_ONE (satindices);
        return 0;
      }
      else if (value == sutype::bool_false) {
        emit_AND_ALWAYS_ZERO (satindices);
        return 0;
      }
      else {
        return emit_AND_or_return_constant (satindices);
      }
    }
    else if (func == sutype::logic_func_or) {
      
      if (value == sutype::bool_true) {
        emit_OR_ALWAYS_ONE (satindices);
        return 0;
      }
      else if (value == sutype::bool_false) {
        emit_OR_ALWAYS_ZERO (satindices);
        return 0;
      }
      else {
        return emit_OR_or_return_constant (satindices);
      }
    }
    else {
      SUASSERT (false, "");
    }

    return 0;
    
  } // end of suSatSolverWrapper::emit_constraint
  
  //!
  void suSatSolverWrapper::emit_ALWAYS_ONE (sutype::satindex_t satindex)
  {
    sutype::clause_t & clause = _hashedClauseSize1;
    clause[0] = satindex;
    emit_clause_ (clause);
   
  } // end of suSatSolverWrapper::emit_ALWAYS_ONE

  //
  void suSatSolverWrapper::emit_ALWAYS_ONE (const sutype::clause_t & clause)
  {
    for (const auto & iter : clause) {
      emit_ALWAYS_ONE (iter);
    }
    
  } // end of suSatSolverWrapper::emit_ALWAYS_ONE
  
  //!
  void suSatSolverWrapper::emit_ALWAYS_ZERO (sutype::satindex_t satindex)
  {    
    sutype::clause_t & clause = _hashedClauseSize1;
    clause[0] = -satindex;
    emit_clause_ (clause);
    
  } // end of suSatSolverWrapper::emit_ALWAYS_ZERO

  //
  void suSatSolverWrapper::emit_ALWAYS_ZERO (const sutype::clause_t & clause)
  {
    for (const auto & iter : clause) {
      emit_ALWAYS_ZERO (iter);
    }
    
  } // end of suSatSolverWrapper::emit_ALWAYS_ZERO
  
  //! do not create extra literals; add to CNF directrly a short form
  void suSatSolverWrapper::emit_OR_ALWAYS_ZERO (const sutype::clause_t & satindices)
  {
    SUASSERT (!satindices.empty(), "");
    
    for (sutype::uvi_t i=0; i < satindices.size(); ++i) {
      emit_ALWAYS_ZERO (satindices[i]);
    }
    
  } // end of suSatSolverWrapper::emit_OR_ALWAYS_ZERO
  
  //! do not create extra literals; add to CNF directrly a short form
  void suSatSolverWrapper::emit_OR_ALWAYS_ONE (const sutype::clause_t & satindices)
  {    
    emit_clause_ (satindices);
    
  } // end of suSatSolverWrapper::emit_OR_ALWAYS_ONE

  //! do not create extra literals; add to CNF directrly a short form
  void suSatSolverWrapper::emit_AND_ALWAYS_ZERO (const sutype::clause_t & satindices)
  {    
    sutype::clause_t & clause = suClauseBank::loan_clause ();
    
    for (sutype::uvi_t i=0; i < satindices.size(); ++i) {
      clause.push_back (-satindices[i]);
    }
    
    emit_clause_ (clause);
    
    suClauseBank::return_clause (clause);
    
  } // end of suSatSolverWrapper::emit_AND_ALWAYS_ZERO
  
  // custom procedure to reduce calls of loan_clause, push_back, return_clause
  void suSatSolverWrapper::emit_AND_ALWAYS_ZERO (sutype::satindex_t satindex1,
                                                 sutype::satindex_t satindex2)
  {
    sutype::clause_t & clause = _hashedClauseSize2;
    
    clause[0] = -satindex1;
    clause[1] = -satindex2;
    
    emit_clause_ (clause);
    
  } // end of emit_AND_ALWAYS_ZERO
  
  //! do not create extra literals; add to CNF directrly a short form
  void suSatSolverWrapper::emit_AND_ALWAYS_ONE (const sutype::clause_t & satindices)
  {
    SUASSERT (!satindices.empty(), "");
    
    for (sutype::uvi_t i=0; i < satindices.size(); ++i) {
      emit_ALWAYS_ONE (satindices[i]);
    }
    
  } // end of suSatSolverWrapper::emit_AND_ALWAYS_ONE

  //!
  sutype::satindex_t suSatSolverWrapper::emit_NOT_or_return_constant (sutype::satindex_t satindex)
  {
    SUASSERT (false, "Correct but not used anywhere as useless. I use just sign 'minus'.");
    
    if      (is_constant (1, satindex)) return get_constant(0);
    else if (is_constant (0, satindex)) return get_constant(1);
    
    return -satindex;
    
  } // suSatSolverWrapper::emit_NOT_or_return_constant

  //!
  sutype::satindex_t suSatSolverWrapper::emit_AND_or_return_constant (const sutype::clause_t & satindices,
                                                                      sutype::satindex_t dontCareValue)
  {
    SUASSERT (!satindices.empty(), "");
    SUASSERT (dontCareValue == 0 || is_constant (0, dontCareValue) || is_constant (1, dontCareValue), "");

    if (satindices.size() == 1) return satindices.front(); //bebe
    
    // check for constant zero
    bool allSatIndicesAreConstants = true;

    for (sutype::uvi_t i=0; i < satindices.size(); ++i) {
      
      sutype::satindex_t satindex = satindices[i];
      
      if (is_constant (1, satindex))
        continue;
      
      if (is_constant (0, satindex))
        return get_constant(0);
      
      allSatIndicesAreConstants = false;
    }
    
    if (allSatIndicesAreConstants)
      return get_constant(1);
    
    sutype::satindex_t out = get_next_sat_index ();
    
    emit_AND_ (satindices, out, dontCareValue);
    
    return out;
    
  } // end of suSatSolverWrapper::emit_AND_or_return_constant

  //!
  sutype::satindex_t suSatSolverWrapper::emit_OR_or_return_constant (const sutype::clause_t & satindices,
                                                                      sutype::satindex_t dontCareValue)
  {
    SUASSERT (!satindices.empty(), "");
    SUASSERT (dontCareValue == 0 || is_constant (0, dontCareValue) || is_constant (1, dontCareValue), "");

    if (satindices.size() == 1) return satindices.front(); //bebe
    
    // check for constant zero
    bool allSatIndicesAreConstants = true;

    for (sutype::uvi_t i=0; i < satindices.size(); ++i) {
      
      sutype::satindex_t satindex = satindices[i];
      
      if (is_constant (0, satindex))
        continue;
      
      if (is_constant (1, satindex))
        return get_constant(1);
      
      allSatIndicesAreConstants = false;
    }
    
    if (allSatIndicesAreConstants)
      return get_constant(0);
    
    sutype::satindex_t out = get_next_sat_index ();
    
    emit_OR_ (satindices, out, dontCareValue);
    
    return out;
    
  } // end of suSatSolverWrapper::emit_OR_or_return_constant

  //
  sutype::satindex_t suSatSolverWrapper::emit_EQUAL_TO (const sutype::clause_t & clause,
                                                        int number)
  {
    SUASSERT (!clause.empty(), "");
    SUASSERT (number >= 0 && number <= (int)clause.size(), "");

    sutype::clause_t & clause1 = suClauseBank::loan_clause ();
    
    clause1.push_back (emit_GREATER_or_EQUAL_THEN (clause, number));
    clause1.push_back (emit_LESS_or_EQUAL_THEN    (clause, number));

    sutype::satindex_t out = emit_AND_or_return_constant (clause1);

    suClauseBank::return_clause (clause1);

    return out;
    
  } // end of suSatSolverWrapper::emit_EQUAL_TO

  //
  sutype::satindex_t suSatSolverWrapper::emit_GREATER_or_EQUAL_THEN (const sutype::clause_t & clause,
                                                                     int number)
  {
    SUASSERT (!clause.empty(), "");
    SUASSERT (number >= 0, "");
    
    if (number == 0) {
      return get_constant(1);
    }
    else if (number == 1) {
      return emit_OR_or_return_constant (clause);
    }
    else if (number == (int)clause.size()) {
      return emit_AND_or_return_constant (clause);
    }
    else if (number > (int)clause.size()) {
      return get_constant(0);
    }

    sutype::clause_t & counter = suSatSolverWrapper::instance()->build_assumption_min_number_of_outs_used_from_the_clause (clause, number, number);
    SUASSERT (counter.size() == clause.size() + 1, "");
    SUASSERT (number >= 0 && number < (int)counter.size(), "");
    
    sutype::satindex_t out = counter[number];
    SUASSERT (out != sutype::UNDEFINED_SAT_INDEX, "");

    suClauseBank::return_clause (counter);

    return out;
    
  } // end of suSatSolverWrapper::emit_GREATER_or_EQUAL_THEN

  //
  sutype::satindex_t suSatSolverWrapper::emit_LESS_or_EQUAL_THEN (const sutype::clause_t & clause,
                                                                  int number)
  {
    SUASSERT (!clause.empty(), "");
    SUASSERT (number >= 0, "");

    if (number >= (int)clause.size()) {
      return get_constant(1);
    }
    else if (number == (int)clause.size() - 1) {
      return - (emit_AND_or_return_constant (clause));
    }
    else if (number == 0) {
      return - (emit_OR_or_return_constant (clause));
    }
    else if (number < 0) {
      return get_constant(0);
    }

    sutype::clause_t & counter = suSatSolverWrapper::instance()->build_assumption_max_number_of_outs_used_from_the_clause (clause, number, number);
    SUASSERT (counter.size() == clause.size() + 1, "");
    SUASSERT (number >= 0 && number < (int)counter.size(), "");
    
    sutype::satindex_t out = counter[number];
    SUASSERT (out != sutype::UNDEFINED_SAT_INDEX, "");

    suClauseBank::return_clause (counter);
    
    return out;
    
  } // end of suSatSolverWrapper::emit_LESS_or_EQUAL_THEN
  
  // a position P of out in outs meants: number of true outs from the list <= P;
  // if minnumber == 0 return a vector of clauses: <= 0, <= 1, <= 2, <= 3, ... <= maxnumber, '0', '0', '0', '0' --> totally clause.size()+1
  // return a vector '0', '0', '0', <= minnumber, ..., <= maxnumber, '0', '0', '0', '0' --> totally clause.size()+1
  //
  // position 0 means that number of trues in the clause <= 0
  // position 1 means that number of trues in the clause <= 1
  // position 2 means that number of trues in the clause <= 2
  // etc
  sutype::clause_t & suSatSolverWrapper::build_assumption_max_number_of_outs_used_from_the_clause (const sutype::clause_t & clause,
                                                                                                   int minnumber,
                                                                                                   int maxnumber)
  {
    SUASSERT (!clause.empty(), "");
    SUASSERT (maxnumber >= 0 && maxnumber <= (int)clause.size(), "");
    SUASSERT (minnumber >= 0 && minnumber <= maxnumber, "");

    if (clause.size() == 1) {

      sutype::clause_t & outClause = suClauseBank::loan_clause (); // will be released outside this procedure
      
      if (minnumber == 0) {
        outClause.push_back (- (emit_OR_or_return_constant (clause))); // <= 0
      }
      else {
        outClause.push_back (0);
      }
      
      if (maxnumber == 1) {
        outClause.push_back (get_constant(1)); // always <= 1
      }
      else {
        outClause.push_back (0);
      }
      
      return outClause;
    }
    
    // invert the task
    sutype::clause_t & invertedclause = suClauseBank::loan_clause ();
    for (sutype::uvi_t i=0; i < clause.size(); ++i) {
      invertedclause.push_back (-clause[i]);
    }
    
    const int minChecksum = (int)clause.size() - maxnumber;
    const int maxCheckSum = (int)clause.size() - minnumber;
    
    sutype::clause_t & invertedcounter = build_adder_ (invertedclause, minChecksum, maxCheckSum);
    SUASSERT (invertedcounter.size() == clause.size()+1, "");
    suClauseBank::return_clause (invertedclause);

    sutype::clause_t & directcounter = suClauseBank::loan_clause (); // will be returned outside this procedure
    SUASSERT (directcounter.size() == 0, "");
    
    for (int i = (int)invertedcounter.size()-1; i >= 0; --i) {
      directcounter.push_back (invertedcounter[i]);
    }
    suClauseBank::return_clause (invertedcounter);
    SUASSERT (directcounter.size() == clause.size()+1, "");
    
    for (sutype::uvi_t i=0; i < directcounter.size(); ++i) {
      int out = directcounter[i];
      if ((int)i >= minnumber && (int)i <= maxnumber) {
        SUASSERT (out != 0, "minnumber=" << minnumber << " maxnumber=" << maxnumber << " i=" << i << " out=" << out);
      }
      else {
        SUASSERT (out == 0, "minnumber=" << minnumber << " maxnumber=" << maxnumber << " i=" << i << " out=" << out);
      }
    }
        
    return directcounter;
    
  } // end of suSatSolverWrapper::build_assumption_max_number_of_outs_used_from_the_clause

  //!
  // position 0 means that number of trues in the clause == 0
  // position 1 means that number of trues in the clause >= 1
  // position 2 means that number of trues in the clause >= 2
  // etc
  sutype::clause_t & suSatSolverWrapper::build_assumption_min_number_of_outs_used_from_the_clause (const sutype::clause_t & clause,
                                                                                                   int minnumber,
                                                                                                   int maxnumber)
  {
    SUASSERT (!clause.empty(), "");
    SUASSERT (maxnumber >= 0 && maxnumber <= (int)clause.size(), "");
    SUASSERT (minnumber >= 0 && minnumber <= maxnumber, "");
    
    return build_adder_ (clause, minnumber, maxnumber);
    
  } // end of suSatSolverWrapper::build_assumption_min_number_of_outs_used_from_the_clause

  //
  void suSatSolverWrapper::optimize_satindices (const sutype::clause_t & clause,
                                                const sutype::opt_mode_t optmode,
                                                int minbound)
  {
    const double cputime0 = suTimeManager::instance()->get_cpu_time();

    SUASSERT (optmode == sutype::om_minimize || optmode == sutype::om_maximize, "");
    //SUASSERT (model_is_valid(), "");
    
    if (clause.empty()) return;
    
    const sutype::opt_mode_t originaloptmodeToReport = optmode;
    
    if (optmode == sutype::om_maximize) {

      SUASSERT (minbound == -1, "Other values are not supported yet.");

      sutype::clause_t & invertedclause = suClauseBank::loan_clause();

      for (const auto & iter : clause) {
        sutype::satindex_t satindex = iter;
        SUASSERT (satindex != sutype::UNDEFINED_SAT_INDEX, "");
        invertedclause.push_back (-satindex);
      }

      minimize_satindices_ (invertedclause, originaloptmodeToReport);

      suClauseBank::return_clause (invertedclause);

      return;
    }

    else {

      SUASSERT (minbound == -1 || minbound > 0, "");
      
      minimize_satindices_ (clause, originaloptmodeToReport, minbound);
    }

    SUASSERT (model_is_valid(), "");

    const double cputime1 = suTimeManager::instance()->get_cpu_time();
    const double numsec = (cputime1 - cputime0);
    
    SUINFO(1) << "Optimize satindices elapsed " << (int)numsec << " sec." << std::endl;
    
  } // end of suSatSolverWrapper::optimize_satindices

  //
  void suSatSolverWrapper::minimize_satindices_ (const sutype::clause_t & clause,
                                                 const sutype::opt_mode_t originaloptmodeToReport,
                                                 int minbound)
                                                 
  {
    SUASSERT (!clause.empty(), "");
    SUASSERT (originaloptmodeToReport == sutype::om_minimize || originaloptmodeToReport == sutype::om_maximize, ""); 
    
    int minnumber = 0;
    int maxnumber = clause.size();

    bool getUpperEstimation = true;
    bool useOnlyFastMode = false;
    
    // getting upper bound may be much slower than the unlimited bisection/iteration
    // heavily depends on the nature of a task
    // do not estimate upper bound for a while if we want to maximize something
    if (originaloptmodeToReport == sutype::om_maximize) {
      getUpperEstimation = true;
      useOnlyFastMode    = true;
    }
    
    // get upper bound estimation
    // if fastmode=false then fix absent as absent; try to minimize present to get the upper bound estimation
    for (int mode = 0; mode <= 1; ++mode) {

      bool fastmode = (mode == 0) ? true : false;
      if (useOnlyFastMode && !fastmode) continue;
      
      if (!getUpperEstimation) {
        SUINFO(1) << "Do not estimate the upper bound for this case." << std::endl;
        break;
      }
        
      int localmaxnumber = estimate_the_number_of_mandatory_sat_indices_ (clause, sutype::om_minimize, fastmode);
      SUASSERT (localmaxnumber <= maxnumber, "");

//       if (localmaxnumber == maxnumber) {
//         SUINFO(1) << "Estimation in " << (fastmode ? "fast" : "slow") << " mode was useless." << std::endl;
//       }
//       else {
//         SUINFO(1) << "Estimation in " << (fastmode ? "fast" : "slow") << " mode was usefull." << std::endl;
//       }
      
      maxnumber = localmaxnumber;
      
      // report
      if (originaloptmodeToReport == sutype::om_minimize) {
        SUINFO(1) << "Minimize " << clause.size() << " sat indices (" << (fastmode ? "fast" : "slow") << "); estimated bounds: total=" << clause.size() << "; min=" << minnumber << "; max=" << maxnumber << std::endl;
      }
      else {
        SUINFO(1) << "Maximize " << clause.size() << " sat indices (" << (fastmode ? "fast" : "slow") << "); estimated bounds: total=" << clause.size() << "; min=" << (clause.size() - maxnumber) << "; max=" << (clause.size() - minnumber) << std::endl;
      }
    }

    // relax to allow minbound of satindices
    if (minbound > 0) {
      maxnumber = std::max (maxnumber, minbound);
      //minnumber = std::min (minnumber, minbound);
    }
    
    if (maxnumber == 0) {
      
      if (originaloptmodeToReport == sutype::om_minimize) {
        SUINFO(1) << "Solved: == 0 of " << clause.size() << std::endl;
      }
      else {
        SUINFO(1) << "Solved: == " << clause.size() << " of " << clause.size() << std::endl;
      }

      SUINFO(1) << "Emitted " << clause.size() << " constants." << std::endl;

      // this emit doesn't invalidate/change the model; I don't need to re-solve
      bool keepTheModel = can_keep_model_unchanged_ (clause, sutype::bool_false);

      if (keepTheModel) {
        keep_model_unchanged (true);
      }
      
      emit_OR_ALWAYS_ZERO (clause);
      
      if (keepTheModel) {
        keep_model_unchanged (false); // just restore the flag
      }
      else {
        const double cputime0 = suTimeManager::instance()->get_cpu_time();
        bool ok = solve_the_problem (); // primitive solving
        suStatic::increment_elapsed_time (sutype::ts_sat_other_time, suTimeManager::instance()->get_cpu_time() - cputime0);
        SUASSERT (ok, "");
      }

      return;
    }

    if (minnumber == (int)clause.size()) {

      SUASSERT (false, "Can't be here");

      if (originaloptmodeToReport == sutype::om_minimize) {
        SUINFO(1) << "Solved: == " << clause.size() << " of " << clause.size() << std::endl;
      }
      else {
        SUINFO(1) << "Solved: == 0 of " << clause.size() << std::endl;
      }

      SUINFO(1) << "Emitted " << clause.size() << " constants." << std::endl;

      // this emit doesn't invalidate/change the model; I don't need to re-solve
      bool keepTheModel = can_keep_model_unchanged_ (clause, sutype::bool_true);
      
      if (keepTheModel) {
        keep_model_unchanged (true);
      }
      
      emit_AND_ALWAYS_ONE (clause);
      
      if (keepTheModel) {
        keep_model_unchanged (false); // just restore the flag
      }
      else {
        const double cputime0 = suTimeManager::instance()->get_cpu_time();
        bool ok = solve_the_problem (); // primitive solving
        suStatic::increment_elapsed_time (sutype::ts_sat_other_time, suTimeManager::instance()->get_cpu_time() - cputime0);
        SUASSERT (ok, "");
      }
      
      return;
    }

    // relax to allow minbound of satindices
    if (minbound > 0) {
      //maxnumber = std::max (maxnumber, minbound);
      minnumber = std::max (minnumber, minbound);
    }
    
    sutype::clause_t & assumptions = suClauseBank::loan_clause ();
    
    sutype::satindex_t latestSatisfiable = sutype::UNDEFINED_SAT_INDEX;
    sutype::satindex_t latestTried       = sutype::UNDEFINED_SAT_INDEX;

    unsigned numiters = 0;

    while (1) {

      ++numiters;

      int number = 0;
      
      // try most simple and frequent cases
      if        (numiters == 1) { number = minnumber;
      } else if (numiters == 2) { number = maxnumber-1;
      }

      if (number < minnumber || number > maxnumber) {
        number = (maxnumber + minnumber) / 2;
      }
            
      SUASSERT (number >= 0 && number <= (int)clause.size(), "");
      
      sutype::clause_t & counter = build_assumption_max_number_of_outs_used_from_the_clause (clause, number, number);
      SUASSERT (counter.size() == clause.size()+1, "");
      sutype::satindex_t out = counter[number];
      suClauseBank::return_clause (counter);
      
      assumptions.push_back (out);

      latestTried = out;

      if (1) {
        if (originaloptmodeToReport == sutype::om_minimize) {
          SUINFO(1) << "-try-:  <= " << number << " of " << clause.size() << std::endl;
        }
        else {
          SUINFO(1) << "-try-:  >= " << (clause.size() - number) << " of " << clause.size() << std::endl;
        }
      }

      const double cputime0 = suTimeManager::instance()->get_cpu_time();
      bool ok = solve_the_problem (assumptions);
      suStatic::increment_elapsed_time (sutype::ts_sat_assumptions, suTimeManager::instance()->get_cpu_time() - cputime0);
      
      assumptions.clear();
            
      if (!ok) {

        if (originaloptmodeToReport == sutype::om_minimize) {
          SUINFO(1) << "UNSAT:  <= " << number << " of " << clause.size() << std::endl;
        }
        else {
          SUINFO(1) << "UNSAT:  >= " << (clause.size() - number) << " of " << clause.size() << std::endl;
        }

        if (number == (int)clause.size() - 1) {
          for (const auto & iter : clause) {
            sutype::satindex_t satindex = iter;
            emit_ALWAYS_ONE (satindex);
          }
          SUINFO(1) << "Detected and emitted " << clause.size() << " constants." << std::endl;
        }

        SUASSERT (number >= minnumber && number <= maxnumber, "");
        
        minnumber = number + 1;
        if (minnumber > maxnumber) break;
      }
      else {

        if (originaloptmodeToReport == sutype::om_minimize) {
          SUINFO(1) << "Solved: <= " << number << " of " << clause.size() << std::endl;
        }
        else {
          SUINFO(1) << "Solved: >= " << (clause.size() - number) << " of " << clause.size() << std::endl;
        }

        int actualnumber = 0;
        for (const auto & iter : clause) {
          sutype::satindex_t satindex = iter;
          sutype::bool_t value = get_modeled_value (satindex);
          if (value == sutype::bool_true)
            ++actualnumber;
        }
        
        actualnumber = std::max (actualnumber, minnumber); // minnumber may be intetionally increased by minbound
        
        SUASSERT (actualnumber <= number, "actualnumber=" << actualnumber << "; number=" << number);

        // utilize this actual number for free
        if (actualnumber != number) {

          SUINFO(1) << "Found better solution for free; actualnumber: " << actualnumber << "; delta: " << (number - actualnumber) << std::endl;
          
          // override
          number = actualnumber;
          
          sutype::clause_t & counter2 = build_assumption_max_number_of_outs_used_from_the_clause (clause, number, number);
          SUASSERT (counter2.size() == clause.size()+1, "");
          out = counter2 [number]; // override
          suClauseBank::return_clause (counter2);
          
          SUASSERT (assumptions.empty(), "");
          assumptions.push_back (out);
          latestTried = out;
          
          const double cputime2 = suTimeManager::instance()->get_cpu_time();
          ok = solve_the_problem (assumptions); // primitive solving but I created new literals and clauses during build_assumption_max_number_of_outs_used_from_the_clause
          suStatic::increment_elapsed_time (sutype::ts_sat_other_time, suTimeManager::instance()->get_cpu_time() - cputime2);
          SUASSERT (ok, "");
          
          assumptions.clear();
          
          if (originaloptmodeToReport == sutype::om_minimize) {
            SUINFO(1) << "Solved: <= " << number << " of " << clause.size() << std::endl;
          }
          else {
            SUINFO(1) << "Solved: >= " << (clause.size() - number) << " of " << clause.size() << std::endl;
          }
        }
        //

        latestSatisfiable = out;
        
        if (number == 0 && !clause.empty()) {
          
          SUASSERT (model_is_valid(), "");
          
          keep_model_unchanged (true);
          
          emit_OR_ALWAYS_ZERO (clause);
          
          keep_model_unchanged (false); // just restore the flag
          
          SUINFO(1) << "Detected and emitted " << clause.size() << " constants." << std::endl;
        }

        SUASSERT (number >= minnumber && number <= maxnumber, "number=" << number << "; minnumber=" << minnumber << "; maxnumber=" << maxnumber);
        
        if (number == minnumber) break;
        maxnumber = number - 1;
      }
    }

    suClauseBank::return_clause (assumptions);

    SUASSERT (latestSatisfiable != sutype::UNDEFINED_SAT_INDEX, "");

    bool keepTheModel = ((latestSatisfiable == latestTried) && can_keep_model_unchanged_ (latestSatisfiable, sutype::bool_true));
    
    if (keepTheModel) {
      keep_model_unchanged (true);
    }
    
    emit_ALWAYS_ONE (latestSatisfiable);
    
    if (keepTheModel) {
      keep_model_unchanged (false); // just restore the flag
    }
    else {
      const double cputime0 = suTimeManager::instance()->get_cpu_time();
      bool ok = solve_the_problem (); // primitive solving
      suStatic::increment_elapsed_time (sutype::ts_sat_other_time, suTimeManager::instance()->get_cpu_time() - cputime0);
      SUASSERT (ok, "");
    }
    
  } // end of suSatSolverWrapper::minimize_satindices_

  //
  bool suSatSolverWrapper::can_keep_model_unchanged_ (sutype::satindex_t satindex,
                                                      sutype::bool_t targetvalulue)
    const
  {
    const bool np = false;

    if (!model_is_valid()) {
      SUINFO(np) << "1) Model is not valid" << std::endl;
      return false;
    }

    if (!is_registered_satindex (satindex)) {
      SUINFO(np) << "2) Satindex was not registered yet: satindex=" << satindex << std::endl;
      return false;
    }

    if (get_modeled_value (satindex) != targetvalulue) {
      SUINFO(np) << "3) Satindex has another value: satindex=" << satindex << "; targetvalulue=" << suStatic::bool_2_str (targetvalulue) << std::endl;
      return false;
    }

    return true;
    
  } // end of suSatSolverWrapper::can_keep_model_unchanged_

  //
  bool suSatSolverWrapper::can_keep_model_unchanged_ (const sutype::clause_t & clause,
                                                      sutype::bool_t targetvalulue)
    const
  {
    const bool np = false;
    
    if (!model_is_valid()) {
      SUINFO(np) << "1) Model is not valid" << std::endl;
      return false;
    }

    for (const auto & iter : clause) {

      sutype::satindex_t satindex = iter;

      if (!is_registered_satindex (satindex)) {
        SUINFO(np) << "2) Satindex was not registered yet: satindex=" << satindex << std::endl;
        return false;
      }

      if (get_modeled_value (satindex) != targetvalulue) {
        SUINFO(np) << "3) Satindex has another value: satindex=" << satindex << "; targetvalulue=" << suStatic::bool_2_str (targetvalulue) << std::endl;
        return false;
      }
    }
    
    return true;
    
  } // end of suSatSolverWrapper::can_keep_model_unchanged_

  // sign is important
  bool suSatSolverWrapper::satindex_is_used_in_clauses (sutype::satindex_t satindex)
    const
  {
    SUASSERT (_clauseManager, "Clause mamager must be created to use this debug procedure");
    
    return _clauseManager->satindex_has_stored_clauses (satindex);
    
  } // end of suSatSolverWrapper::satindex_is_used_in_clauses

  //
  void suSatSolverWrapper::print_statistic_of_emitted_clauses ()
    const
  {
    SUINFO(1) << "Statistic of emitted clauses:" << std::endl;

    for (const auto & iter : _statisticOfEmittedClauses) {

      unsigned size    = iter.first;
      unsigned counter = iter.second;
      
      SUINFO(1) << "  " << size << ": " << counter << std::endl;
    }
    
  } // end of suSatSolverWrapper::print_statistic_of_emitted_clauses
  
  //
  void suSatSolverWrapper::read_and_emit_dimacs_file (const std::string & filename)
  {
    SUINFO(1) << "Read dimacs file " << filename << std::endl;

    std::ifstream ifs (filename);
    
    if (!ifs.is_open()) {
      SUASSERT (false, "Can't open file for reading: " << filename);
      return;
    }

    sutype::satindex_t maxsatindex = 0;
    sutype::uvi_t numClauses = 0;

    sutype::clause_t & clause = suClauseBank::loan_clause ();

    while (true) {

      std::string str;
      bool ok = std::getline (ifs, str);
      if (!ok) break;

      str = suStatic::trim_string (str);
      if (str.empty()) continue;

      sutype::strings_t strs = suStatic::parse_string (str);
      SUASSERT (!strs.empty(), "");

      if (strs.front() == "c") continue; // skip comment
      
      if (strs.front() == "p") {
        SUINFO(1) << "Header: " << str << std::endl;
        continue; // skip header
      }
      
      clause.clear();

      for (sutype::uvi_t i=0; i < strs.size(); ++i) {
        
        const std::string & str2 = strs[i];
        sutype::satindex_t satindex = std::stoi (str2);
        
        if (satindex == 0) {
          SUASSERT (i+1 == strs.size(), "");
          break;
        }
        else {
          SUASSERT (i+1 < strs.size(), "");
        }
        
        clause.push_back (satindex);
        
        sutype::satindex_t abssatindex = abs (satindex);
        maxsatindex = std::max (maxsatindex, abssatindex);
      }

      SUASSERT (!clause.empty(), "");

      ++numClauses;

      //SUINFO(1) << numClauses << ": " << suStatic::clause_to_str (clause) << std::endl;
      
      emit_OR_ALWAYS_ONE (clause);
      
      //bool solved = solve_the_problem ();
      //SUASSERT (solved, "");
    }
    
    ifs.close();

    suClauseBank::return_clause (clause);

    SUINFO(1) << "Done reading " << filename << std::endl;
    SUINFO(1) << "Found " << maxsatindex << " literals." << std::endl;
    SUINFO(1) << "Found " << numClauses << " clauses." << std::endl;
    
  } // end of suSatSolverWrapper::read_and_emit_dimacs_file

  //
  void suSatSolverWrapper::fix_sat_indices (const sutype::clause_t & clause)
  {
    if (clause.empty()) return; 
    
    sutype::bools_t values (clause.size(), sutype::bool_false);
    values.clear();
    
    for (const auto & iter : clause) {
      
      sutype::satindex_t satindex = iter;
      
      sutype::bool_t value = get_modeled_value (satindex);

      values.push_back (value);
    }
    
    for (sutype::uvi_t i=0; i < clause.size(); ++i) {
      
      sutype::satindex_t satindex = clause[i];
      sutype::bool_t     value    = values[i];
      
      if        (value == sutype::bool_false) { emit_ALWAYS_ZERO (satindex);
      } else if (value == sutype::bool_true)  { emit_ALWAYS_ONE  (satindex);
      }
      else {
        SUASSERT (false, "");
      }
    }
    
  } // end of suSatSolverWrapper::fix_sat_indices
  
  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------

  //! can return only one model at time
  const sutype::bools_t & suSatSolverWrapper::get_model_ ()
    const
  {
    SUASSERT (!_solvers.empty(), "");
    SUASSERT (_solvers.size() == 1, "");

    const sutype::uvi_t solverindex = 0; // I'll switch it to a parameter if needed
    
    SUASSERT (solverindex >= 0 && solverindex < (sutype::uvi_t)_solvers.size(), "");
    
    const sutype::bools_t & model = _solvers[solverindex]->get_model();
    
    // maybe empty if UNSAT
    if (model.empty()) {
      return model;
    }
    
    // constants may be zero if instance was created without pre-emitted constants
    if (_constant[0] != 0 && _constant[1] != 0) {
      SUASSERT (_constant[0] == 1, "");
      SUASSERT (_constant[1] == 2, "");
      SUASSERT (model.size() >= 3, "dymmy, constant-0, constant-1: " << model.size());
      SUASSERT (model[1] == sutype::bool_false, "expected constant-0");
      SUASSERT (model[2] == sutype::bool_true, "expected constant-1");      
    }
    
    return model;
        
  } // end of suSatSolverWrapper::get_model_

  //!
  void suSatSolverWrapper::emit_AND_ (const sutype::clause_t & satindices,
                                      sutype::satindex_t out,
                                      sutype::satindex_t dontCareValue)
  {
    SUASSERT (dontCareValue == 0 || is_constant (0, dontCareValue) || is_constant (1, dontCareValue), "");
    SUASSERT (!satindices.empty(), "");
    SUASSERT (out > 0, "");
    
    sutype::clause_t & clause = suClauseBank::loan_clause ();
    
    // emit long clause
    if (!is_constant (1, dontCareValue)) {
            
      clause.push_back (out);
      
      for (sutype::uvi_t i=0; i < satindices.size(); ++i) {
        clause.push_back (-satindices[i]);
      }
      
      emit_clause_ (clause);
    }
    
    // emit pairs
    if (!is_constant (0, dontCareValue)) {;
      
      clause.resize (2);
      clause[0] = -out;
      
      for (sutype::uvi_t i=0; i < satindices.size(); ++i) {
        
        clause[1] = satindices[i];
        emit_clause_ (clause);
      }
    }
    
    suClauseBank::return_clause (clause);
    
  } // end of suSatSolverWrapper::emit_AND_

  //!
  void suSatSolverWrapper::emit_OR_ (const sutype::clause_t & satindices,
                                      sutype::satindex_t out,
                                      sutype::satindex_t dontCareValue)
  {
    SUASSERT (dontCareValue == 0 || is_constant (0, dontCareValue) || is_constant (1, dontCareValue), "");
    SUASSERT (!satindices.empty(), "");
    SUASSERT (out > 0, "");

    sutype::clause_t & clause = suClauseBank::loan_clause ();

    // emit long clause
    if (!is_constant (0, dontCareValue)) {
            
      clause.push_back (-out);
      
      for (sutype::uvi_t i=0; i < satindices.size(); ++i) {
        clause.push_back (satindices[i]);
      }
      
      emit_clause_ (clause);
    }

    // emit pairs
    if (!is_constant (1, dontCareValue)) {
      
      clause.resize (2);
      clause[0] = out;
      
      for (sutype::uvi_t i=0; i < satindices.size(); ++i) {
                
        clause[1] = -satindices[i];
        emit_clause_ (clause);
      }
    }
    
    suClauseBank::return_clause (clause);
    
  } // end of suSatSolverWrapper::emit_OR_
  
  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------

  // mimics emit_always_zero
  void suSatSolverWrapper::emit_constant_zero_ (sutype::satindex_t satindex)
  {
    SUASSERT (satindex > 0, "");
    SUASSERT (_constant[0] == 0, "Constant-zero is not expected to be set yet.");
    
    sutype::clause_t & clause = _hashedClauseSize1;
    clause[0] = -satindex;
    emit_clause_ (clause);
    
  } // end of suSatSolverWrapper::emit_constant_zero_
  
  // mimics emit_always_one
  void suSatSolverWrapper::emit_constant_one_ (sutype::satindex_t satindex)
  {
    SUASSERT (satindex > 0, "");
    SUASSERT (_constant[1] == 0, "Constant-one is not expected to be set yet.");
    
    sutype::clause_t & clause = _hashedClauseSize1;
    clause[0] = satindex;
    emit_clause_ (clause);
    
  } // end of suSatSolverWrapper::emit_constant_one_
    
  //
  void suSatSolverWrapper::init_constants_ ()
  {
    // emit first of all two fixed constants
    const sutype::satindex_t const0 = get_next_sat_index ();
    const sutype::satindex_t const1 = get_next_sat_index ();
    
    SUASSERT (const0 == 1, "");
    SUASSERT (const1 == 2, "");
    
    emit_constant_zero_ (const0);
    emit_constant_one_  (const1);
    
    _constant[0] = const0;
    _constant[1] = const1;
    
  } // end of suSatSolverWrapper::init_constants_

  //! \return a vector of clauses: >= 0, >= 1, >= 2, ... == clause.size() --> totally clause.size()+1 outs
  //! == 0 has own separate implementation. all of inputs must be zero.
  sutype::clause_t & suSatSolverWrapper::build_adder_ (const sutype::clause_t & clause,
                                                       const int minCheckSum,
                                                       const int maxCheckSum)
  {
    SUASSERT (!clause.empty(), "");
    SUASSERT (minCheckSum >= 0 && minCheckSum <= (int)clause.size(), "");
    SUASSERT (maxCheckSum >= 0 && maxCheckSum <= (int)clause.size(), "");
    SUASSERT (maxCheckSum <= maxCheckSum, "");
    
    const int dontCareValue1 = get_constant (1);
    
    sutype::clause_t & outClause = suClauseBank::loan_clause();
    
    if (minCheckSum == 0) {
      outClause.push_back (get_constant(1));
    }
    else {
      outClause.push_back (0);
    }
    
    if (maxCheckSum == 0) {
      
      // append undefined value
      for (sutype::uvi_t i = outClause.size(); i <= clause.size(); ++i)
        outClause.push_back (0);
      
      return outClause;
    }
    
    // {x0, x1, ..., xN-1} >= 1
    if (maxCheckSum == 1) {

      outClause.push_back (emit_OR_or_return_constant (clause));
      
      // append undefined value
      for (sutype::uvi_t i = outClause.size(); i <= clause.size(); ++i)
        outClause.push_back (0);
      
      return outClause;
    }

    SUASSERT (clause.size() >= 2, "");
    
    sutype::clause_t & prevouts = suClauseBank::loan_clause ();
    prevouts.push_back (clause[0]);
    
    const int minHorLevel = (minCheckSum == 0) ? 1 : minCheckSum;
    
    for (sutype::uvi_t varindex=1; varindex < clause.size(); ++varindex) {

      int lastandout = clause [varindex];
      sutype::clause_t & nextouts = suClauseBank::loan_clause ();
      
      for (int horlevel=1; horlevel <= (int)prevouts.size(); ++horlevel) {
        
        int prevout = prevouts [horlevel-1];
        bool needOrGate = false;
        bool needAndGate = false;
        const int numVars = clause.size();
        
        if (horlevel >= minHorLevel - (numVars - 1 - (int)varindex)) needOrGate = true;
        if (horlevel >= minHorLevel - (numVars - 1 - (int)varindex) -1) needAndGate = true;
                
        // OR
        if (!needOrGate) {
          nextouts.push_back (0);
        }
        else {
          SUASSERT (prevout != 0, "");
          SUASSERT (lastandout != 0, "");
          
          sutype::clause_t & orclause = suClauseBank::loan_clause ();
          
          orclause.push_back (prevout);
          
          if (lastandout != prevout)
            orclause.push_back (lastandout);
          
          int orout = emit_OR_or_return_constant (orclause, dontCareValue1);
          suClauseBank::return_clause (orclause);
          
          nextouts.push_back (orout);
        }

        // time to abort; do not need other upper gates
        if ((int)nextouts.size() == maxCheckSum) {
          break;
        }
        
        // AND        
        if (!needAndGate) {
          lastandout = 0;
        }
        else {
          
          sutype::clause_t & andclause = suClauseBank::loan_clause ();
          
          andclause.push_back (prevout);
          
          if (clause[varindex] != prevout)
            andclause.push_back (clause[varindex]);
          
          int andout = emit_AND_or_return_constant (andclause, dontCareValue1);
          suClauseBank::return_clause (andclause);

          lastandout = andout;
        }
      }
      
      if ((int)nextouts.size() < maxCheckSum)
        nextouts.push_back (lastandout);
      
      //prevouts = nextouts;

      prevouts.clear ();
      prevouts.resize (nextouts.size());
      for (sutype::uvi_t pp=0; pp < nextouts.size(); ++pp) {
        prevouts[pp] = nextouts[pp];
      }

      suClauseBank::return_clause (nextouts);
    }
    
    // std::vector::insert causes a very strange memory error in Vtune inspector: "Invalid partial memory access"
    // maybe it happens when outClause is resizes several times when std::vector::insert inserts too many new elements
    //outClause.insert (outClause.end(), prevouts.begin(), prevouts.end());
    
    const sutype::uvi_t prevsize = outClause.size();
    outClause.resize (prevsize + prevouts.size()); // do a single resize
    for (sutype::uvi_t pp=0; pp < prevouts.size(); ++pp) {
      outClause [prevsize + pp] = prevouts [pp];
    }

    suClauseBank::return_clause (prevouts);
    
    // append undefined value
    for (sutype::uvi_t i = outClause.size(); i < clause.size()+1; ++i)
      outClause.push_back (0);
    
    SUASSERT (outClause.size() == clause.size()+1, "");
    
    return outClause;
    
  } // end of suSatSolverWrapper::build_adder_

  // optmode = sutype::om_minimize --> fix absent as absent; try to minimize present
  // optmode = sutype::om_maximize --> fix present as present; try to maximize absent
  // // if not fastmode then run a bisection to mimize the number of present satindices
  int suSatSolverWrapper::estimate_the_number_of_mandatory_sat_indices_ (const sutype::clause_t & clause,
                                                                         const sutype::opt_mode_t optmode,
                                                                         bool fastmode)
  {
    SUINFO(1) 
      << "Estimate the number of mandatory sat indices"
      << ": clause=" << clause.size()
      << "; optmode=" << suStatic::opt_mode_2_str (optmode)
      << "; fastmode=" << fastmode
      << std::endl;

    SUASSERT (optmode == sutype::om_minimize || optmode == sutype::om_maximize, "");

    bool ok = true;

    if (!model_is_valid()) {
      const double cputime0 = suTimeManager::instance()->get_cpu_time();
      ok = solve_the_problem (); // primitive solving
      suStatic::increment_elapsed_time (sutype::ts_sat_other_time, suTimeManager::instance()->get_cpu_time() - cputime0);
    }
    SUASSERT (ok, "Could not solve the problem before applying any extra constraints");
    
    int maxnumber = 0;
    
    if (clause.empty()) return maxnumber;

    sutype::clause_t & assumptions         = suClauseBank::loan_clause ();
    sutype::clause_t & undesiredsatindices = suClauseBank::loan_clause ();

    sutype::bool_t desiredvalue = (optmode == sutype::om_minimize) ? sutype::bool_false : sutype::bool_true;
    
    for (sutype::uvi_t i=0; i < clause.size(); ++i) {
      
      sutype::satindex_t satindex = clause[i];
   
      sutype::bool_t value = get_modeled_value (satindex, desiredvalue);
      
      if (optmode == sutype::om_minimize) {
        
        if (value == sutype::bool_true) {
          undesiredsatindices.push_back (satindex);
          ++maxnumber;
        }
        else {
          assumptions.push_back (-satindex); // fix current solution: make absent permanenetly absent
        }
      }
      
      else if (optmode == sutype::om_maximize) {
        
        if (value == sutype::bool_false) {
          undesiredsatindices.push_back (-satindex);
          ++maxnumber;
        }
        else {
          assumptions.push_back (satindex); // fix current solution: make present permanenetly present
        }
      }
      
      else {
        SUASSERT (false, "");
      }
    }

    if (maxnumber == 0) {
      suClauseBank::return_clause (undesiredsatindices);
      suClauseBank::return_clause (assumptions);
      return maxnumber;
    }

    SUASSERT (!undesiredsatindices.empty(), "");

    // run greedy approach and try to reduce maxnumber
    if (!fastmode) {
      
      sutype::clause_t & mandatorysatindices = suClauseBank::loan_clause ();
      
      detect_optional_satindices_ (assumptions, undesiredsatindices, mandatorysatindices);
      
      SUASSERT (undesiredsatindices.empty(), "");
      SUASSERT (assumptions.size() + mandatorysatindices.size() == clause.size(), "");
      
      maxnumber = mandatorysatindices.size();
      
      suClauseBank::return_clause (mandatorysatindices);
    }
    
    suClauseBank::return_clause (undesiredsatindices);
    suClauseBank::return_clause (assumptions);

    return maxnumber;
    
  } // end of suSatSolverWrapper::estimate_the_number_of_mandatory_sat_indices_

  //
  void suSatSolverWrapper::detect_optional_satindices_ (sutype::clause_t & assumptions,
                                                        sutype::clause_t & undesiredsatindices,
                                                        sutype::clause_t & mandatorysatindices)
  {
    const bool np = false;

    SUINFO(np) << "assumptions: " << suStatic::to_str (assumptions) << std::endl;
    SUINFO(np) << "undesiredsatindices: " << suStatic::to_str (undesiredsatindices) << std::endl;
    SUINFO(np) << "mandatorysatindices: " << suStatic::to_str (mandatorysatindices) << std::endl;

    bool ok = true;

    // debug
    if (0) {
      const double cputime0 = suTimeManager::instance()->get_cpu_time();
      ok = solve_the_problem (assumptions);
      suStatic::increment_elapsed_time (sutype::ts_sat_other_time, suTimeManager::instance()->get_cpu_time() - cputime0);
    }
    
    SUASSERT (ok, "");

    if (undesiredsatindices.empty()) {
      SUASSERT (false, "");
      return;
    }

    if (model_is_valid()) {

      sutype::uvi_t counter = 0;

      for (sutype::uvi_t i=0; i < undesiredsatindices.size(); ++i) {

        sutype::satindex_t satindex = undesiredsatindices[i];
        sutype::bool_t value = get_modeled_value (satindex);
        
        // found an optional satindex for free
        if (value == sutype::bool_false) {
          assumptions.push_back (-satindex);
          continue;
        }

        undesiredsatindices[counter] = satindex;
        ++counter;
      }
      
      if (undesiredsatindices.size() != counter) {
        //SUINFO(1) << "Found " << (undesiredsatindices.size() - counter) << " optional sat indices for free" << std::endl;
        undesiredsatindices.resize (counter);
      }

      // all undesired satindices are optional
      if (undesiredsatindices.empty()) return;
    }

    for (const auto & iter : undesiredsatindices) {
      
      sutype::satindex_t satindex = iter;
      assumptions.push_back (-satindex);
    }

    const double cputime0 = suTimeManager::instance()->get_cpu_time();
    ok = solve_the_problem (assumptions);
    suStatic::increment_elapsed_time (sutype::ts_sat_bound_estimation, suTimeManager::instance()->get_cpu_time() - cputime0);

    if (ok) {
      undesiredsatindices.clear();
      return;
    }

    if (undesiredsatindices.size() == 1) {
      sutype::satindex_t satindex = undesiredsatindices.back();
      mandatorysatindices.push_back (satindex);
      undesiredsatindices.clear();
      assumptions.pop_back();
      return;
    }

    sutype::clause_t & undesiredsatindices1 = suClauseBank::loan_clause ();
    sutype::clause_t & undesiredsatindices2 = suClauseBank::loan_clause ();

    int counter = 0;

    for (const auto & iter : undesiredsatindices) {
      ++counter;
      sutype::satindex_t satindex = iter;
      assumptions.pop_back();
      if (counter % 2)
        undesiredsatindices1.push_back (satindex);
      else
        undesiredsatindices2.push_back (satindex);
    }

    undesiredsatindices.clear();


    detect_optional_satindices_ (assumptions, undesiredsatindices1, mandatorysatindices);
    detect_optional_satindices_ (assumptions, undesiredsatindices2, mandatorysatindices);
    
    suClauseBank::return_clause (undesiredsatindices1);
    suClauseBank::return_clause (undesiredsatindices2);
    
  } // end of suSatSolverWrapper::detect_optional_satindices_
  
} // end of namespace amsr

// end of suSatSolverWrapper.cpp
