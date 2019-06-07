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

//! \file   suSatSolverUnitTestB.cpp
//! \brief  Classical chess problem. Here, I check SAT and Boolean counters.

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
#include <suClauseBank.h>
#include <suSatSolverWrapper.h>

// module include
#include <suSatSolverUnitTestB.h>

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
  void suSatSolverUnitTestB::run_unit_test (const unsigned problemsize,
                                            const int implementationid)
  {
    SUINFO(1)
      << "Run suSatSolverUnitTestB"
      << ": problemsize=" << problemsize
      << "; implementation=" << implementationid
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

    // emit one of answers:
    //
    // 7    . . . o . . . . 
    // 6    . . . . . . o . 
    // 5    . . o . . . . . 
    // 4    . . . . . . . o 
    // 3    . o . . . . . . 
    // 2    . . . . o . . . 
    // 1    o . . . . . . . 
    // 0    . . . . . o . .
    //      
    //      0 1 2 3 4 5 6 7
    //
    if (1) {
      if (problemsize == 8) {
        suSatSolverWrapper::instance()->emit_ALWAYS_ONE (board[0][1]);
        suSatSolverWrapper::instance()->emit_ALWAYS_ONE (board[1][3]);
        suSatSolverWrapper::instance()->emit_ALWAYS_ONE (board[2][5]);
        suSatSolverWrapper::instance()->emit_ALWAYS_ONE (board[3][7]);
        suSatSolverWrapper::instance()->emit_ALWAYS_ONE (board[4][2]);
        suSatSolverWrapper::instance()->emit_ALWAYS_ONE (board[5][0]);
        suSatSolverWrapper::instance()->emit_ALWAYS_ONE (board[6][6]);
        suSatSolverWrapper::instance()->emit_ALWAYS_ONE (board[7][4]);
      }
    }
    
    // just pair conflicts (fast)
    // a*b must be 0
    // following will be emitted to CNF:
    // (!a + !b) == 1
    // Advantage: no additional variables
    if (implementationid == 0) {
      
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

      // diagonal conflicts
      for (int x1=0; x1 < (int)problemsize; ++x1) {
        for (int y1=0; y1 < (int)problemsize; ++y1) {
        
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
              if (board[x1][y1] > board[x2][y2]) continue; // to avoid duplication

              //if (abs (x1-x2) != 1 || abs (y1-y2) != 1) continue;
              
              suSatSolverWrapper::instance()->emit_constraint (sutype::logic_func_and, sutype::bool_false, board[x1][y1], board[x2][y2]);
            }
          }
        }
      }
    }

    // counters (slow)
    else if (implementationid == 1) {

      // must be <= 1 in a column
      for (unsigned x=0; x < problemsize; ++x) {
        const sutype::clause_t & col = board[x];
        sutype::clause_t & counter = suSatSolverWrapper::instance()->build_assumption_max_number_of_outs_used_from_the_clause (col, 1, 1);
        suSatSolverWrapper::instance()->emit_ALWAYS_ONE (counter[1]);
        suClauseBank::return_clause (counter);
      }
      
      // must be <= 1 in a row
      for (unsigned y=0; y < problemsize; ++y) {
        sutype::clause_t & row = suClauseBank::loan_clause ();
        for (unsigned x=0; x < problemsize; ++x) {
          row.push_back (board[x][y]);
        }
        sutype::clause_t & counter = suSatSolverWrapper::instance()->build_assumption_max_number_of_outs_used_from_the_clause (row, 1, 1);
        suSatSolverWrapper::instance()->emit_ALWAYS_ONE (counter[1]);
        suClauseBank::return_clause (counter);
        suClauseBank::return_clause (row);
      }

      // must be <= 1 in every diagonal
      for (int n=-problemsize; n <= (int)problemsize + (int)problemsize - 1; ++n) {
        
        sutype::clause_t & diagonal1 = suClauseBank::loan_clause ();
        sutype::clause_t & diagonal2 = suClauseBank::loan_clause ();
        
        for (int mode = 1; mode <= 2; ++mode) {
          
          const int incrx = 1;
          const int incry = (mode == 1) ? 1 : -1;
          
          int x2 = -1;
          int y2 = n;
          
          while (1) {
            
            x2 += incrx;
            y2 += incry;
            
            if (x2 >= (int)problemsize) break;
            
            if (x2 >= 0 && x2 < (int)problemsize && y2 >= 0 && y2 < (int)problemsize) {
              if (mode == 1) diagonal1.push_back (board[x2][y2]);
              if (mode == 2) diagonal2.push_back (board[x2][y2]);
            }
          }
        }

        SUASSERT (diagonal1.size() >= 0 && diagonal1.size() <= problemsize, "");
        SUASSERT (diagonal2.size() >= 0 && diagonal2.size() <= problemsize, "");
        SUASSERT (!diagonal1.empty() || !diagonal2.empty(), "");
        
        if (n == -1) {
          SUASSERT (diagonal1.size() == problemsize, "");
          SUASSERT (diagonal2.size() == 0, "");
        }

        if (n == (int)problemsize) {
          SUASSERT (diagonal1.size() == 0, "");
          SUASSERT (diagonal2.size() == problemsize, "");
        }
        
        if (diagonal1.size() >= 1) {
          sutype::clause_t & counter = suSatSolverWrapper::instance()->build_assumption_max_number_of_outs_used_from_the_clause (diagonal1, 1, 1);
          suSatSolverWrapper::instance()->emit_ALWAYS_ONE (counter[1]);
          suClauseBank::return_clause (counter);
        }
        
        if (diagonal2.size() >= 1) {
          sutype::clause_t & counter = suSatSolverWrapper::instance()->build_assumption_max_number_of_outs_used_from_the_clause (diagonal2, 1, 1);
          suSatSolverWrapper::instance()->emit_ALWAYS_ONE (counter[1]);
          suClauseBank::return_clause (counter);
        }
        
        suClauseBank::return_clause (diagonal1);
        suClauseBank::return_clause (diagonal2);
      }
    }
    
    // extra vars
    else if (implementationid == 2) {

      // all conflicts
      for (int x1=0; x1 < (int)problemsize; ++x1) {
        for (int y1=0; y1 < (int)problemsize; ++y1) {

          sutype::clause_t & orclause = suClauseBank::loan_clause ();

          // North
          for (int y2=y1+1; y2 < (int)problemsize; ++y2) {
            orclause.push_back (board[x1][y2]);
          }

          // SE, E, NE
          for (int x2=x1+1; x2 < (int)problemsize; ++x2) {
            
            int delta = x2-x1;
            int y2 = y1 + delta; // North
            int y4 = y1 - delta; // South
            
            if (y4 >= 0)
              orclause.push_back (board[x2][y4]); // SE
            
            orclause.push_back (board[x2][y1]); // E
            
            if (y2 < (int)problemsize)
              orclause.push_back (board[x2][y2]); // NE
          }
          
          if (orclause.empty()) {
            suClauseBank::return_clause (orclause);
            continue;
          }

          // don't need to sort
          for (unsigned i=0; i+1 < orclause.size(); ++i) {
            SUASSERT (orclause[i+1] > orclause[i], "");
          }

          // a*(b+c+d)
          // following will be emitted to the CNF
          // (outa + !b) * (outa + !c) * (outa + !d) * (!outa + !a) --> must be 1
                    
          sutype::satindex_t outa = suSatSolverWrapper::instance()->emit_OR_or_return_constant (orclause, suSatSolverWrapper::instance()->get_constant(0));
          suClauseBank::return_clause (orclause);
          
          suSatSolverWrapper::instance()->emit_constraint (sutype::logic_func_and, sutype::bool_false, board[x1][y1], outa);
        }
      }
    }
    
    else if (implementationid == 3) {

      const unsigned N = problemsize;
      
      // N^3 - N + 2 * SUM (n*n-n, where n={2, N-1})
      unsigned numconflicts = 0;
      
      numconflicts += N*((N*N - N)/2); // rows
      numconflicts += N*((N*N - N)/2); // cols
      
      // non-main diagonals
      for (unsigned n = 2; n+1 <= N; ++n) {
        numconflicts += 4 * ((n*n - n) / 2);
      }

      // main diagonals
      numconflicts += 2 * ((N*N - N)/2);

      unsigned numClauses = 0;
      numClauses += 2; // two constants
      numClauses += numconflicts;
      numClauses += N; // OR: at least one must be in a column
      numClauses += N; // OR: at least one must be in a row

      unsigned numLiterals = 0;
      numLiterals += 2;   // constants
      numLiterals += N*N; // board

      SUINFO(1) << "Minimal literals:  " << numLiterals << std::endl;
      SUINFO(1) << "Minimal clauses:   " << numClauses << std::endl;
      SUINFO(1) << "Minimal conflicts: " << numconflicts << std::endl;
      
      unsigned num = 0;
      
      sutype::satindex_t prevsatindex = 0;

      sutype::clause_t & orclause = suClauseBank::loan_clause ();
      
      // all conflicts in a sorted order
      for (int x1=0; x1 < (int)problemsize; ++x1) {
        for (int y1=0; y1 < (int)problemsize; ++y1) {

          sutype::satindex_t satindex = board[x1][y1];
          SUASSERT (satindex > prevsatindex, "");
          prevsatindex = satindex;

          orclause.clear ();
          
          // North
          for (int y2=y1+1; y2 < (int)problemsize; ++y2) {
            orclause.push_back (board[x1][y2]);
          }

          // SE, E, NE
          for (int x2=x1+1; x2 < (int)problemsize; ++x2) {
            
            int delta = x2-x1;
            int y2 = y1 + delta; // North
            int y4 = y1 - delta; // South
            
            if (y4 >= 0)
              orclause.push_back (board[x2][y4]); // SE
            
            orclause.push_back (board[x2][y1]); // E
            
            if (y2 < (int)problemsize)
              orclause.push_back (board[x2][y2]); // NE
          }
          
          // don't need to sort
          for (unsigned i=0; i+1 < orclause.size(); ++i) {
            SUASSERT (orclause[i+1] > orclause[i], "");
          }

          // emit constraints
          for (unsigned i=0; i < orclause.size(); ++i) {
            suSatSolverWrapper::instance()->emit_AND_ALWAYS_ZERO (satindex, orclause[i]);
            ++num;
          }          
        }
      }

      suClauseBank::return_clause (orclause);

      SUINFO(1) << "Created conflicts: " << num << std::endl;
    }
    
    else {
      SUASSERT (false, "");
    }
        
    if (1) {
      bool ok = suSatSolverWrapper::instance()->simplify();
      SUASSERT (ok, "Can't simplify");
    }

    if (1) {
      bool ok = suSatSolverWrapper::instance()->solve_the_problem ();
      SUASSERT (ok, "Can't solve");
    }
    
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
    
  } // end of suSatSolverUnitTestB::run_unit_test
  

  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------

  // static
  void suSatSolverUnitTestB::check_diagonal_sum_ (int x1,
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

// end of suSatSolverUnitTestB.cpp
