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
//! \date   Wed Sep 27 13:52:15 2017

//! \file   suStatic.h
//! \brief  A header of the class suStatic.

#ifndef _suStatic_h_
#define _suStatic_h_

// system includes

// std includes
#include <map>
#include <set>
#include <string>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes

using std::tr1::get;

namespace amsr
{
  // classes
  class suGlobalRoute;
  class suLayoutNode;
  class suLayer;
  class suMetalTemplateInstance;
  class suNet;
  class suPatternInstance;
  class suTie;
  class suWire;

  //! 
  class suStatic
  {

  public:
    
    static bool _allow_final_assert_action;
    

  private:

    static char _defaultDelimeter;
    static char _defaultOpenBracket;
    static char _defaultCloseBracket;
    
    static bool _strings_are_inited;
    
    static std::string _rule_type_2_str [sutype::rt_num_types];
    static std::string _log_event_2_str [sutype::le_num_types];

    static std::map<std::string, sutype::uvi_t> _typesOfLogMessages [sutype::le_num_types];

    static double _elapsedTime [sutype::ts_num_types];
    
  public:

    //! default constructor
    suStatic ()
    {
      init_ ();
      SUASSERT (false, "The method is not expected to be called.");

    } // end of suStatic

    //! copy constructor
    suStatic (const suStatic & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suStatic

    //! assignment operator
    suStatic & operator = (const suStatic & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suStatic ()
    {
      SUASSERT (false, "The method is not expected to be called.");
      
    } // end of ~suStatic
    
  private:

    //! init all class members
    inline void init_ ()
    {
    } // end of init_

    //! copy all class members
    inline void copy_ (const suStatic & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

  public:

    // static accessors
    // 

    static sutype::dcoord_t get_dx  (const sutype::tr_t & v) { return get<0>(v); }
    static sutype::dcoord_t get_dy  (const sutype::tr_t & v) { return get<1>(v); }
    static sutype::ref_t    get_ref (const sutype::tr_t & v) { return get<2>(v); }
    
    // static methods
    //

    //
    static void sanity_check ();

    //
    static void increment_elapsed_time (sutype::time_spend_t t, double value) { suStatic::_elapsedTime[t] += value; }
    
    //
    static inline sutype::unary_sign_t invert_unary_sign (sutype::unary_sign_t v) { return (sutype::unary_sign_t)(1 - (int)v); }

    //
    static inline sutype::bool_t invert_bool_value (sutype::bool_t v) { SUASSERT (v == sutype::bool_true || v == sutype::bool_false, ""); return (sutype::bool_t)(1 - (int)v); }

    //
    static inline sutype::grid_orientation_t invert_gd (sutype::grid_orientation_t v) { return (sutype::grid_orientation_t)(1 - (int)v); }
    
    // static methods: str to type
    //

    //
    static sutype::grid_orientation_t str_2_grid_orientaion (const std::string & str);

    //
    static sutype::layer_type_t str_2_layer_type (const std::string & str);

    
    // static methods: type to str
    //

    //
    static std::string bool_2_str (sutype::bool_t v);
    
    //
    static std::string layer_type_2_str (int v);

    //
    static std::string wire_type_2_str (int v);

    //
    static std::string ref_2_str (sutype::ref_t v);

    //
    static std::string object_2_str (sutype::object_t v);
    
    //
    static std::string grid_orientation_2_str (sutype::grid_orientation_t v);
    
    //
    static std::string opt_mode_2_str (sutype::opt_mode_t v);

    //
    inline static const std::string & rule_type_2_str (sutype::rule_type_t v) { SUASSERT (suStatic::_strings_are_inited, ""); return suStatic::_rule_type_2_str [v]; }
    
    //
    inline static const std::string & log_event_2_str (sutype::log_event_t v) { SUASSERT (suStatic::_strings_are_inited, ""); return suStatic::_log_event_2_str [v]; }

    //
    static std::string tr_2_str (const sutype::tr_t & tr);

    //
    static std::string wire_conflict_2_str (sutype::wire_conflict_t wc);
   
    // static methods: comparison functions
    //

    //
    static bool compare_dcoords (const sutype::dcoords_t & p,
                                 const sutype::dcoords_t & q);

    //
    static bool compare_pairs_00 (const std::pair<std::string,sutype::uvi_t> & p,
                                  const std::pair<std::string,sutype::uvi_t> & q);
    
    //
    static bool compare_layers_by_level (const suLayer * p,
                                         const suLayer * q);

    //
    static bool compare_global_routes (const suGlobalRoute * p,
                                       const suGlobalRoute * q);

    //
    static bool compare_nets_by_id (const suNet * p,
                                    const suNet * q);

    //
    static bool compare_ties_by_id (const suTie * p,
                                    const suTie * q);

    //
    static bool compare_wires_by_level (const suWire * p,
                                        const suWire * q);

    //
    static bool compare_wires_by_id (const suWire * p,
                                     const suWire * q);

    //
    static bool compare_wires_by_gid (const suWire * p,
                                      const suWire * q);
    
    //
    static bool compare_wires_by_edgel (const suWire * p,
                                        const suWire * q);

    //
    static bool compare_wires_by_sidel (const suWire * p,
                                        const suWire * q);
    
    //
    static bool compare_pattern_instances_by_id (const suPatternInstance * p,
                                                 const suPatternInstance * q);

    //
    static bool compare_ver_wires (suWire * p,
                                   suWire * q);

    //
    static bool compare_hor_wires (suWire * p,
                                   suWire * q);

    //
    static bool compare_layout_nodes (const suLayoutNode * p,
                                      const suLayoutNode * q);

    //
    static bool compare_metal_template_instances (const suMetalTemplateInstance * p,
                                                  const suMetalTemplateInstance * q);
   
    // other static methods
    //

    static bool sorted_clause_has_satindex (const sutype::clause_t & clause,
                                            sutype::satindex_t targetsatindex);
    
    static void print_elapsed_times ();

    //
    static void sort_and_leave_unique (std::vector<int> & values);

    //
    static inline sutype::dcoord_t micron_to_udm (float umvalue)
    {
      float tmpvalue = 10000.0 * umvalue;

      tmpvalue += (tmpvalue > 0) ? 0.5 : -0.5;

      sutype::dcoord_t udmvalue = (sutype::dcoord_t)tmpvalue;
      
      //SUASSERT (udmvalue % 10 == 0, "umvalue=" << umvalue << "; udmvalue=" << udmvalue << "; tmpvalue=" << tmpvalue);
      SUASSERT (udmvalue % 5 == 0, "umvalue=" << umvalue << "; udmvalue=" << udmvalue << "; tmpvalue=" << tmpvalue);
      
      return udmvalue;
      
    } // end of micron_to_udm

    //
    static inline float udm_to_micron (sutype::dcoord_t udmvalue)
    {      
      return ((float)udmvalue / 10000.0);
      
    } // emd of udm_to_micron

    //
    inline static void register_a_log_message (sutype::log_event_t le, const std::string & logid) { ++suStatic::_typesOfLogMessages[le][logid]; }

    //
    static void print_statistic_of_log_messages (sutype::log_event_t le);
    
    //
    static void sort_wires (sutype::wires_t & wires,
                            sutype::grid_orientation_t gd);
    
    //
    static void create_parent_dir (const std::string & filename);

    //
    static void create_dir (const std::string & dirname);

    //
    static std::string trim_string (const std::string & str);

    //
    static std::vector<std::string> parse_string (const std::string & str);

    //
    static bool string_matches_one_of_regular_expressions (const std::string & str,
                                                           const sutype::strings_t & regexps);

    //
    static bool string_matches_one_of_regular_expressions (const std::string & str,
                                                           const std::set<std::string> & regexps);

    //
    static bool string_matches_regular_expression (const std::string & str,
                                                   const std::string & regexp);

    //
    static std::string dcoord_to_str (sutype::dcoord_t value);
    
    // core
    static std::string to_str (const sutype::strings_t & strs,
                               const char delimeter = suStatic::_defaultDelimeter,
                               const char openbracket = suStatic::_defaultOpenBracket,
                               const char closebracket = suStatic::_defaultCloseBracket);

    //
    static std::string clause_to_str (const sutype::clause_t & values,
                                      const char delimeter = suStatic::_defaultDelimeter,
                                      const char openbracket = suStatic::_defaultOpenBracket,
                                      const char closebracket = suStatic::_defaultCloseBracket);
    
    //
    static std::string to_str (const sutype::dcoords_t & values,
                               const char delimeter = suStatic::_defaultDelimeter,
                               const char openbracket = suStatic::_defaultOpenBracket,
                               const char closebracket = suStatic::_defaultCloseBracket);

    //
    static std::string to_str (const std::vector<int> & values,
                               const char delimeter = suStatic::_defaultDelimeter,
                               const char openbracket = suStatic::_defaultOpenBracket,
                               const char closebracket = suStatic::_defaultCloseBracket);
    
    //
    static std::string to_str (const std::set<std::string> & values,
                               const char delimeter = suStatic::_defaultDelimeter,
                               const char openbracket = suStatic::_defaultOpenBracket,
                               const char closebracket = suStatic::_defaultCloseBracket);
    
    //
    static std::string to_str (const sutype::wires_t & wires,
                               const char delimeter = suStatic::_defaultDelimeter,
                               const char openbracket = suStatic::_defaultOpenBracket,
                               const char closebracket = suStatic::_defaultCloseBracket);

    //
    static bool wires_are_electrically_connected (const suWire * wire1,
                                                  const suWire * wire2);

    //
    static bool wire_is_redundant (const suWire * tstwire,
                                   const sutype::wires_t & wires);
    
  }; // end of class suStatic

} // end of namespace amsr

#endif // _suStatic_h_

// end of suStatic.h

