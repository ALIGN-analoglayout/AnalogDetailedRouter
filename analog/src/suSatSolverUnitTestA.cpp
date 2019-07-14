// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Thu Sep 28 10:16:30 2017

//! \file   suSatSolverUnitTestA.cpp
//! \brief  Classical Einstein's problem. It has the only correct answer if SAT coded correctly.

// std includes
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suClauseBank.h>
#include <suSatSolverWrapper.h>

// module include
#include <suSatSolverUnitTestA.h>

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

  //
  void suSatSolverUnitTestA::run_unit_test ()
  {
    const bool np = !false;

    std::vector<std::string> strs;

    int i = 0;

    int h1 = ++i; const int Blue       = h1; strs.push_back ("Blue"); const int firstIndex = h1;
    int h2 = ++i; const int Red        = h2; strs.push_back ("Red");
    int h3 = ++i; const int Green      = h3; strs.push_back ("Green");
    int h4 = ++i; const int White      = h4; strs.push_back ("White");
    int h5 = ++i; const int Yellow     = h5; strs.push_back ("Yellow");

    int p1 = ++i; const int Number1    = p1; strs.push_back ("Number1");
    int p2 = ++i; const int Number2    = p2; strs.push_back ("Number2");
    int p3 = ++i; const int Number3    = p3; strs.push_back ("Number3");
    int p4 = ++i; const int Number4    = p4; strs.push_back ("Number4");
    int p5 = ++i; const int Number5    = p5; strs.push_back ("Number5");

    int n1 = ++i; const int Brit       = n1; strs.push_back ("Brit");
    int n2 = ++i; const int Dane       = n2; strs.push_back ("Dane");
    int n3 = ++i; const int German     = n3; strs.push_back ("German");
    int n4 = ++i; const int Norwegian  = n4; strs.push_back ("Norwegian");
    int n5 = ++i; const int Swede      = n5; strs.push_back ("Swede");

    int c1 = ++i; const int BlueMaster = c1; strs.push_back ("BlueMaster");
    int c2 = ++i; const int Dunhill    = c2; strs.push_back ("Dunhill");
    int c3 = ++i; const int PallMall   = c3; strs.push_back ("PallMall");
    int c4 = ++i; const int Prince     = c4; strs.push_back ("Prince");
    int c5 = ++i; const int Blend      = c5; strs.push_back ("Blend");

    int a1 = ++i; const int Cat        = a1; strs.push_back ("Cat");
    int a2 = ++i; const int Bird       = a2; strs.push_back ("Bird");
    int a3 = ++i; const int Dog        = a3; strs.push_back ("Dog");
    int a4 = ++i; const int Fish       = a4; strs.push_back ("Fish");
    int a5 = ++i; const int Horse      = a5; strs.push_back ("Horse");

    int b1 = ++i; const int Beer       = b1; strs.push_back ("Beer");
    int b2 = ++i; const int Coffee     = b2; strs.push_back ("Coffee");
    int b3 = ++i; const int Milk       = b3; strs.push_back ("Milk");
    int b4 = ++i; const int Tea        = b4; strs.push_back ("Tea");
    int b5 = ++i; const int Water      = b5; strs.push_back ("Water"); const int lastIndex = b5;

    std::vector<int> h; h.push_back (h1); h.push_back (h2); h.push_back (h3); h.push_back (h4); h.push_back (h5); 
    std::vector<int> p; p.push_back (p1); p.push_back (p2); p.push_back (p3); p.push_back (p4); p.push_back (p5);
    std::vector<int> n; n.push_back (n1); n.push_back (n2); n.push_back (n3); n.push_back (n4); n.push_back (n5);
    std::vector<int> c; c.push_back (c1); c.push_back (c2); c.push_back (c3); c.push_back (c4); c.push_back (c5);
    std::vector<int> a; a.push_back (a1); a.push_back (a2); a.push_back (a3); a.push_back (a4); a.push_back (a5);
    std::vector<int> b; b.push_back (b1); b.push_back (b2); b.push_back (b3); b.push_back (b4); b.push_back (b5);

    std::map<std::pair<int,int>, sutype::satindex_t> itIsInPosition;
    
    for (int index = firstIndex; index <= lastIndex; ++index) {
      
      sutype::clause_t & vars = suClauseBank::loan_clause();
      
      for (int pos=1; pos <= 5; ++pos) {
        
        sutype::satindex_t var = suSatSolverWrapper::instance()->get_next_sat_index();
        itIsInPosition [std::pair<int,int>(index,pos)] = var;
        vars.push_back (var);
        
        // emit starting constraints
        if (index >= p1 && index <= p5) {
          
          if (index - p1 + 1 == pos) 
            suSatSolverWrapper::instance()->emit_ALWAYS_ONE (var);
          else
            suSatSolverWrapper::instance()->emit_ALWAYS_ZERO  (var);
        }
      }

      suSatSolverWrapper::instance()->emit_OR_ALWAYS_ONE (vars); // any value is always in any position
      
      // any value can't be only in one position
      for (sutype::uvi_t i=0; i < vars.size(); ++i) {
        
        sutype::satindex_t var1 = vars[i];

        for (sutype::uvi_t k=i+1; k < vars.size(); ++k) {

          sutype::satindex_t var2 = vars[k];
          
          sutype::clause_t & clause = suClauseBank::loan_clause();
          clause.push_back (var1);
          clause.push_back (var2);
          suSatSolverWrapper::instance()->emit_AND_ALWAYS_ZERO (clause);
          suClauseBank::return_clause (clause);
        }
      }

      suClauseBank::return_clause (vars);
    }
    
    // 0. emit ground rules
    suSatSolverUnitTestA::unit_test_basic_rules (h, itIsInPosition);
    suSatSolverUnitTestA::unit_test_basic_rules (p, itIsInPosition);
    suSatSolverUnitTestA::unit_test_basic_rules (n, itIsInPosition);
    suSatSolverUnitTestA::unit_test_basic_rules (c, itIsInPosition);
    suSatSolverUnitTestA::unit_test_basic_rules (a, itIsInPosition);
    suSatSolverUnitTestA::unit_test_basic_rules (b, itIsInPosition);

    // 1. The Brit lives in a red house. 
    suSatSolverUnitTestA::unit_test_bind_two_vars (Brit, Red, itIsInPosition);

    // 2. The Swede keeps dogs as pets. 
    suSatSolverUnitTestA::unit_test_bind_two_vars (Swede, Dog, itIsInPosition);
    
    // 3. The Dane drinks tea. 
    suSatSolverUnitTestA::unit_test_bind_two_vars (Dane, Tea, itIsInPosition);

    // 4. The green house is on the left of the white house (next to it).
    // the order is: Green White
    suSatSolverUnitTestA::unit_test_two_vars_are_in_conflict (Green, Number5, itIsInPosition);
    suSatSolverUnitTestA::unit_test_two_vars_are_in_conflict (White, Number1, itIsInPosition);
    suSatSolverUnitTestA::unit_test_two_vars_are_next_to_each_other (Green, White, itIsInPosition, sutype::side_east);
    
    // 5. The green house owner drinks coffee. 
    suSatSolverUnitTestA::unit_test_bind_two_vars (Green, Coffee, itIsInPosition);

    // 6. The person who smokes PallMall rears birds. 
    suSatSolverUnitTestA::unit_test_bind_two_vars (PallMall, Bird, itIsInPosition);

    // 7. The owner of the yellow house smokes Dunhill. 
    suSatSolverUnitTestA::unit_test_bind_two_vars (Yellow, Dunhill, itIsInPosition);

    // 8. The man living in the house right in the center drinks milk. 
    suSatSolverUnitTestA::unit_test_bind_two_vars (Number3, Milk, itIsInPosition);
    
    // 9. The Norwegian lives in the first house. 
    suSatSolverUnitTestA::unit_test_bind_two_vars (Norwegian, Number1, itIsInPosition);

    // 10. The man who smokes Blend lives next to the one who keeps cats.
    suSatSolverUnitTestA::unit_test_two_vars_are_in_conflict (Blend, Cat, itIsInPosition);
    suSatSolverUnitTestA::unit_test_two_vars_are_next_to_each_other (Blend, Cat, itIsInPosition, sutype::side_undefined);
    
    // 11. The man who keeps horses lives next to the man who smokes Dunhill.
    suSatSolverUnitTestA::unit_test_two_vars_are_in_conflict (Horse, Dunhill, itIsInPosition);
    suSatSolverUnitTestA::unit_test_two_vars_are_next_to_each_other (Horse, Dunhill, itIsInPosition, sutype::side_undefined);
    
    // 12. The owner who smokes Blue Master drinks beer. 
    suSatSolverUnitTestA::unit_test_bind_two_vars (BlueMaster, Beer, itIsInPosition);

    // 13. The German smokes Prince. 
    suSatSolverUnitTestA::unit_test_bind_two_vars (German, Prince, itIsInPosition);

    // 14. The Norwegian lives next to the blue house.
    suSatSolverUnitTestA::unit_test_two_vars_are_in_conflict (Norwegian, Blue, itIsInPosition);
    suSatSolverUnitTestA::unit_test_two_vars_are_next_to_each_other (Norwegian, Blue, itIsInPosition, sutype::side_undefined);
    
    // 15. The man who smokes blend has a neighbor who drinks water.
    suSatSolverUnitTestA::unit_test_two_vars_are_in_conflict (Blend, Water, itIsInPosition);
    suSatSolverUnitTestA::unit_test_two_vars_are_next_to_each_other (Blend, Water, itIsInPosition, sutype::side_undefined);

    std::vector <std::vector<int> > answers;

    std::vector<int> answer1 = {Number1, Norwegian, Cat,   Water,  Dunhill,    Yellow};
    std::vector<int> answer2 = {Number2, Dane,      Horse, Tea,    Blend,      Blue};
    std::vector<int> answer3 = {Number3, Brit,      Bird,  Milk,   PallMall,   Red};
    std::vector<int> answer4 = {Number4, German,    Fish,  Coffee, Prince,     Green};
    std::vector<int> answer5 = {Number5, Swede,     Dog,   Beer,   BlueMaster, White};

    answers.push_back (answer1);
    answers.push_back (answer2);
    answers.push_back (answer3);
    answers.push_back (answer4);
    answers.push_back (answer5);

    // simplify
    if (1) {
      bool ok = suSatSolverWrapper::instance()->simplify();
      SUASSERT (ok, "Can't simplify the problem");
    }

    // solve
    if (1) {
      bool ok = suSatSolverWrapper::instance()->solve_the_problem ();
      SUASSERT (ok, "Can't solve the problem.");
    }
        
    // print answer
    for (int pos=1; pos <= 5; ++pos) {
      
      SUINFO(np) << "Position #" << pos << std::endl;
      
      const std::vector<int> & answer = answers[pos-1];
      int counter = 0;

      for (std::map<std::pair<int,int>, sutype::satindex_t>::const_iterator iter = itIsInPosition.begin(); iter != itIsInPosition.end(); ++iter) {

        const std::pair<int,int> & varAndPos = iter->first;
        sutype::satindex_t satindex = iter->second;

        int var2 = varAndPos.first;
        int pos2 = varAndPos.second;

        if (pos2 != pos) continue;
        
        sutype::bool_t ret = suSatSolverWrapper::instance()->get_modeled_value (satindex);
        if (ret != sutype::bool_true) continue;

        SUINFO(np) << "  " << strs[var2 - firstIndex] << std::endl;

        SUASSERT (std::find (answer.begin(), answer.end(), var2) != answer.end(), "");
        ++counter;
      }

      SUASSERT (counter == 6, "");
    }
    
  } // end of suSatSolverUnitTestA::run_unit_test
  
  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------

  // two vars can't be in one position
  void suSatSolverUnitTestA::unit_test_basic_rules (const std::vector<int> & vars,
                                                    const std::map<std::pair<int,int>, sutype::satindex_t> & itIsInPosition)
  {
    SUASSERT (vars.size() == 5, "");
    
    for (int pos = 1; pos <= 5; ++pos) {

      for (sutype::uvi_t i=0; i < vars.size(); ++i) {

        int var1 = vars[i];

        SUASSERT (itIsInPosition.count (std::pair<int,int>(var1,pos)) > 0, "");
        sutype::satindex_t satindex1 = itIsInPosition.at (std::pair<int,int>(var1,pos));

        for (sutype::uvi_t k=i+1; k < vars.size(); ++k) {
          
          int var2 = vars[k];

          SUASSERT (itIsInPosition.count (std::pair<int,int>(var2,pos)) > 0, "");
          sutype::satindex_t satindex2 = itIsInPosition.at (std::pair<int,int>(var2,pos));

          sutype::clause_t & clause = suClauseBank::loan_clause();
          clause.push_back (satindex1);
          clause.push_back (satindex2);
          suSatSolverWrapper::instance()->emit_AND_ALWAYS_ZERO (clause);
          suClauseBank::return_clause (clause);
        }
      }
    }
    
  } // end of suSatSolverUnitTestA::unit_test_basic_rules

  //
  void suSatSolverUnitTestA::unit_test_bind_two_vars (int var1,
                                                      int var2,
                                                      const std::map<std::pair<int,int>, sutype::satindex_t> & itIsInPosition)
  {
    sutype::clause_t & topclause = suClauseBank::loan_clause ();

    for (int pos1 = 1; pos1 <= 5; ++pos1) {

      SUASSERT (itIsInPosition.count (std::pair<int,int>(var1,pos1)) > 0, "");
      sutype::satindex_t satindex1 = itIsInPosition.at (std::pair<int,int>(var1,pos1));

      for (int pos2 = 1; pos2 <= 5; ++pos2) {

        SUASSERT (itIsInPosition.count (std::pair<int,int>(var2,pos2)) > 0, "");
        sutype::satindex_t satindex2 = itIsInPosition.at (std::pair<int,int>(var2,pos2));
        
        sutype::clause_t & clause = suClauseBank::loan_clause ();
        clause.push_back (satindex1);
        clause.push_back (satindex2);

        bool legal = true;

        if (pos1 != pos2) legal = false;

        if (!legal) {
          suSatSolverWrapper::instance()->emit_AND_ALWAYS_ZERO (clause);
        }
        else {
          sutype::satindex_t out = suSatSolverWrapper::instance()->emit_AND_or_return_constant (clause);
          topclause.push_back (out);
        }
        suClauseBank::return_clause (clause);
      }
    }

    suSatSolverWrapper::instance()->emit_OR_ALWAYS_ONE (topclause);
    
    suClauseBank::return_clause (topclause);

  } // end of suSatSolverUnitTestA::unit_test_bind_two_vars

  void suSatSolverUnitTestA::unit_test_two_vars_are_in_conflict (int var1,
                                                                 int var2,
                                                                 const std::map<std::pair<int,int>, sutype::satindex_t> & itIsInPosition)
  {
    for (int pos1 = 1; pos1 <= 5; ++pos1) {

      SUASSERT (itIsInPosition.count (std::pair<int,int>(var1,pos1)) > 0, "");
      sutype::satindex_t satindex1 = itIsInPosition.at (std::pair<int,int>(var1,pos1));

      for (int pos2 = 1; pos2 <= 5; ++pos2) {

        SUASSERT (itIsInPosition.count (std::pair<int,int>(var2,pos2)) > 0, "");
        sutype::satindex_t satindex2 = itIsInPosition.at (std::pair<int,int>(var2,pos2));

        bool legal = true;

        if (pos1 == pos2) legal = false;

        if (!legal) {
          sutype::clause_t & clause = suClauseBank::loan_clause ();
          clause.push_back (satindex1);
          clause.push_back (satindex2);
          suSatSolverWrapper::instance()->emit_AND_ALWAYS_ZERO (clause);
          suClauseBank::return_clause (clause);
        }
      }
    }
    
  } // end of suSatSolverUnitTestA::unit_test_two_vars_are_in_conflict

  void suSatSolverUnitTestA::unit_test_two_vars_are_next_to_each_other (int var1, // Portland
                                                                        int var2, // New York
                                                                        const std::map<std::pair<int,int>, sutype::satindex_t> & itIsInPosition,
                                                                        sutype::side_t side) // New York is on the east
  {
    for (int pos1 = 1; pos1 <= 5; ++pos1) {

      SUASSERT (itIsInPosition.count (std::pair<int,int>(var1,pos1)) > 0, "");
      sutype::satindex_t satindex1 = itIsInPosition.at (std::pair<int,int>(var1,pos1));

      for (int pos2 = 1; pos2 <= 5; ++pos2) {

        SUASSERT (itIsInPosition.count (std::pair<int,int>(var2,pos2)) > 0, "");
        sutype::satindex_t satindex2 = itIsInPosition.at (std::pair<int,int>(var2,pos2));

        bool legal = true;

        // Portland (var1) --> New York (pos2)
        if      (pos1 == pos2 || abs (pos1 - pos2) > 1) legal = false;
        else if (side == sutype::side_east && pos2 < pos1) legal = false;
        else if (side == sutype::side_west && pos2 > pos1) legal = false;

        if (!legal) {
          sutype::clause_t & clause = suClauseBank::loan_clause ();
          clause.push_back (satindex1);
          clause.push_back (satindex2);
          suSatSolverWrapper::instance()->emit_AND_ALWAYS_ZERO (clause);
          suClauseBank::return_clause (clause);
        }
      }
    }
    
  } // end of suSatSolverUnitTestA::unit_test_two_vars_are_next_to_each_other
  
} // end of namespace amsr

// end of suSatSolverUnitTestA.cpp
