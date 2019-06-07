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
//! \date   Wed Sep 27 14:02:58 2017

//! \file   suMain.cpp
//! \brief  A collection of methods of the class suMain.

// basic includes
#include <time.h>

// std includes
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suCellManager.h>
#include <suCellMaster.h>
#include <suClauseBank.h>
#include <suCommandLineManager.h>
#include <suGeneratorManager.h>
#include <suGlobalRouter.h>
#include <suErrorManager.h>
#include <suLayerManager.h>
#include <suLayoutNode.h>
#include <suMetalTemplateManager.h>
#include <suOptionManager.h>
#include <suPatternGenerator.h>
#include <suPatternManager.h>
#include <suRectangle.h>
#include <suRuleManager.h>
#include <suSatSolverUnitTestE.h>
#include <suSatSolverWrapper.h>
#include <suSatRouter.h>
#include <suStatic.h>
#include <suSVI.h>
#include <suTimeManager.h>
#include <suWireManager.h>

//#define _BUILD_TIME_IN_SECONDS_ 1537490390

using namespace amsr;

class testclass {

private:

  int _x;

public:

  static unsigned _numcalls;

public:

  testclass (int v) : _x (v)
  {
    ++testclass::_numcalls;
  }

  inline int x () const { return _x; }
  
  inline bool operator == (const testclass & b)
    const
  {
    return (_x == b._x);
  }
  
  inline bool operator != (const testclass & b)
    const
  {
    return !(*this == b);
  }

  inline bool operator <= (const testclass & b)
    const
  {
    return (_x <= b._x);
  }

  inline bool operator < (const testclass & b)
    const
  {
    return (_x < b._x);
  }

  inline bool operator >= (const testclass & b)
    const
  {
    return (_x >= b._x);
  }

  inline bool operator > (const testclass & b)
    const
  {
    return (_x > b._x);
  }
  
  inline testclass operator ^ (const testclass & b)
    const
  {
    return testclass (_x ^ b._x);
  }

  inline bool operator += (const testclass & b)
  {
    return (_x += b._x);
  }
};

unsigned testclass::_numcalls = 0;

//#define TESTTYPE int
#define TESTTYPE testclass

inline int to_int (int v) { return v; }
inline int to_int (const testclass & v) { return v.x(); }

void test_class_ ()
{
  srand (time(NULL));

  TESTTYPE a = rand () % 100000;
  TESTTYPE b = rand () % 100000;
  
  int c1 = 0;
  
  const unsigned maxcounter1 = 1000000000 + (rand() % 10);
  const unsigned maxcounter2 = 1000;

  unsigned counter1 = 0;
  unsigned counter2 = 0;
  
  while (1) {
    
    ++counter1;

    c1 += to_int (a ^ b);
    
    if (counter1 >= maxcounter1) {
      counter1 = 0;
      ++counter2;
      if (counter2 >= maxcounter2) {
        break;
      }
    }
  }

  std::cout << "c1=" << c1 << std::endl;
  std::cout << "numcalls=" << testclass::_numcalls << std::endl;
  
} // end of test_class_

void calibration_ ()
{
  SUINFO(1) << "Started calibration." << std::endl;

  //srand(time(NULL));

  double totcputime = 0.0;
  double avecputime = 0.0;
  
  for (sutype::uvi_t i = 1; i <= 1; ++i) {
  
    const double cputime0 = suTimeManager::instance()->get_cpu_time();

    // ~5 minutes
    const sutype::uvi_t minutes5target = 217;
    
    const sutype::uvi_t maxnumber = minutes5target * 1000 * 1000;
    const sutype::svi_t value1 = 1000;
  
    sutype::uvi_t number = 0;
    sutype::dcoords_t statistic (value1 * value1, 0);
  
    while (number < maxnumber) {

      ++number;
    
      sutype::svi_t value2 = (rand() % value1) * (rand() % value1);

      SUASSERT (value2 >= 0 && value2 < (sutype::svi_t)statistic.size(), "");
    
      ++statistic[value2];
    }

    const double cputime1 = suTimeManager::instance()->get_cpu_time();

    const double curcputime = (cputime1 - cputime0);
    
    totcputime += curcputime;

    double prevavecputime = avecputime;

    avecputime = totcputime / (double)i;

    double deltaavecputime = (avecputime - prevavecputime);

    SUINFO(1) << "i=" << i << "; cur=" << curcputime << "; ave=" << avecputime << "; delta=" << deltaavecputime << std::endl;
  }

  suStatic::increment_elapsed_time (sutype::ts_calibration_time, totcputime);
  
  SUINFO(1) << "Ended calibration." << std::endl;
  
} // end of calibration_

void free_memory_ ();

bool alive_ ()
{  
#ifdef _BUILD_SECONDS_

  // the time elapsed, in seconds, since January 1, 1970 0:00 UTC
  time_t now = time(NULL);
  
  time_t build = _BUILD_SECONDS_;
  
  if (now < build) {
    return false;
  }

  const time_t numdays = 60;
  
  if (now > build + numdays * 24 * 60) {
    return false;
  }
  
  //SUINFO(1) << "Build: " << _BUILD_SECONDS_ << std::endl;
  //SUINFO(1) << "Now:   " << now << std::endl;

#endif
  
  return true;
  
} // end of alive_

void print_layer_file_ ()
{
  int level = 3;

  for (int index=0; index <= 20; ++index) {
    
    SUOUT(1)
      << "Layer name=metal" << index << " pgd=" << ((index % 2 == 0) ? "hor" : "ver") << " level=" << level << " {"
      << std::endl;
    
    SUOUT(1) << std::endl;

    SUOUT(1) << "  " << "Type value=wire" << std::endl;
    SUOUT(1) << "  " << "Type value=metal" << std::endl;

    SUOUT(1) << std::endl;
     
    SUOUT(1) << "  " << "ElectricallyConnected layer=via" << (index-1) << std::endl;
    SUOUT(1) << "  " << "ElectricallyConnected layer=via" << (index+0) << std::endl;

    SUOUT(1) << "}" << std::endl;
    SUOUT(1) << std::endl;

    ++level;

    SUOUT(1)
      << "Layer name=via" << index << " level=" << level << " {"
      << std::endl;

    SUOUT(1) << std::endl;

    SUOUT(1) << "  " << "Type value=via" << std::endl;
    SUOUT(1) << std::endl;
    
    SUOUT(1) << "  " << "ElectricallyConnected layer=metal" << (index+0) << std::endl;
    SUOUT(1) << "  " << "ElectricallyConnected layer=metal" << (index+1) << std::endl;

    SUOUT(1) << "}" << std::endl;
    SUOUT(1) << std::endl;

    ++level;
  } 

  SUABORT;
  
} // end of print_layer_file_

void print_build_details_ ()
{
  const int ll = 2; // log level

  SUINFO(ll) << "###########################################################################" << std::endl;
  SUINFO(ll) << "# Build highlights:" << std::endl;
  
  // user
  SUINFO(ll) << "# User     : ";
#ifdef _BUILD_USER_
  SUOUT(ll) << _BUILD_USER_;
#else
  SUOUT(ll) << "<unknown>";
#endif
  SUOUT(ll) << std::endl;

  // date
  SUINFO(ll) << "# Date     : ";
#ifdef _BUILD_DATE_
  SUOUT(ll) << _BUILD_DATE_;
#else
  SUOUT(ll) << "<unknown>";
#endif
  SUOUT(ll) << std::endl;

  // host
  SUINFO(ll) << "# Host     : ";
#ifdef _BUILD_HOST_
  SUOUT(ll) << _BUILD_HOST_;
#else
  SUOUT(ll) << "<unknown>";
#endif
  SUOUT(ll) << std::endl; 

  // mode
  SUINFO(ll) << "# Mode     : ";
#ifdef _BUILD_MODE_
  SUOUT(ll) << _BUILD_MODE_;
#else
  SUOUT(ll) << "<unknown>";
#endif
  SUOUT(ll) << std::endl; 

  // revision
  SUINFO(ll) << "# Revision :";
#ifdef _BUILD_REVISION_
  SUOUT(ll) << _BUILD_REVISION_;
#else
  SUOUT(ll) << " <unknown>";
#endif
  SUOUT(ll) << std::endl;

  // compiler
  SUINFO(ll) << "# Compiler : ";
#ifdef _BUILD_COMPILER_
  SUOUT(ll) << _BUILD_COMPILER_;
#else
  SUOUT(ll) << "<unknown>";
#endif
  SUOUT(ll) << std::endl;

  SUINFO(ll) << "###########################################################################" << std::endl;
  
} // end of print_build_details_

int main (int argc, char ** argv)
{
  suRectangle::sanity_check ();
  suStatic::sanity_check ();

  print_build_details_ ();
  //SUABORT;

  if (0) {
    if (!alive_()) {
      SUFATAL("Release has expired") << std::endl;
      return 0;
    }
  }
  
  //print_layer_file_ ();
  
  SUINFO(2) << "Execution started." << std::endl;
  //SUASSERT (false, "Aborted by Nikolai. No panic.");
  
  bool ok = true;
  sutype::status_t status = sutype::status_ok;

  //suSatSolverUnitTestE::run_unit_test ();

//   std::vector<unsigned> tmplist;
//   unsigned counter = 0;
//   while (1) {
//     ++counter;
//     tmplist.push_back (counter);
//   }

//   for (int absindex = -10; absindex <= 10; ++absindex) {

//     const sutype::svi_t numwidths = 4;

//     sutype::svi_t numPeriods = (absindex / numwidths);
//     sutype::svi_t refindex   = (absindex % numwidths);

//     if (refindex < 0) {
//       refindex += numwidths;
//       --numPeriods;
//       SUASSERT (refindex >= 0 && refindex < numwidths, "");
//     }

//     SUASSERT (numPeriods * numwidths + refindex == absindex, "");
    
//     SUINFO(1) << "absindex=" << absindex << "; numwidths=" << numwidths << "; numPeriods=" << numPeriods << "; refindex=" << refindex << std::endl;
//   }
//   SUASSERT (false, "Aborted by Nikolai");
  
  suTimeManager::create_instance ();

  //calibration_ ();

//   if (1) {
//     test_class_ ();
//     free_memory_ ();
//     SUINFO(1) << "Execution ended normally." << std::endl;
//     return 0;
//   }

  sutype::strings_t commandlineoptions;
  for (int i=1; i < argc; ++i) {
    commandlineoptions.push_back (std::string (argv[i]));
  }

  suCommandLineManager::instance()->parse_options (commandlineoptions);
  suOptionManager::instance()->read_run_file (suCommandLineManager::instance()->runFile());
  
  // read dimacs file, solve, and exit
#ifndef _RELEASE_BUILD_
  if (!suOptionManager::instance()->get_string_option("input_dimacs_file").empty()) {
    
    suSatSolverWrapper::create_instance (false); // do not emit constants
    suSatSolverWrapper::instance()->read_and_emit_dimacs_file (suOptionManager::instance()->get_string_option("input_dimacs_file"));
    const double cputime1 = suTimeManager::instance()->get_cpu_time();
    bool ok = suSatSolverWrapper::instance()->solve_the_problem ();
    const double cputime2 = suTimeManager::instance()->get_cpu_time();
    SUINFO(1) << "Dimacs solving elapsed " << (cputime2 - cputime1) << " sec." << std::endl;
    if (ok) {
      SUINFO(1) << "Solved." << std::endl;
    }
    else {
      SUINFO(1) << "UNSAT." << std::endl;
    }
    SUASSERT (ok, "UNSAT is unexpected.");

    free_memory_ ();
    SUINFO(1) << "Execution ended normally." << std::endl;
    return 0;
  }
#endif // _RELEASE_BUILD_

  //
  suSatSolverWrapper::create_instance ();
  
  // read all external files at once
  suGlobalRouter::instance()->read_global_routing ();
  suWireManager::instance()->reserve_gid_for_wires (suGlobalRouter::instance()->get_terminal_gids());
  suOptionManager::instance()->read_external_files ();
  suCellManager::instance()->detect_nets_to_route ();
  suCellManager::instance()->fix_conflicts ();
  suGlobalRouter::instance()->parse_global_routing ();
  
  //
  suMetalTemplateManager::instance()->print_metal_templates();
  
  // main routing flow
  do {

    // create global routing
    suGlobalRouter::instance()->create_global_routing_grid ();
    
    // create metal template instances
    suMetalTemplateManager::instance()->create_metal_template_instances ();
    
    bool mtisAreGood = suMetalTemplateManager::instance()->check_regions_of_metal_template_instances ();
    mtisAreGood = true; // do not abort the flow even MTIs are incorrect
    
    if (!mtisAreGood) {
      SUFATAL("Metal template instances are incorrect. The flow was aborted.") << std::endl;
      status = sutype::status_fail;
    }
    
    if (0 && suOptionManager::instance()->get_boolean_option ("create_fake_metal_template_instances")) {
      suSatRouter::instance()->create_fake_metal_template_instances ();
      break;
    }
    
    if (!mtisAreGood) {
      break;
    }
    
#ifndef _RELEASE_BUILD_
    if (!suOptionManager::instance()->get_string_option ("generate_pattern_file").empty()) {
      suPatternGenerator::instance()->generate_pattern_file (suOptionManager::instance()->get_string_option ("generate_pattern_file"));
      break;
    }
#endif    

#ifndef _RELEASE_BUILD_
    suGlobalRouter::instance()->dump_global_routing ("out/" + suCellManager::instance()->topCellMaster()->name() + "_gr.txt");
#endif // _RELEASE_BUILD_

    //
    suMetalTemplateManager::instance()->check_metal_template_instances(); // just print issues
    
    //
    ok = suSatRouter::instance()->check_input_wires ();

    if (!ok) {
      SUFATAL("Input wires are incorrect. The flow was aborted.") << std::endl;
      status = sutype::status_fail;
      break;
    }
    
    suSatRouter::instance()->prepare_input_wires_and_create_connected_entities ();

    if (suOptionManager::instance()->get_boolean_option ("create_fake_metal_template_instances")) {
      suSatRouter::instance()->create_fake_metal_template_instances ();
      break;
    }

#ifndef _RELEASE_BUILD_
    suSatRouter::instance()->dump_connected_entities ("out/" + suCellManager::instance()->topCellMaster()->name() + "_ce.txt");
#endif // _RELEASE_BUILD_

    //suSatRouter::instance()->create_evident_preroutes (); // I don't know why but it increases antenna removal time in several times
    
    // problem formulation
    //suSatRouter::instance()->create_mesh_routing_for_connected_entities (); // became obsolete; need review; moved to suBackUp.cpp
    suSatRouter::instance()->convert_global_routing_to_trunks ();

    //SUASSERT (false, "");

    if (!suOptionManager::instance()->get_boolean_option ("route")) break;

    // create ties and routes to connect
    suSatRouter::instance()->create_ties ();

    if (suOptionManager::instance()->get_boolean_option ("pin_checker_mode")) {
      suSatRouter::instance()->create_global_routing ();
    }

    suSatRouter::instance()->create_routes ();

    //SUABORT;

    suSatRouter::instance()->prune_ties ();
    suSatRouter::instance()->prune_redundant_ties ();
    
    //SUABORT;

    // formulate the boolean problem
    suSatRouter::instance()->emit_connected_entities ();
    suSatRouter::instance()->emit_ties_and_routes ();
    suSatRouter::instance()->emit_trunk_routing ();
    suSatRouter::instance()->emit_metal_fill_rules ();
    suSatRouter::instance()->emit_conflicts ();
    suSatRouter::instance()->emit_patterns ();
    //suSatRouter::instance()->emit_tie_triangles (); // an optional constraint; just to improve routing quality and to reduce redundant routes
    suSatRouter::instance()->emit_same_width_for_trunks ();
    
    // initial solving; only hard rules; no soft rules
    suSatRouter::instance()->post_routing_delete_useless_generator_instances (false);
    ok = suSatRouter::instance()->solve_the_problem ();
    if (!ok) break;
    
    // optimization of soft constraints
    suSatRouter::instance()->optimize_routing ();
    suSatRouter::instance()->post_routing_fix_trunks (); // it works only if you minimize explicitly the number of tracks for this layer
    suSatRouter::instance()->post_routing_delete_useless_generator_instances (true);
    suSatRouter::instance()->post_routing_fix_routes (true);
    suSatRouter::instance()->eliminate_antennas ();
    
    // post-routing wrap-up
    suSatRouter::instance()->print_applied_ties ();
    //suSatRouter::instance()->print_applied_routes ();
    suSatRouter::instance()->apply_solution ();
    suSatRouter::instance()->report_disconnected_entities ();
    
  } while (false);

  // add fake wires to visualize global routes
  if (suOptionManager::instance()->get_boolean_option ("create_fake_global_routes")) {
    suGlobalRouter::instance()->create_fake_global_routes ();
  }

  // add fake wires to visualize connected entities
  if (suOptionManager::instance()->get_boolean_option ("create_fake_connected_entities")) {
    suSatRouter::instance()->create_fake_connected_entities ();
  }

  // add fake wires to visualize applied ties
  if (suOptionManager::instance()->get_boolean_option ("create_fake_ties")) {
    suSatRouter::instance()->create_fake_ties ();
  }

  // add fake wires to visualize applied ties
  if (suOptionManager::instance()->get_boolean_option ("create_fake_sat_wires")) {
    suSatRouter::instance()->create_fake_sat_wires ();
  }
  
  if (suOptionManager::instance()->get_boolean_option ("create_fake_ogd_grids")) {
    suSatRouter::instance()->create_fake_metal_template_instances (false, true, false);
  }
  
  if (suOptionManager::instance()->get_boolean_option ("create_fake_pgd_grids")) {
    suSatRouter::instance()->create_fake_metal_template_instances (false, false, true);
  }
  
#ifndef _RELEASE_BUILD_
  suSVI::instance()->dump_xml_file ("out/" + suCellManager::instance()->topCellMaster()->name() + ".xml");
#endif // _RELEASE_BUILD_
  
  //#ifndef _RELEASE_BUILD_
  suCellManager::instance()->dump_out_file ("out/debug_" + suCellManager::instance()->topCellMaster()->name() + ".lgf", true);
  //#endif // _RELEASE_BUILD_
  
  suCellManager::instance()->dump_out_file ("out/" + suCellManager::instance()->topCellMaster()->name() + ".lgf");
  
  suErrorManager::instance()->cellName (suCellManager::instance()->topCellMaster()->name());
  suErrorManager::instance()->dump_error_file ("out");
  
  free_memory_ ();

  if (status != sutype::status_ok) {
    SUFATAL("Found a fatal error during exution.") << std::endl;
    return 1;
  }
  
  SUINFO(2) << "Execution ended normally." << std::endl;
  return 0;
  
} // end of main

void free_memory_ ()
{
  suClauseBank::delete_clause_pool ();
  suCommandLineManager::delete_instance ();
  suOptionManager::delete_instance ();
  suCellManager::delete_instance ();
  suSatRouter::delete_instance ();
  suMetalTemplateManager::delete_instance ();
  suGlobalRouter::delete_instance ();
  suGeneratorManager::delete_instance ();
  suPatternManager::delete_instance ();
  suRuleManager::delete_instance ();
  suWireManager::delete_instance ();
  suLayoutNode::delete_leaves();
  suLayoutNode::delete_funcs();
  suLayerManager::delete_instance ();
  suSatSolverWrapper::delete_instance ();
  suSVI::delete_instance ();
  suErrorManager::delete_instance ();

  suStatic::print_statistic_of_log_messages (sutype::le_issue);
  suStatic::print_statistic_of_log_messages (sutype::le_error);
  suStatic::print_statistic_of_log_messages (sutype::le_fatal);

  suStatic::increment_elapsed_time (sutype::ts_total_time, suTimeManager::instance()->get_cpu_time());

  // delete the last manager
  suTimeManager::delete_instance ();

  suStatic::print_elapsed_times ();

  //print_build_details_ ();
  
} // end of free_memory_

// end of suMain.cpp

