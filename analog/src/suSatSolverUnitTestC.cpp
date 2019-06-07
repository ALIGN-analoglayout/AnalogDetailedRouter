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
//! \date   Mon Oct  2 09:46:40 2017

//! \file   suSatSolverUnitTestC.cpp
//! \brief  Checks boolean counters -- both hard implementation and assumptions.

// std includes
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
#include <suSatSolverWrapper.h>

// module include
#include <suSatSolverUnitTestC.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------


  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  // static
  void suSatSolverUnitTestC::run_unit_test (const unsigned problemsize)
  {
    SUINFO(1)
      << "Run suSatSolverUnitTestC"
      << ": problemsize=" << problemsize
      << std::endl;

    SUASSERT (problemsize >= 1, "");

    // mode=1 hard
    // mode=2 assumptions
    for (int mode = 1; mode <= 2; ++mode) {
    
      // test build_assumption_max_number_of_outs_used_from_the_clause
      for (int minvalue = 0; minvalue <= (int)problemsize; ++minvalue) {
      
        for (int maxvalue = minvalue; maxvalue <= (int)problemsize; ++maxvalue) {
        
          sutype::clause_t & clause = suClauseBank::loan_clause ();
        
          for (unsigned i=0; i < problemsize; ++i) {
            clause.push_back (suSatSolverWrapper::instance()->get_next_sat_index());
          }

          // emit this back-end dummy literal to guarantee that all other smaller literals will be in the model
          suSatSolverWrapper::instance()->emit_ALWAYS_ONE (suSatSolverWrapper::instance()->get_next_sat_index());
          
          // i=0 <= 0
          // i=1 <= 1
          // i=2 <= 2
          // etc.
          sutype::clause_t & counter = suSatSolverWrapper::instance()->build_assumption_max_number_of_outs_used_from_the_clause (clause, minvalue, maxvalue);
          SUASSERT (counter.size() == clause.size()+1, "");
        
          for (int i = 0; i < (int)counter.size(); ++i) {
            sutype::satindex_t satindex = counter[i];
            if        (i < minvalue) { SUASSERT (satindex == 0, "");
            } else if (i > maxvalue) { SUASSERT (satindex == 0, "");
            } else {
              SUASSERT (satindex, "minvalue=" << minvalue << "; maxvalue=" << maxvalue << "; i=" << i);
            }
          }

          std::vector<int> values;

          // want to test hard mode (can use only this order)
          if (mode == 1) {
            for (int value = maxvalue; value >= minvalue; --value) {
              values.push_back (value);
            }
          }
          // want to test assumptions (want to test this order)
          else {
            for (int value = minvalue; value <= maxvalue; ++value) {
              values.push_back (value);
            }
          }
        
          for (unsigned v=0; v < values.size(); ++v) {

            int value = values [v];
            sutype::satindex_t satindex = counter[value];
            SUASSERT (satindex, "");

            // hard
            if (mode == 1) {
              suSatSolverWrapper::instance()->emit_ALWAYS_ONE (satindex);
              bool ok = suSatSolverWrapper::instance()->solve_the_problem ();
              SUASSERT (ok, "");
            }
            // assumptions
            else {
              sutype::clause_t & assumptions = suClauseBank::loan_clause ();
              assumptions.push_back (satindex);
              bool ok = suSatSolverWrapper::instance()->solve_the_problem (assumptions);
              SUASSERT (ok, "");
              suClauseBank::return_clause (assumptions);
            }
            
            int num = 0;

            for (unsigned i=0; i < clause.size(); ++i) {
            
              sutype::satindex_t literal = clause[i];
              SUASSERT (literal > 0, "");
              if (suSatSolverWrapper::instance()->get_modeled_value (literal) == sutype::bool_true)
                ++num;
            }

            SUASSERT (num <= value, "minvalue=" << minvalue << "; maxvalue=" << maxvalue << "; value=" << value << "; num=" << num);
          }
        
          suClauseBank::return_clause (counter);
          suClauseBank::return_clause (clause);
        }
      }

      // test build_assumption_min_number_of_outs_used_from_the_clause
      for (int minvalue = 0; minvalue <= (int)problemsize; ++minvalue) {
      
        for (int maxvalue = minvalue; maxvalue <= (int)problemsize; ++maxvalue) {

          sutype::clause_t & clause = suClauseBank::loan_clause ();
        
          for (unsigned i=0; i < problemsize; ++i) {
            clause.push_back (suSatSolverWrapper::instance()->get_next_sat_index());
          }
        
          // emit this bach-end dummy literal to guarantee that all other smaller literals will be in the model
          suSatSolverWrapper::instance()->emit_ALWAYS_ONE (suSatSolverWrapper::instance()->get_next_sat_index());
        
          // i=0 >= 0 (i.e. constant1, always satisfiable)
          // i=1 >= 1
          // i=2 >= 2
          // etc.
          sutype::clause_t & counter = suSatSolverWrapper::instance()->build_assumption_min_number_of_outs_used_from_the_clause (clause, minvalue, maxvalue);
          SUASSERT (counter.size() == clause.size()+1, "");
        
          for (int i = 0; i < (int)counter.size(); ++i) {
            sutype::satindex_t satindex = counter[i];
            if        (i < minvalue) { SUASSERT (satindex == 0, "");
            } else if (i > maxvalue) { SUASSERT (satindex == 0, "");
            } else {
              SUASSERT (satindex, "minvalue=" << minvalue << "; maxvalue=" << maxvalue << "; i=" << i << "; counter.size()=" << counter.size());
            }
          }

          std::vector<int> values;

          // want to test hard mode (can use only this order)
          if (mode == 1) {
            for (int value = minvalue; value <= maxvalue; ++value) {
              values.push_back (value);
            }
          }
          // want to test assumptions (want to test this order)
          else {
            for (int value = maxvalue; value >= minvalue; --value) {
              values.push_back (value);
            }
          }

          for (unsigned v=0; v < values.size(); ++v) {

            int value = values [v];
            sutype::satindex_t satindex = counter[value];
            SUASSERT (satindex, "");
          
            // hard
            if (mode == 1) {
              suSatSolverWrapper::instance()->emit_ALWAYS_ONE (satindex);
              bool ok = suSatSolverWrapper::instance()->solve_the_problem ();
              SUASSERT (ok, "");
            }
            // assumptions
            else {
              sutype::clause_t & assumptions = suClauseBank::loan_clause ();
              assumptions.push_back (satindex);
              bool ok = suSatSolverWrapper::instance()->solve_the_problem (assumptions);
              SUASSERT (ok, "");
              suClauseBank::return_clause (assumptions);
            }
                    
            int num = 0;
          
            for (unsigned i=0; i < clause.size(); ++i) {
            
              sutype::satindex_t literal = clause[i];
              SUASSERT (literal > 0, "");
              if (suSatSolverWrapper::instance()->get_modeled_value(literal) == sutype::bool_true)
                ++num;
            }
          
            SUASSERT (num >= value, "minvalue=" << minvalue << "; maxvalue=" << maxvalue << "; value=" << value << "; num=" << num);
          }
        
          suClauseBank::return_clause (counter);
          suClauseBank::return_clause (clause);
        }
      }
    }
    
  } // end of suSatSolverUnitTestC::run_unit_test


  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------


} // end of namespace amsr

// end of suSatSolverUnitTestC.cpp
