// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Wed Sep 27 13:52:25 2017

//! \file   suStatic.cpp
//! \brief  A collection of methods of the class suStatic.

// system includes
#include <assert.h>

// std includes
#include <iostream>
#include <set>
#include <sstream>
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
#include <suGlobalRoute.h>
#include <suErrorManager.h>
#include <suLayer.h>
#include <suLayoutFunc.h>
#include <suLayoutLeaf.h>
#include <suLayoutNode.h>
#include <suMetalTemplate.h>
#include <suMetalTemplateInstance.h>
#include <suNet.h>
#include <suPatternInstance.h>
#include <suTie.h>
#include <suTimeManager.h>
#include <suWire.h>

// module include
#include <suStatic.h>

// ------------------------------------------------------------
// -
// --- Non-class methods
// -
// ------------------------------------------------------------

//
void __do_something_before_assert__ ()
{
  if (amsr::suStatic::_allow_final_assert_action &&
      amsr::suCellManager::instance() &&
      amsr::suCellManager::instance()->topCellMaster()) {
    
    // to avoid endless assert actions if procedures below also assert
    amsr::suStatic::_allow_final_assert_action = false;
    
    amsr::suCellManager::instance()->dump_out_file ("out/" + amsr::suCellManager::instance()->topCellMaster()->name() + ".lgf");
    
    amsr::suErrorManager::instance()->cellName (amsr::suCellManager::instance()->topCellMaster()->name());
    amsr::suErrorManager::instance()->dump_error_file ("out");
  }

  amsr::suStatic::print_statistic_of_log_messages (amsr::sutype::le_issue);
  amsr::suStatic::print_statistic_of_log_messages (amsr::sutype::le_error);
  amsr::suStatic::print_statistic_of_log_messages (amsr::sutype::le_fatal);
  
  amsr::suStatic::increment_elapsed_time (amsr::sutype::ts_total_time, amsr::suTimeManager::instance()->get_cpu_time());
  amsr::suStatic::print_elapsed_times ();
  
} // end of __do_something_before_assert__

//
void __final_assert_action__ ()
{  
  assert (false);
  
} // end of __final_assert_action__

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  char suStatic::_defaultDelimeter    = ',';
  char suStatic::_defaultOpenBracket  = '{';
  char suStatic::_defaultCloseBracket = '}';

  bool suStatic::_strings_are_inited = false;
  bool suStatic::_allow_final_assert_action = true;

  std::string suStatic::_rule_type_2_str [sutype::rt_num_types];
  std::string suStatic::_log_event_2_str [sutype::le_num_types];

  std::map<std::string, sutype::uvi_t> suStatic::_typesOfLogMessages [sutype::le_num_types];

  double suStatic::_elapsedTime [sutype::ts_num_types];
  
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

  // static
  void suStatic::sanity_check ()
  {
    SUASSERT ((int)sutype::unary_sign_not >= 0, "");
    SUASSERT ((int)sutype::unary_sign_just >= 0, "");
    SUASSERT (((int)sutype::unary_sign_not + (int)sutype::unary_sign_just) == 1, "");

    SUASSERT (sutype::wc_undefined == 0,  "");
    SUASSERT (sutype::wc_short     == 1,  "");
    SUASSERT (sutype::wc_minete    == 2,  "");
    SUASSERT (sutype::wc_trunks    == 4,  "");
    SUASSERT (sutype::wc_colors    == 8,  "");
    SUASSERT (sutype::wc_routing   == 16, "");
    SUASSERT (sutype::wc_all_types == 31, "");
    SUASSERT (sutype::wc_num_types == 32, "");

    SUASSERT (sutype::vgl_cut    == 0, "");
    SUASSERT (sutype::vgl_layer1 == 1, "");
    SUASSERT (sutype::vgl_layer2 == 2, "");

    SUASSERT (sutype::go_ver == 0, "");
    SUASSERT (sutype::go_hor == 1, "");
    
    SUASSERT (sutype::gd_pgd == 0, "");
    SUASSERT (sutype::gd_ogd == 1, "");
    
    SUASSERT (sutype::bool_false == 0, "");
    SUASSERT (sutype::bool_true  == 1, "");
    
    // time
    for (sutype::svi_t i=0; i < (sutype::svi_t)sutype::ts_num_types; ++i) {
      suStatic::_elapsedTime[i] = 0.0;
    }
    
    // init
    for (int i=0; i < (int)sutype::rt_num_types; ++i) suStatic::_rule_type_2_str [i] = "";
    for (int i=0; i < (int)sutype::le_num_types; ++i) suStatic::_log_event_2_str [i] = "";
    
    // define
    suStatic::_rule_type_2_str [sutype::rt_minete]    = "minete";
    suStatic::_rule_type_2_str [sutype::rt_minlength] = "minlength";
    suStatic::_rule_type_2_str [sutype::rt_minencl]   = "minencl";

    // define
    suStatic::_log_event_2_str [sutype::le_fatal] = "FATAL";
    suStatic::_log_event_2_str [sutype::le_error] = "ERROR";
    suStatic::_log_event_2_str [sutype::le_issue] = "ISSUE";
    suStatic::_log_event_2_str [sutype::le_label] = "LABEL";
    suStatic::_log_event_2_str [sutype::le_info]  = "I";

    suStatic::_strings_are_inited = true;
    
    // check
    for (int i=0; i < (int)sutype::rt_num_types; ++i) { SUASSERT (!suStatic::_rule_type_2_str[i].empty(), ""); }
    for (int i=0; i < (int)sutype::le_num_types; ++i) { SUASSERT (!suStatic::_log_event_2_str[i].empty(), ""); }
        
  } // end of suStatic::sanity_check

  // static
  sutype::grid_orientation_t suStatic::str_2_grid_orientaion (const std::string & str)
  {
    if      (str.compare ("ver") == 0) return sutype::go_ver;
    else if (str.compare ("hor") == 0) return sutype::go_hor;
    else {
      SUASSERT (false, str);
    }
    
    return sutype::go_ver;
    
  } // end of suStatic::str_2_grid_orientaion

  // static
  std::string suStatic::grid_orientation_2_str (sutype::grid_orientation_t gd)
  {
    if      (gd == sutype::go_ver) return "ver";
    else if (gd == sutype::go_hor) return "hor";
    else {
      SUASSERT (false, gd);
    }
    
    return "";
    
  } // end of suStatic::grid_orientation_2_str
  
  // static
  std::string suStatic::opt_mode_2_str (sutype::opt_mode_t v)
  {
    if      (v == sutype::om_minimize) return "minimize";
    else if (v == sutype::om_maximize) return "maximize";
    else {
      SUASSERT (false, "");
    }

    return "";
    
  } // end of suStatic::opt_mode_2_str

  // static
  std::string suStatic::tr_2_str (const sutype::tr_t & tr)
  {
    std::ostringstream oss;

//     oss
//       << "("
//       << "dx="    << suStatic::get_dx (tr)
//       << "; dy="  << suStatic::get_dy (tr)
//       << "; ref=" << suStatic::ref_2_str (suStatic::get_ref (tr))
//       << ")";

    oss
      << suStatic::get_dx (tr)
      << ":" << suStatic::get_dy (tr)
      << ":" << suStatic::ref_2_str (suStatic::get_ref (tr));
    
    return oss.str();
    
  } // end of suStatic::tr_2_str

  // static 
  std::string suStatic::wire_conflict_2_str (sutype::wire_conflict_t wc)
  {
    if      (wc == sutype::wc_undefined) return "undefined";
    else if (wc == sutype::wc_short)     return "short";
    else if (wc == sutype::wc_minete)    return "minete";
    else if (wc == sutype::wc_trunks)    return "trunks";
    else if (wc == sutype::wc_colors)    return "colors";
    else if (wc == sutype::wc_routing)   return "routing";
    else {
      SUASSERT (false, "undefined wc: " << (int)wc);
    }

    return "";
    
  } // end of wire_conflict_2_str
  
  // static
  std::string suStatic::bool_2_str (sutype::bool_t v)
  {
    if      (v == sutype::bool_false)     return "false";
    else if (v == sutype::bool_true)      return "true";
    else if (v == sutype::bool_undefined) return "undefined";
    else {
      SUASSERT (false, "");
    }

    return "";
    
  } // end of suStatic::bool_2_str

  // static
  sutype::layer_type_t suStatic::str_2_layer_type (const std::string & str)
  {
    if      (str.compare ("wire")      == 0) return sutype::lt_wire;
    else if (str.compare ("via")       == 0) return sutype::lt_via;
    else if (str.compare ("metal")     == 0) return sutype::lt_metal;
    else if (str.compare ("poly")      == 0) return sutype::lt_poly;
    else if (str.compare ("diffusion") == 0) return sutype::lt_diffusion;
    else if (str.compare ("well")      == 0) return sutype::lt_well;
    else {
      SUISSUE("Unexpected layer type") << ": " << str << std::endl;
      //SUASSERT (false, str);
    }
    
    return sutype::lt_undefined;
    
  } // end of suStatic::str_2_layer_type

  // static
  std::string suStatic::layer_type_2_str (int value)
  {
    std::ostringstream oss;

    unsigned num = 0;

    if (value & sutype::lt_wire)      { oss << ((num == 0) ? "{" : ","); ++num; oss << "wire";      }
    if (value & sutype::lt_via)       { oss << ((num == 0) ? "{" : ","); ++num; oss << "via";       }
    if (value & sutype::lt_metal)     { oss << ((num == 0) ? "{" : ","); ++num; oss << "metal";     }
    if (value & sutype::lt_poly)      { oss << ((num == 0) ? "{" : ","); ++num; oss << "poly";      }
    if (value & sutype::lt_diffusion) { oss << ((num == 0) ? "{" : ","); ++num; oss << "diffusion"; }
    if (value & sutype::lt_well)      { oss << ((num == 0) ? "{" : ","); ++num; oss << "well";      }
    
    if (num)
      oss << "}";

    return oss.str();
    
  } // end of suStatic::layer_type_2_str

  // static
  std::string suStatic::ref_2_str (sutype::ref_t v)
  {
    if      (v == sutype::ref_0)  return "REF_0";
    else if (v == sutype::ref_x)  return "REF_X";
    else if (v == sutype::ref_y)  return "REF_Y";
    else if (v == sutype::ref_xy) return "REF_XY";
    else {
      SUASSERT (false, "");
    }
    
    return "";
    
  } // end of suStatic::ref_2_str

  // static
  std::string suStatic::object_2_str (sutype::object_t v)
  {
    if      (v == sutype::obj_none)                return "none";
    else if (v == sutype::obj_wire)                return "wire";
    else if (v == sutype::obj_generator_instance)  return "generator_instance";
    else {
      SUASSERT (false, "");
    }
    
    return "";
    
  } // end of suStatic::object_2_str
  
  //
  std::string suStatic::wire_type_2_str (int value)
  {
    std::ostringstream oss;

    unsigned num = 0;

    if (value & sutype::wt_preroute)  { oss << ((num == 0) ? "" : ","); ++num; oss << "preroute";  }
    if (value & sutype::wt_trunk)     { oss << ((num == 0) ? "" : ","); ++num; oss << "trunk";     }
    if (value & sutype::wt_shunt)     { oss << ((num == 0) ? "" : ","); ++num; oss << "shunt";     }
    if (value & sutype::wt_route)     { oss << ((num == 0) ? "" : ","); ++num; oss << "route";     }
    if (value & sutype::wt_fill)      { oss << ((num == 0) ? "" : ","); ++num; oss << "fill";      }
    if (value & sutype::wt_cut)       { oss << ((num == 0) ? "" : ","); ++num; oss << "cut";       }
    if (value & sutype::wt_enclosure) { oss << ((num == 0) ? "" : ","); ++num; oss << "enclosure"; }
    if (value & sutype::wt_pattern)   { oss << ((num == 0) ? "" : ","); ++num; oss << "pattern";   }
    
    if (num)
      oss << "";
    
    return oss.str();
    
  } // end of suStatic::wire_type_2_str

  // ------------------------------------------------------------
  // -
  // --- Public static methods
  // -
  // ------------------------------------------------------------

  // static
  bool suStatic::compare_dcoords (const sutype::dcoords_t & p,
                                  const sutype::dcoords_t & q)
  {
    SUASSERT (p.size() == q.size(), "");

    sutype::uvi_t numvalues = p.size();

    for (sutype::uvi_t i=0; i < numvalues; ++i) {
      
      sutype::dcoord_t value0 = p[i];
      sutype::dcoord_t value1 = q[i];

      if (value0 != value1)
        return (value0 < value1);
    }

    return false;
    
  } // end of suStatic::compare_dcoords

  // static
  bool suStatic::compare_pairs_00 (const std::pair<std::string,sutype::uvi_t> & p,
                                   const std::pair<std::string,sutype::uvi_t> & q)
  {
    sutype::uvi_t v0 = p.second;
    sutype::uvi_t v1 = q.second;

    if (v0 != v1)
      return v0 > v1;

    const std::string & s0 = p.first;
    const std::string & s1 = q.first;

    int cmpvalue = s0.compare (s1);

    return (cmpvalue <= 0);
    
  } // end of suStatic::compare_pairs_00

  // static
  bool suStatic::compare_layers_by_level (const suLayer * p,
                                          const suLayer * q)
  {
    if (p->level() != q->level())
      return p->level() < q->level();

    return p->pers_id() < q->pers_id();
    
  } // end of suStatic::compare_layers_by_level

  // static
  bool suStatic::compare_global_routes (const suGlobalRoute * p,
                                        const suGlobalRoute * q)
  {
    const suNet * net1 = p->net();
    const suNet * net2 = q->net();

    SUASSERT (net1, "");
    SUASSERT (net2, "");

    if (net1->id() != net2->id())
      return net1->id() < net2->id();

    const suLayer * layer1 = p->layer();
    const suLayer * layer2 = q->layer();

    SUASSERT (layer1, "");
    SUASSERT (layer2, "");

    if (layer1->level() != layer2->level())
      return layer1->level() < layer2->level();

    if (layer1->pers_id() != layer2->pers_id())
      return layer1->pers_id() < layer2->pers_id();

    if (layer1->pgd() != layer2->pgd())
      return layer1->pgd() < layer2->pgd();

    const suRectangle & bbox1 = p->bbox();
    const suRectangle & bbox2 = q->bbox();
    
    if (layer1->pgd() == sutype::go_ver) {
      
      if (bbox1.xl() != bbox2.xl())
        return bbox1.xl() < bbox2.xl();
      
      if (bbox1.yl() != bbox2.yl())
        return bbox1.yl() < bbox2.yl();
    }
    else {

      if (bbox1.yl() != bbox2.yl())
        return bbox1.yl() < bbox2.yl();

      if (bbox1.xl() != bbox2.xl())
        return bbox1.xl() < bbox2.xl();
    }

    return false;
    
  } // end of suStatic::compare_global_routes

  // static 
  bool suStatic::compare_nets_by_id (const suNet * p,
                                     const suNet * q)
  {
    return p->id() < q->id();
    
  } // end of suStatic::compare_nets_by_id

  // static 
  bool suStatic::compare_ties_by_id (const suTie * p,
                                     const suTie * q)
  {
    return p->id() < q->id();
    
  } // end of suStatic::compare_ties_by_id

  // static
  bool suStatic::compare_wires_by_level (const suWire * p,
                                         const suWire * q)
  {
    int level1 = p->layer()->level();
    int level2 = q->layer()->level();

    if (level1 != level2)
      return (level1 < level2);
    
    return p->id() < q->id();
    
  } // end of suStatic::compare_wires_by_level

  // static
  bool suStatic::compare_wires_by_id (const suWire * p,
                                      const suWire * q)
  {
    return p->id() < q->id();
    
  } // end of suStatic::compare_wires_by_id
  
  // static
  bool suStatic::compare_wires_by_gid (const suWire * p,
                                       const suWire * q)
  {
    if (p->gid() != q->gid())
      return p->gid() < q->gid();

    return p->id() < q->id();
    
  } // end of suStatic::compare_wires_by_gid

  // static
  bool suStatic::compare_wires_by_edgel (const suWire * p,
                                         const suWire * q)
  {
    sutype::dcoord_t edgel1 = p->edgel();
    sutype::dcoord_t edgel2 = q->edgel();

    if (edgel1 != edgel2)
      return (edgel1 < edgel2);

    return p->id() < q->id();
    
  } // end of suStatic::compare_wires_by_edgel

  // static
  bool suStatic::compare_wires_by_sidel (const suWire * p,
                                         const suWire * q)
  {
    sutype::dcoord_t sidel1 = p->sidel();
    sutype::dcoord_t sidel2 = q->sidel();

    if (sidel1 != sidel2)
      return (sidel1 < sidel2);

    sutype::dcoord_t edgel1 = p->edgel();
    sutype::dcoord_t edgel2 = q->edgel();

    if (edgel1 != edgel2)
      return (edgel1 < edgel2);
    
    return p->id() < q->id();
    
  } // end of suStatic::compare_wires_by_sidel
  
  // static 
  bool suStatic::compare_pattern_instances_by_id (const suPatternInstance * p,
                                                  const suPatternInstance * q)
  {
    return p->id() < q->id();
    
  } // end of suStatic::compare_pattern_instances_by_id

  // static
  // I don't use layer because it may have undefined PGD
  bool suStatic::compare_ver_wires (suWire * p,
                                    suWire * q)
  {
    const suRectangle & rect1 = p->rect();
    const suRectangle & rect2 = q->rect();

    if (rect1.xl() != rect2.xl())
      return rect1.xl() < rect2.xl();

    if (rect1.yl() != rect2.yl())
      return rect1.yl() < rect2.yl();

    return p->id() < q->id();
    
  } // end of suStatic::compare_ver_wires

  // static
  // I don't use layer because it may have undefined PGD
  bool suStatic::compare_hor_wires (suWire * p,
                                    suWire * q)
  {
    const suRectangle & rect1 = p->rect();
    const suRectangle & rect2 = q->rect();

    if (rect1.yl() != rect2.yl())
      return rect1.yl() < rect2.yl();
    
    if (rect1.xl() != rect2.xl())
      return rect1.xl() < rect2.xl();
    
    return p->id() < q->id();
    
  } // end of suStatic::compare_ver_wires

  //
  bool suStatic::compare_layout_nodes (const suLayoutNode * p,
                                       const suLayoutNode * q)
  {
    bool isleaf1 = p->is_leaf();
    bool isleaf2 = q->is_leaf();

    if (isleaf1 != isleaf2)
      return isleaf1;

    if (isleaf1) {

      sutype::satindex_t satindex1 = p->to_leaf()->satindex();
      sutype::satindex_t satindex2 = q->to_leaf()->satindex();

      SUASSERT ((satindex1 == sutype::UNDEFINED_SAT_INDEX) == (satindex2 == sutype::UNDEFINED_SAT_INDEX), "");

      return satindex1 < satindex2;
    }
    
    else {
      
      const suLayoutFunc * func1 = p->to_func();
      const suLayoutFunc * func2 = q->to_func();
      
      if (func1->func() != func2->func())
        return (int)func1->func() < (int)func2->func();

      if (func1->sign() != func2->sign())
        return (int)func1->sign() < (int)func2->sign();

      const sutype::layoutnodes_t & nodes1 = func1->nodes();
      const sutype::layoutnodes_t & nodes2 = func2->nodes();

      if (nodes1.size() != nodes2.size())
        return nodes1.size() < nodes2.size();

      for (sutype::uvi_t i=0; i < nodes1.size(); ++i) {

        bool v12 = suStatic::compare_layout_nodes (nodes1[i], nodes2[i]);
        bool v21 = suStatic::compare_layout_nodes (nodes2[i], nodes1[i]);

        if (v12 != v21)
          return v12;
      }
    }

    return false;
    
  } // end of suStatic::compare_layout_nodes

  //
  bool suStatic::compare_metal_template_instances (const suMetalTemplateInstance * mti1,
                                                   const suMetalTemplateInstance * mti2)
  {
    const suMetalTemplate * mt1 = mti1->metalTemplate();
    const suMetalTemplate * mt2 = mti2->metalTemplate();

    const suLayer * baselayer1 = mt1->baseLayer();
    const suLayer * baselayer2 = mt2->baseLayer();

    if (baselayer1->level() != baselayer2->level())
      return (baselayer1->level() < baselayer2->level());

    if (mt1->id() != mt2->id())
      return (mt1->id() < mt2->id());

    if (mti1->shift() != mti2->shift()) {
      return (mti1->shift() < mti2->shift());
    }

    sutype::dcoord_t pgdcoord1 = 0;
    sutype::dcoord_t pgdcoord2 = 0;
    sutype::dcoord_t ogdcoord1 = 0;
    sutype::dcoord_t ogdcoord2 = 0;

    if (baselayer1->pgd() == sutype::go_ver) {

      pgdcoord1 = mti1->region().yl();
      pgdcoord2 = mti2->region().yl();

      ogdcoord1 = mti1->region().xl();
      ogdcoord2 = mti2->region().xl();
    }
    
    else {
      
      pgdcoord1 = mti1->region().xl();
      pgdcoord2 = mti2->region().xl();

      ogdcoord1 = mti1->region().yl();
      ogdcoord2 = mti2->region().yl();
    }

    if (pgdcoord1 != pgdcoord2)
      return (pgdcoord1 < pgdcoord2);

    return (ogdcoord1 < ogdcoord2);
    
  } // end of suStatic::compare_metal_template_instances

  // ------------------------------------------------------------
  // -
  // --- Other static methods
  // -
  // ------------------------------------------------------------

  // static
  void suStatic::print_statistic_of_log_messages (sutype::log_event_t le)
  {
    const int ll = 2;

    int index = (int)le;
    
    //SUASSERT (index >= 0 && index < (int)sutype::le_num_types, "");
    
    const std::map<std::string, sutype::uvi_t> & messages = suStatic::_typesOfLogMessages [index];

    SUINFO(ll) << "All " << suStatic::log_event_2_str (le) << " messages:";

    if (messages.empty()) {
      SUOUT(ll) << " none";
    }
    
    SUOUT(ll) << std::endl;

    std::vector<std::pair<std::string,sutype::uvi_t> > listToSort;
    
    for (const auto & iter : messages) {

      const std::string & message = iter.first;
      sutype::uvi_t       count   = iter.second;

      listToSort.push_back (std::make_pair (message, count));
    }

    //std::sort (listToSort.begin(), listToSort.end(), suStatic::compare_pairs_00);

    for (const auto & iter : listToSort) {

      const std::string & message = iter.first;
      sutype::uvi_t       count   = iter.second;

      SUINFO(ll) << "  " << message << " (count=" << count << ")" << std::endl;
    }
    
  } // end of suStatic::print_statistic_of_log_messages

  // bisection search
  // static
  bool suStatic::sorted_clause_has_satindex (const sutype::clause_t & clause,
                                             sutype::satindex_t targetsatindex)
  {
    if (clause.empty()) return false;

    // dummy check
    if (1) {
      for (sutype::uvi_t i=1; i < clause.size(); ++i) {
        SUASSERT (clause[i] > clause[i-1], "");
      }
    }
    //
    
    if (targetsatindex < clause.front()) return false;
    if (targetsatindex > clause.back())  return false;

    int minindex = 0;
    int maxindex = (int)clause.size() - 1;

    while (minindex <= maxindex) {
      
      int index = (minindex + maxindex) / 2;
      SUASSERT (index >= 0, "");
      SUASSERT (index < (int)clause.size(), "");
      
      sutype::satindex_t satindex = clause[index];
      
      if        (targetsatindex > satindex) { minindex = index+1;
      } else if (targetsatindex < satindex) { maxindex = index-1;
      } else {
        return true;
      }
    }

    return false;
    
  } // end of suStatic::sorted_clause_has_satindex

  // static
  void suStatic::print_elapsed_times ()
  {
    SUINFO(2) << "Total time:                 " << _elapsedTime[sutype::ts_total_time]           << " sec." << std::endl;
    SUINFO(1) << "Calibration time:           " << _elapsedTime[sutype::ts_calibration_time]     << std::endl;
    SUINFO(1) << "Antenna time:               " << _elapsedTime[sutype::ts_antenna_total_time]   << std::endl;
    SUINFO(1) << "Total SAT time:             " << _elapsedTime[sutype::ts_sat_total_time]       << std::endl;
    SUINFO(1) << "  Initial SAT solving:      " << _elapsedTime[sutype::ts_sat_initial_solving]  << std::endl;
    SUINFO(1) << "  SAT bound estimation:     " << _elapsedTime[sutype::ts_sat_bound_estimation] << std::endl;
    SUINFO(1) << "  SAT assumptions:          " << _elapsedTime[sutype::ts_sat_assumptions]      << std::endl;
    SUINFO(1) << "  Antenna SAT time:         " << _elapsedTime[sutype::ts_sat_antenna]          << std::endl;
    SUINFO(1) << "  Other SAT time:           " << _elapsedTime[sutype::ts_sat_other_time]       << std::endl;
    
  } // end of suStatic::print_elapsed_times 

  //static
  void suStatic::sort_and_leave_unique (std::vector<int> & values)
  {
    if (values.size() <= 1) return;
    
    std::sort (values.begin(), values.end());

    sutype::uvi_t counter = 1;

    for (sutype::uvi_t i=1; i < values.size(); ++i) {

      if (values[i] == values[counter-1]) continue;

      values[counter] = values[i];
      
      ++counter;
    }

    if (counter != values.size())
      values.resize (counter);
    
  } // end of suStatic::sort_and_leave_unique

  // static
  void suStatic::sort_wires (sutype::wires_t & wires,
                             sutype::grid_orientation_t gd)
  {
    if        (gd == sutype::go_ver) { std::sort (wires.begin(), wires.end(), suStatic::compare_ver_wires);
    } else if (gd == sutype::go_hor) { std::sort (wires.begin(), wires.end(), suStatic::compare_hor_wires);
    } else {
      SUASSERT (false, "");
    }
    
  } // end of suStatic::sort_wires 

  //
  void suStatic::create_parent_dir (const std::string & filename)
  {
    int pos = (int)filename.find_last_of('/');

    // no dir to create
    if (pos < 0) {
      return;
    }

    if (pos + 1 == (int)filename.length()) {
      suStatic::create_dir (filename);
      return;
    }

    create_dir (filename.substr (0, pos));
    
  } // end of suStatic::create_parent_dir

  //
  void suStatic::create_dir (const std::string & dirname)
  {
    system (std::string("mkdir -p " + dirname).c_str());
    
    //SUINFO(1) << "Created a directory: " << dirname << std::endl;
    
  } // end of suStatic::create_dir

  // static
  std::string suStatic::trim_string (const std::string & str)
  {
    if (str.empty()) return "";

    const int length = str.length();

    int startindex = 0;
    int endindex = length-1;

    for (int i=0; i<length; ++i, ++startindex) {
      char c = str[i];
      if (!std::isspace (c))
        break;
    }

    if (startindex == length) return "";

    for (int i=length-1; i>=0; --i, --endindex) {
      char c = str[i];
      if (!std::isspace (c))
        break;
    }

    return str.substr (startindex, endindex-startindex+1);
    
  } // suStatic::trim_string
  
  // static
  std::vector<std::string> suStatic::parse_string (const std::string & str)
  {
    std::vector<std::string> tokens;

    int length = str.length();

    bool startednewtoken = false;
    int newtokenstartindex = -1;

    for (int i=0; i <= length; ++i) {

      char c = (i < length) ? str[i] : ' ';
      
      if (std::isspace(c)) {
        
        if (startednewtoken) {
          std::string token = str.substr (newtokenstartindex, i-newtokenstartindex);
          tokens.push_back (token);
          startednewtoken = false;
        }
      }
      else {
        if (!startednewtoken) {
          startednewtoken = true;
          newtokenstartindex = i;
        }
      }
    }

    return tokens;
    
  } // end of suStatic::parse_string

  // static
  bool suStatic::string_matches_one_of_regular_expressions (const std::string & str,
                                                            const sutype::strings_t & regexps)
  {
    SUASSERT (!regexps.empty(), "");

    for (const auto & iter : regexps) {

      const std::string & regexp = iter;
      if (suStatic::string_matches_regular_expression (str, regexp)) return true;
    }

    return false;
    
  } // end of suStatic::string_matches_one_of_regular_expressions

  // static
  bool suStatic::string_matches_one_of_regular_expressions (const std::string & str,
                                                            const std::set<std::string> & regexps)
  {
    SUASSERT (!regexps.empty(), "");
    
    for (const auto & iter : regexps) {

      const std::string & regexp = iter;
      if (suStatic::string_matches_regular_expression (str, regexp)) return true;
    }

    return false;
    
  } // end of suStatic::string_matches_one_of_regular_expressions  

  // static
  bool suStatic::string_matches_regular_expression (const std::string & inputstr,
                                                    const std::string & regexp)
  {
    SUASSERT (!inputstr.empty(), "");
    SUASSERT (!regexp.empty(), "");

    const std::string anystr = "*";
    sutype::strings_t parts;
    
    // split regular expression onto parts
    if (1) {
      int prevpos = 0;
    
      while (true) {

        if (prevpos >= (int)regexp.length()) break;

        int pos = regexp.find (anystr, prevpos);
      
        if (pos < 0) {
          std::string part = regexp.substr (prevpos);
          //parts.push_back ("AAA_" + suStatic::dcoord_to_str(prevpos) + "_" + suStatic::dcoord_to_str(pos) + "_" + part);
          parts.push_back (part);
          break;
        }

        if (pos != prevpos) {
          std::string part = regexp.substr (prevpos, pos-prevpos);
          //parts.push_back ("BBB_" + suStatic::dcoord_to_str(prevpos) + "_" + suStatic::dcoord_to_str(pos) + "_" + part);
          parts.push_back (part);
        }
      
        //parts.push_back ("CCC_" + suStatic::dcoord_to_str(prevpos) + "_" + suStatic::dcoord_to_str(pos) + "_" + anystr);
        parts.push_back (anystr);
        SUASSERT (regexp.substr (pos, anystr.length()).compare (anystr) == 0, "");
      
        prevpos = pos + anystr.length();
      }
    }

    //SUINFO(1) << "REGEXP: " << regexp << " ==> " << suStatic::to_str(parts) << std::endl;
    
    bool matches = true;

    // try to find every part of the regular expression
    if (1) { 

      SUASSERT (!parts.empty(), "");

      int prevpos = 0;
      bool posMustBeEqualPrevPos = true;
      
      for (const auto & iter : parts) {
        
        const std::string & searchstr = iter;
        if (searchstr.compare (anystr) == 0) {
          posMustBeEqualPrevPos = false;
          continue;
        }

        if ((int)searchstr.length() > ((int)inputstr.length() - prevpos)) {
          matches = false;
          break;
        }
        
        int pos = (int)inputstr.find (searchstr, prevpos);
        //SUINFO(1) << "REGEXP_0: inputstr=" << inputstr << " searchstr=" << searchstr << " prevpos=" << prevpos << " pos=" << pos << std::endl;
        
        if (pos < 0) {
          matches = false;
          break;
        }

        if (posMustBeEqualPrevPos && pos != prevpos) {
          matches = false;
          break;
        }
        
        std::string part = inputstr.substr (pos, searchstr.length());
        SUASSERT (part.compare (searchstr) == 0, "part=" << part << "; searchstr=" << searchstr);
        
        prevpos = pos + searchstr.length();

        posMustBeEqualPrevPos = true;
      }

      if (posMustBeEqualPrevPos && prevpos < (int)inputstr.length()) {
        //SUINFO(1) << "REGEXP_0: inputstr=" << inputstr << " prevpos=" << prevpos << std::endl;
        matches = false;
      }
    }
    
//     if (1) {
//       if (str.compare (regexp) != 0)
//         matches = false;
//     }

    if (matches) {
      //SUINFO(1) << "  REGEXP_1: String: " << inputstr << " matches regexp: " << regexp << " " << suStatic::to_str(parts) << std::endl;
    }
    
    return matches;
    
  } // end of suStatic::string_matches_regular_expression

  // static
  std::string suStatic::dcoord_to_str (sutype::dcoord_t value)
  {
    std::ostringstream oss;

    oss << value;

    return oss.str();
    
  } // end of suStatic::dcoord_to_str
  
  // static
  std::string suStatic::to_str (const sutype::strings_t & strs,
                                const char delimeter,
                                const char openbracket,
                                const char closebracket)
  {
    std::ostringstream oss;

    if (openbracket != '\0')
      oss << openbracket;
    
    for (sutype::uvi_t i = 0; i < strs.size(); ++i) {

      const std::string & str = strs[i];
      
      if (i > 0)
        oss << ",";
      
      oss << str;
    }
    
    if (closebracket != '\0')
      oss << closebracket;
    
    return oss.str();    
    
  } // end of suStatic::to_str

  // static
  std::string suStatic::clause_to_str (const sutype::clause_t & values,
                                       const char delimeter,
                                       const char openbracket,
                                       const char closebracket)
  {
    sutype::strings_t strs;
    
    for (const auto & iter : values) {
      strs.push_back (std::to_string (iter));
    }
    
    return suStatic::to_str (strs, delimeter, openbracket, closebracket);
    
  } // end of suStatic::clause_2_str

  // static
  std::string suStatic::to_str (const sutype::dcoords_t & values,
                                const char delimeter,
                                const char openbracket,
                                const char closebracket)
  {
    sutype::strings_t strs;
    
    for (const auto & iter : values) {
      strs.push_back (std::to_string (iter));
    }
    
    return suStatic::to_str (strs, delimeter, openbracket, closebracket);
 
  } // end of suStatic::to_str

  // static
  std::string suStatic::to_str (const std::vector<int> & values,
                                const char delimeter,
                                const char openbracket,
                                const char closebracket)
  {
    sutype::strings_t strs;
    
    for (const auto & iter : values) {
      strs.push_back (std::to_string (iter));
    }
    
    return suStatic::to_str (strs, delimeter, openbracket, closebracket);
 
  } // end of suStatic::to_str
  
  // static
  std::string suStatic::to_str (const std::set<std::string> & values,
                                const char delimeter,
                                const char openbracket,
                                const char closebracket)
  {
    sutype::strings_t strs;
    
    for (const auto & iter : values) {
      strs.push_back (iter);
    }
    
    return suStatic::to_str (strs, delimeter, openbracket, closebracket);
    
  } // end of suStatic::to_str

  // static
  std::string suStatic::to_str (const sutype::wires_t & wires,
                                const char delimeter,
                                const char openbracket,
                                const char closebracket)
  {
    sutype::strings_t strs;

    for (const auto & iter : wires) {
      strs.push_back (iter->to_str());
    }

    return suStatic::to_str (strs, delimeter, openbracket, closebracket);
    
  } // end of suStatic::to_str

  // static
  bool suStatic::wires_are_electrically_connected (const suWire * wire1,
                                                   const suWire * wire2)
  {
    const suLayer * layer1 = wire1->layer();
    const suLayer * layer2 = wire2->layer();

    return (layer1->is_electrically_connected_with (layer2) && wire1->rect().has_at_least_common_point (wire2->rect()));
    
  } // end of suStatic::wires_are_electrically_connected

  // static
  bool suStatic::wire_is_redundant (const suWire * tstwire,
                                    const sutype::wires_t & wires)
  {
    for (const auto & iter : wires) {
      
      suWire * wire = iter;
      if (wire == tstwire) return true;

      if (wire->layer() == tstwire->layer() &&
          wire->rect().xl() <= tstwire->rect().xl() &&
          wire->rect().yl() <= tstwire->rect().yl() &&
          wire->rect().xh() >= tstwire->rect().xh() &&
          wire->rect().yh() >= tstwire->rect().yh()) return true;
    }
    
    return false;
    
  } // end of suStatic::wire_is_redundant
  
  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------


} // end of namespace amsr

// end of suStatic.cpp
