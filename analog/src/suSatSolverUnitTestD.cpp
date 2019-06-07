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
//! \date   Fri Sep 29 10:18:16 2017

//! \file   suSatSolverUnitTestD.cpp
//! \brief  Checks incrementally added constraints.

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
#include <suStatic.h>

// module include
#include <suSatSolverUnitTestD.h>

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
  void suSatSolverUnitTestD::run_unit_test (const unsigned problemsize)
  {
    SUINFO(1)
      << "Run suSatSolverUnitTestD"
      << ": problemsize=" << problemsize
      << std::endl;
    
    sutype::clause_t tmpclause (problemsize, 0);
    sutype::clauses_t board (problemsize, tmpclause);

    // get variables
    for (unsigned x=0; x < problemsize; ++x) {
      for (unsigned y=0; y < problemsize; ++y) {
        board[x][y] = suSatSolverWrapper::instance()->get_next_sat_index();
      }
    }
    
    // at least one entry must be populated in a column
    for (unsigned x=0; x < problemsize; ++x) {
      const sutype::clause_t & col = board[x];
      suSatSolverWrapper::instance()->emit_constraint (sutype::logic_func_or, sutype::bool_true, col);
    }

    // at least one entry must be populated in a row
    for (unsigned y=0; y < problemsize; ++y) {
      sutype::clause_t & row = suClauseBank::loan_clause ();
      for (unsigned x=0; x < problemsize; ++x) {
        row.push_back (board[x][y]);
      }
      suSatSolverWrapper::instance()->emit_constraint (sutype::logic_func_or, sutype::bool_true, row);
      suClauseBank::return_clause (row);
    }
    
    // can't be two in a column
    for (unsigned x=0; x < problemsize; ++x) {
      for (unsigned y1=0; y1 < problemsize; ++y1) {
        for (unsigned y2=y1+1; y2 < problemsize; ++y2) {
          suSatSolverWrapper::instance()->emit_constraint (sutype::logic_func_and, sutype::bool_false, board[x][y1], board[x][y2]);
        }
      }
    }
    
    // can't be two in a row
    for (unsigned y=0; y < problemsize; ++y) {
      for (unsigned x1=0; x1 < problemsize; ++x1) {
        for (unsigned x2=x1+1; x2 < problemsize; ++x2) {
          suSatSolverWrapper::instance()->emit_constraint (sutype::logic_func_and, sutype::bool_false, board[x1][y], board[x2][y]);
        }
      }
    }

    // emit this back-end dummy literal to guarantee that all other smaller literals will be in the model
    //suSatSolverWrapper::instance()->emit_ALWAYS_ONE (suSatSolverWrapper::instance()->get_next_sat_index());

    std::set<std::pair<sutype::satindex_t,sutype::satindex_t> > conflicts;
    
    // incrementally add new pair constraints
    while (1) {
      
      if (1) {
        bool ok = suSatSolverWrapper::instance()->simplify();
        SUASSERT (ok, "Can't simplify");
      }
      
      if (1) {
        //SUINFO(1) << "Solve the problem" << std::endl;
        bool ok = suSatSolverWrapper::instance()->solve_the_problem ();
        SUASSERT (ok, "Can't solve");
      }
      
      sutype::bools_t model = suSatSolverWrapper::instance()->get_model(); // get a copy
      SUASSERT (!model.empty(), "");

      bool solutionIsClean = true;
      
      for (int x1=0; x1 < (int)problemsize; ++x1) {
        for (int y1=0; y1 < (int)problemsize; ++y1) {
          
          sutype::satindex_t satindex1 = board[x1][y1];
          SUASSERT (satindex1 > 0 && satindex1 < (int)model.size(), "satindex1=" << satindex1 << "; model.size()=" << model.size());
          
          sutype::bool_t value1 = model[satindex1];
          if (value1 != sutype::bool_true) continue;

          // check diagonals
          for (int mode = 1; mode <= 4; ++mode) {
      
            int incrx = 0;
            int incry = 0;

            if        (mode == 1) { incrx =  1; incry =  1;
            } else if (mode == 2) { incrx = -1; incry = -1;
            } else if (mode == 3) { incrx =  1; incry = -1;
            } else if (mode == 4) { incrx = -1; incry =  1;
            } else {
              SUASSERT (false, "");
            }

            int x2 = x1;
            int y2 = y1;
          
            while (1) {
            
              x2 += incrx;
              y2 += incry;
            
              if (x2 < 0) break;
              if (y2 < 0) break;
              if (x2 >= (int)problemsize) break;
              if (y2 >= (int)problemsize) break;

              sutype::satindex_t satindex2 = board[x2][y2];
              SUASSERT (satindex2 > 0 && satindex2 < (int)model.size(), "satindex2=" << satindex2 << "; model.size()=" << model.size());
              sutype::bool_t value2 = model[satindex2];
              if (value2 != sutype::bool_true) continue;
              if (satindex2 > satindex1) continue;

              std::pair<sutype::satindex_t,sutype::satindex_t> conflict (satindex1, satindex2);
              
              if (conflicts.count (conflict) > 0) {
                
                SUISSUE("Found an old conflict")
                  << ": between satindex1=" << satindex1 << " and satindex2=" << satindex2
                  << "; value1=" << suStatic::bool_2_str (value1)
                  << "; value2=" << suStatic::bool_2_str (value2)
                  << "; p1=(" << x1 << ";" << y1 << ")"
                  << "; p2=(" << x2 << ";" << y2 << ")"
                  << std::endl;
                
                SUASSERT (false, "");
              }

              conflicts.insert (conflict);
              
              // this function cleans the model
              suSatSolverWrapper::instance()->emit_constraint (sutype::logic_func_and, sutype::bool_false, satindex1, satindex2);
              solutionIsClean = false;
            }
            
          } //for(mode)
        } //for(x1)
      } // for(y1)      

      if (solutionIsClean) break;

    } // while(1)
    
    // print
    for (int y = (int)problemsize-1; y >= 0; --y) {
      for (int x=0; x < (int)problemsize; ++x) {
        
        sutype::satindex_t satindex = board[x][y];
        SUASSERT (satindex > 0, "");
        sutype::bool_t value = suSatSolverWrapper::instance()->get_modeled_value (satindex);
        
        if        (value == sutype::bool_true)  { SUOUT(1) << "o";
        } else if (value == sutype::bool_false) { SUOUT(1) << ".";
        } else                                  { SUOUT(1) << "x";
        }

        SUOUT(1) << " ";
      }
      SUOUT(1) << std::endl;
    }

    // check #1
    for (unsigned x=0; x < problemsize; ++x) {
      unsigned num = 0;
      for (unsigned y=0; y < problemsize; ++y) {
        sutype::satindex_t satindex = board[x][y];
        sutype::bool_t value = suSatSolverWrapper::instance()->get_modeled_value (satindex);
        if (value == sutype::bool_true)
          ++num;
      }
      SUASSERT (num == 1, "");
    }

    // check #2
    for (unsigned y=0; y < problemsize; ++y) {
      unsigned num = 0;
      for (unsigned x=0; x < problemsize; ++x) {
        sutype::satindex_t satindex = board[x][y];
        sutype::bool_t value = suSatSolverWrapper::instance()->get_modeled_value (satindex);
        if (value == sutype::bool_true)
          ++num;
      }
      SUASSERT (num == 1, "");
    }
    
    // check #3
    for (unsigned n=0; n < problemsize; ++n) {
      check_diagonal_sum_ (n, 0, board);
      check_diagonal_sum_ (n, problemsize-1, board);
      check_diagonal_sum_ (0, n, board);
      check_diagonal_sum_ (problemsize-1, n, board);
    }
    
  } // end of suSatSolverUnitTestD::run_unit_test
  

  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------

  // static
  void suSatSolverUnitTestD::check_diagonal_sum_ (int x1,
                                                  int y1,
                                                  const sutype::clauses_t & board)
  {
    const int problemsize = board.size();

    int num[5] = {0, 0, 0, 0, 0};
    
    sutype::satindex_t satindex1 = board[x1][y1];
    sutype::bool_t value1 = suSatSolverWrapper::instance()->get_modeled_value (satindex1);
    if (value1 == sutype::bool_true)
      ++num[0];
    
    for (int mode = 1; mode <= 4; ++mode) {
      
      int incrx = 0;
      int incry = 0;

      if        (mode == 1) { incrx =  1; incry =  1;
      } else if (mode == 2) { incrx = -1; incry = -1;
      } else if (mode == 3) { incrx =  1; incry = -1;
      } else if (mode == 4) { incrx = -1; incry =  1;
      } else {
        SUASSERT (false, "");
      }

      int x2 = x1;
      int y2 = y1;
          
      while (1) {
            
        x2 += incrx;
        y2 += incry;
            
        if (x2 < 0) break;
        if (y2 < 0) break;
        if (x2 >= problemsize) break;
        if (y2 >= problemsize) break;

        sutype::satindex_t satindex2 = board[x2][y2];
        sutype::bool_t value2 = suSatSolverWrapper::instance()->get_modeled_value (satindex2);
        if (value2 == sutype::bool_true)
          ++num[mode];
      }
    }

    SUASSERT ((num[0] + num[1] + num[2]) <= 1, "");
    SUASSERT ((num[0] + num[3] + num[4]) <= 1, "");
    
  } // end of check_diagonal_sum_

} // end of namespace amsr

// end of suSatSolverUnitTestD.cpp
