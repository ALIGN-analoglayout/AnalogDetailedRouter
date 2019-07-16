// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Apr 17 10:55:33 2018

//! \file   suSatSolverUnitTestE.cpp
//! \brief  A collection of methods of the class suSatSolverUnitTestE.

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

// module include
#include <suSatSolverUnitTestE.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------


  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------


  // ------------------------------------------------------------
  // -
  // --- Public static methods
  // -
  // ------------------------------------------------------------

  //
  void suSatSolverUnitTestE::run_unit_test ()
  {
    sutype::dcoordpairs_t pairs;

    const int numP = 5; // players
    const int numG = 5; // num games
  
    for (int i=1; i <= numP; ++i) {
      for (int k=i+1; k <= numP; ++k) {
        int player1 = i;
        int player2 = k;
        pairs.push_back (std::make_pair (player1, player2));
      }
    }
  
    std::vector <std::vector<int> > games;

    std::map<int,unsigned> numDummies;

    while (!pairs.empty()) {

      SUASSERT (pairs.size() % 2 == 0, "");
    
      const sutype::dcoordpair_t & pair1 = pairs[0];
    
      int player1 = pair1.first;
      int player2 = pair1.second;

      SUASSERT (player1 != player2, "");

      bool found = false;

      for (unsigned i=1; i < pairs.size(); ++i) {
      
        const sutype::dcoordpair_t & pair2 = pairs[i];

        int player3 = pair2.first;
        int player4 = pair2.second;

        SUASSERT (player3 != player4, "");

        if (player3 == player1 || player3 == player2) continue;
        if (player4 == player1 || player4 == player2) continue;

        int dummy = 15 - player1 - player2 - player3 - player4;
        SUASSERT (dummy >= 1 && dummy <= numP, "");
        SUASSERT (dummy != player1, "");
        SUASSERT (dummy != player2, "");
        SUASSERT (dummy != player3, "");
        SUASSERT (dummy != player4, "");

        ++numDummies[dummy];

        std::vector<int> game;
        game.push_back (player1); // n
        game.push_back (player3); // e
        game.push_back (player2); // s
        game.push_back (player4); // w
        game.push_back (dummy);

        //SUINFO(1) << "Found a game: " << suStatic::to_str (game) << std::endl;
      
        for (int g=0; g < numG; ++g) {
          games.push_back (game);
        }

        pairs[i] = pairs.back();
        pairs.pop_back();

        pairs[0] = pairs.back();
        pairs.pop_back();

        found = true;
        break;
      }

      if (!found) {
        SUINFO(1) << "Could not find a game for player1=" << player1 << "; player2=" << player2 << std::endl;
        SUINFO(1) << "Remained " << pairs.size() << " pairs:" << std::endl;
        for (unsigned i=0; i < pairs.size(); ++i) {
          const sutype::dcoordpair_t & pair2 = pairs[i];
          int player3 = pair2.first;
          int player4 = pair2.second;
          SUINFO(1) << "  Remained player3=" << player3 << "; player4=" << player4 << std::endl;
        }
        SUASSERT (found, "");
      }
    }
  
    for (const auto & iter : numDummies) {

      int dummy = iter.first;
      unsigned count = iter.second;

      SUINFO(1) << "dummy=" << dummy << "; count=" << count << std::endl;
    }

    const unsigned N = 0;
    const unsigned E = 1;
    const unsigned S = 2;
    const unsigned W = 3;
    const unsigned D = 4;
    const unsigned L = 5; // dealear
  
    char indexToDealerChar [4] = {'N', 'E', 'S', 'W'};

    SUASSERT (!games.empty(), "");
    std::vector <std::vector<int> > sortedgames;
    sortedgames.push_back (games.back());
    games.pop_back ();
  
    while (!games.empty()) {

      SUASSERT (!sortedgames.empty(), "");
      const std::vector<int> & lastgame = sortedgames.back();

      int lastdummy = lastgame [D];
    
      int nextdummy = lastdummy + 1;
      if (nextdummy > numP) nextdummy = 1;

      bool found = false;
    
      for (unsigned i=0; i < games.size(); ++i) {

        const std::vector<int> & game = games[i];
        SUASSERT (game.size() == 5, "");
        int d = game[D];
        if (d != nextdummy) continue;

        //SUINFO(1) << suStatic::to_str (game) << std::endl;

        sortedgames.push_back (game);

        games[i] = games.back();
        games.pop_back ();
        found = true;
        break;
      }

      if (!found) {
        SUINFO(1) << "nextdummy=" << nextdummy << std::endl;
        for (unsigned i=0; i < games.size(); ++i) {
          SUINFO(1) << suStatic::to_str (games[i]) << std::endl;
        }
        SUASSERT (found, "");
      }
    }

    unsigned currDealer = W;

    for (unsigned i=0; i < sortedgames.size(); ++i) {

      std::vector<int> & game = sortedgames[i];
      SUASSERT (game.size() == 5, "");
    
      ++currDealer;
      if (currDealer > W) currDealer = N;
    
      // add dealer
      game.push_back (currDealer);
    
      SUASSERT (game.size() == 6, "");
      SUASSERT (sortedgames[i].size() == 6, "");
      SUASSERT (sortedgames[i].back() == (int)currDealer, "");
    }
  
    std::vector <std::vector<int> > round;

    for (const auto & iter1 : sortedgames) {

      const std::vector<int> & game0 = iter1;
      SUASSERT (game0.size() == 6, "");
    
      round.push_back (game0);
    
      if (round.size() != 5) continue;
    
      SUASSERT (round.size() == 5, "");

      suSatSolverUnitTestE::optimize_round_ (round);
    
      for (const auto & iter2 : round) {

        const std::vector<int> & game = iter2;
        SUASSERT (game.size() == 6, "");
    
        int n = game[N]; int e = game[E]; int s = game[S]; int w = game[W]; int d = game[D]; int l = game[L];
      
        char dealerChar = indexToDealerChar [l];
      
        SUOUT(1) << "AAAA\t" << dealerChar << "\t" << n << "\t" << e << "\t" << s << "\t" << w << "\t" << d << std::endl;
      }
    
      round.clear ();
    }
  
    SUABORT;
  
  } // end of suSatSolverUnitTestE::run_unit_test

  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------


  // ------------------------------------------------------------
  // -
  // --- Private static methods
  // -
  // ------------------------------------------------------------

  // static
  void suSatSolverUnitTestE::optimize_round_ (std::vector <std::vector<int> > & round)
  {
    SUASSERT (round.size() == 5, "");

    const unsigned N = 0;
    const unsigned E = 1;
    const unsigned S = 2;
    const unsigned W = 3;
    const unsigned D = 4;
    const unsigned L = 5; // dealear

    std::vector <std::vector <std::vector<int> > > roundGameOptions;

    for (const auto & iter : round) {

      const std::vector<int> & game = iter;
    
      int n = game[N];
      int e = game[E];
      int s = game[S];
      int w = game[W];
      int d = game[D];
      int l = game[L];

      SUASSERT (d == 15 - n - e - s - w, "");
      SUASSERT (l == N || l == E || l == S || l == W, "");

      std::vector <std::vector<int> > gameOptions; 

      std::vector<int> game00 = game;
      std::vector<int> game01 = game;
      std::vector<int> game10 = game;
      std::vector<int> game11 = game;
    
      // game00
      if (1) {
        // don't change anything
      }

      // game01
      if (1) {
        game01[N] = s;
        game01[S] = n;
      }
    
      // game10
      if (1) {
        game10[E] = w;
        game10[W] = e;
      }

      // game11
      if (1) {
        game11[N] = s;
        game11[S] = n;
        game11[E] = w;
        game11[W] = e;
      }
    
      gameOptions.push_back (game00);
      gameOptions.push_back (game01);
      gameOptions.push_back (game10);
      gameOptions.push_back (game11);    

      roundGameOptions.push_back (gameOptions);
    }

    std::vector <std::vector<int> > roundOptions;

    suSatSolverUnitTestE::enumerate_round_options_ (roundOptions, roundGameOptions, 0);
  
    suSatSolverUnitTestE::prune_round_options_ (roundOptions);
    SUASSERT (!roundOptions.empty(), "");
  
    SUINFO(1) << "Found " << roundOptions.size() << " round options." << std::endl;
    SUASSERT (roundOptions.size() == 1, "");

    // convert the best option back to the round
    const std::vector<int> & roundOption = roundOptions.front();
    
    const unsigned gamelength = 6;
    
    SUASSERT (roundOption.size() % gamelength == 0, "");
    SUASSERT (roundOption.size() / gamelength == 5, "");

    // clear input round
    round.clear ();
  
    // fill the round back
    for (unsigned i=0; i < roundOption.size(); i += gamelength) {
      
      int n = roundOption [i+N];
      int e = roundOption [i+E];
      int s = roundOption [i+S];
      int w = roundOption [i+W];
      int d = roundOption [i+D];
      int l = roundOption [i+L];

      std::vector<int> game;

      game.push_back (n);
      game.push_back (e);
      game.push_back (s);
      game.push_back (w);
      game.push_back (d);
      game.push_back (l);
     
      round.push_back (game);
    }
  
  } // end of suSatSolverUnitTestE::optimize_round_

  // static
  void suSatSolverUnitTestE::enumerate_round_options_ (std::vector <std::vector<int> > & roundOptions, // to store
                                                       const std::vector <std::vector <std::vector<int> > > & roundGameOptions, // to enumerate
                                                       const unsigned index)
  {
    SUASSERT (index < roundGameOptions.size(), "");
    SUASSERT (roundGameOptions.size() == 5, "");

    const std::vector <std::vector<int> > & gameOptions = roundGameOptions[index];
    SUASSERT (gameOptions.size() == 4, "");

    // back up
    std::vector <std::vector<int> > roundOptions0 = roundOptions; // hard copy
    roundOptions.clear ();

    // init
    if (roundOptions0.empty()) {

      for (const auto & iter1 : gameOptions) {
        const std::vector<int> & game = iter1;
        roundOptions.push_back (game);
      }
    }

    // append
    else {

      for (const auto & iter0 : roundOptions0) {

        const std::vector<int> & roundOption = iter0;

        for (const auto & iter1 : gameOptions) {
        
          const std::vector<int> & game = iter1;
          std::vector<int> newRoundOption = roundOption; // hard copy

          for (const auto & iter2 : game) {

            int player = iter2;
            newRoundOption.push_back (player);
          }

          roundOptions.push_back (newRoundOption);
        }
      }
    }
  
    if (index+1 < roundGameOptions.size()) {
      suSatSolverUnitTestE::enumerate_round_options_ (roundOptions, roundGameOptions, index+1);
    }
      
  } // end of suSatSolverUnitTestE::enumerate_round_options_

  // static
  void suSatSolverUnitTestE::prune_round_options_ (std::vector <std::vector<int> > & roundOptions)
  {
    unsigned counter = 0;

    int bestIndex   = -1;
    int bestPenalty = 0;

    for (unsigned i=0; i < roundOptions.size(); ++i) {

      const std::vector<int> & roundOption = roundOptions[i];

      int penalty = suSatSolverUnitTestE::round_option_is_legal_ (roundOption);
      if (penalty < 0) continue; // completely illegal
    
      roundOptions[counter] = roundOption;

      if (bestIndex < 0 || penalty < bestPenalty) {
        bestIndex = counter;
        bestPenalty = penalty;
      }

      ++counter;
    }

    if (counter != roundOptions.size())
      roundOptions.resize (counter);

    SUASSERT (!roundOptions.empty(), "");
    SUASSERT (bestIndex >= 0 && bestIndex < (int)roundOptions.size(), "");

    SUINFO(1) << "Best penalty: " << bestPenalty << std::endl;

    roundOptions[0] = roundOptions[bestIndex];
    roundOptions.resize (1);
    
  } // end of suSatSolverUnitTestE::prune_round_options_

  // return -1 if illegal
  // return 0 if no penalty
  int suSatSolverUnitTestE::round_option_is_legal_ (const std::vector<int> & roundOption)
  {
    const unsigned N = 0;
    const unsigned E = 1;
    const unsigned S = 2;
    const unsigned W = 3;
    const unsigned D = 4;
    const unsigned L = 5; // dealear

    int dealer0 [6] = {0, 0, 0, 0, 0, 0}; // dealer
    int dealer1 [6] = {0, 0, 0, 0, 0, 0}; // after dealer
    int dealer2 [6] = {0, 0, 0, 0, 0, 0}; // parthner of the dealer
    int dealer3 [6] = {0, 0, 0, 0, 0, 0}; // before dealer
    int dummy   [6] = {0, 0, 0, 0, 0, 0}; // dummy

    const unsigned gamelength = 6;
  
    SUASSERT (roundOption.size() % gamelength == 0, "");
    SUASSERT (roundOption.size() / gamelength == 5, "");

    int penalty = 0;
  
    for (unsigned i=0; i < roundOption.size(); i += gamelength) {

      int n = roundOption [i+N];
      int e = roundOption [i+E];
      int s = roundOption [i+S];
      int w = roundOption [i+W];
      int d = roundOption [i+D];
      int l = roundOption [i+L];

      SUASSERT (l == N || l == E || l == S || l == W, "");
    
      SUASSERT (n >= 1 && n <= 5, "");
      SUASSERT (e >= 1 && e <= 5, "");
      SUASSERT (w >= 1 && w <= 5, "");
      SUASSERT (s >= 1 && s <= 5, "");
      SUASSERT (d == 15 - n - e - s - w, "");
      SUASSERT (n != e && n != s && n != w && n != d, "");
      SUASSERT (e != s && e != w && e != d, "");
      SUASSERT (s != w && s != d, "");
      SUASSERT (w != d, "");

      unsigned d0 = l;
      unsigned d1 = d0 + 1; if (d1 > W) d1 = N;
      unsigned d2 = d1 + 1; if (d2 > W) d2 = N;
      unsigned d3 = d2 + 1; if (d3 > W) d3 = N;

      ++dealer0 [roundOption [i + d0]];
      ++dealer1 [roundOption [i + d1]];
      ++dealer2 [roundOption [i + d2]];
      ++dealer3 [roundOption [i + d3]];
    
      ++dummy [d];

      int nextDummy = d+1;
      if (nextDummy > 5) nextDummy = 1;

      int currDealer = roundOption [i + d0];

      if (currDealer != nextDummy)
        ++penalty;
    }

    SUASSERT (dummy[0] == 0, "");
    SUASSERT (dummy[1] == 1, "");
    SUASSERT (dummy[2] == 1, "");
    SUASSERT (dummy[3] == 1, "");
    SUASSERT (dummy[4] == 1, "");
    SUASSERT (dummy[5] == 1, "");

    SUASSERT (dealer0[0] == 0, "");
    SUASSERT (dealer1[0] == 0, "");
    SUASSERT (dealer2[0] == 0, "");
    SUASSERT (dealer3[0] == 0, "");

    for (unsigned player = 1; player <= 5; ++player) {  
      if (dealer0[player] != 1) return -1; // every player must be a dealer only once and only once
    }

    // calculate another penalty
    if (0) {
      penalty = 0;
      for (unsigned player = 1; player <= 5; ++player) {
        penalty += (dealer0[player] - 1) ^ 2;
        penalty += (dealer1[player] - 1) ^ 2;
        penalty += (dealer2[player] - 1) ^ 2;
        penalty += (dealer3[player] - 1) ^ 2;
      }
    }
  
    return penalty;
  
  } // end of suSatSolverUnitTestE::round_option_is_legal_
  
  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------


} // end of namespace amsr

// end of suSatSolverUnitTestE.cpp
