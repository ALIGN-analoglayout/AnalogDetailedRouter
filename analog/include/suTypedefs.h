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
//! \date   Wed Sep 27 14:02:07 2017

//! \file   suTypedefs.h
//! \brief  A collection of types

#ifndef _suTypedefs_h_
#define _suTypedefs_h_

// system includes
#include <stdint.h>

// std includes
#include <map>
#include <set>
#include <string>
#include <tr1/tuple>
#include <vector>

namespace amsr
{
  class suCellMaster;
  class suColoredLayer;
  class suConnectedEntity;
  class suGenerator;
  class suGeneratorInstance;
  class suGlobalRoute;
  class suGREdge;
  class suGrid;
  class suError;
  class suLayoutFunc;
  class suLayoutLeaf;
  class suLayoutNode;
  class suLayer;
  class suMetalTemplate;
  class suMetalTemplateInstance;
  class suNet;
  class suPattern;
  class suPatternInstance;
  class suRectangle;
  class suRegion;
  class suRoute;
  class suTie;
  class suToken;
  class suViaField;
  class suWire;
  
  namespace sutype
  {
    // basic types
    typedef int64_t  glucosevar_t;
    typedef int64_t  clauseid_t;
    typedef int64_t  dcoord_t;
    typedef int      gcoord_t;
    typedef int      satindex_t;
    typedef unsigned uvi_t; // "unsigned vector index" -- integer to iteratie through vector elements
    typedef int      svi_t; // "signed vector index"   -- integer to iteratie through vector elements
    typedef int      id_t;
    typedef int      bitid_t; // used for bit operations

    static const sutype::satindex_t  UNDEFINED_SAT_INDEX =  0;
    static const sutype::satindex_t  NEGATIVE_SAT_INDEX  = -3; // used in very rare cases to separate from emitted constants and UNDEFINED_SAT_INDEX
    static const sutype::id_t        UNDEFINED_GLOBAL_ID = -1;
    
    //
    typedef std::pair<dcoord_t,dcoord_t> dcoordpair_t;
    typedef std::vector<dcoordpair_t> dcoordpairs_t;
    
    // vectors
    typedef std::vector<std::string>        strings_t;
    typedef std::vector<sutype::satindex_t> clause_t;
    typedef std::vector<clause_t>           clauses_t;
    typedef std::vector<clauseid_t>         clauseids_t;
    typedef std::vector<dcoord_t>           dcoords_t;
    typedef std::vector<id_t>               ids_t;

    // vectors of objects
    typedef std::vector<suCellMaster *>            cellmasters_t;
    typedef std::vector<suConnectedEntity *>       connectedentities_t;
    typedef std::vector<suGeneratorInstance *>     generatorinstances_t;
    typedef std::vector<suGlobalRoute *>           globalroutes_t;
    typedef std::vector<suGREdge *>                gredges_t;
    typedef std::vector<suGrid *>                  grids_t;
    typedef std::vector<suError *>                 errors_t;
    typedef std::vector<suLayoutFunc *>            layoutfuncs_t;
    typedef std::vector<suLayoutLeaf *>            layoutleaves_t;
    typedef std::vector<suLayoutNode *>            layoutnodes_t;
    typedef std::vector<suMetalTemplateInstance *> metaltemplateinstances_t;
    typedef std::vector<suNet *>                   nets_t;
    typedef std::vector<suPattern *>               patterns_t;
    typedef std::vector<suRectangle *>             rectangles_t;
    typedef std::vector<suRegion *>                regions_t;
    typedef std::vector<suRoute *>                 routes_t;
    typedef std::vector<suTie *>                   ties_t;
    typedef std::vector<suToken *>                 tokens_t;
    typedef std::vector<suViaField *>              viafields_t;
    typedef std::vector<suWire *>                  wires_t;
    
    // vectors of const objects
    typedef std::vector<const suColoredLayer *>          coloredlayers_tc;
    typedef std::vector<const suConnectedEntity *>       connectedentities_tc;
    typedef std::vector<const suGenerator *>             generators_tc;
    typedef std::vector<const suLayer *>                 layers_tc;
    typedef std::vector<const suMetalTemplate *>         metaltemplates_tc;
    typedef std::vector<const suNet *>                   nets_tc;
    typedef std::vector<const suPattern *>               patterns_tc;
    typedef std::vector<const suPatternInstance *>       patterninstances_tc;

    // status_t
    typedef enum status_t {
      
      status_ok = 0,
      status_fail,

    } status_t;

    // ref_t
    typedef enum ref_t {
      
      ref_0 = 0,
      ref_x, 
      ref_y,
      ref_xy,
      ref_num_types,

    } ref_t;

    // transformations
    typedef std::tuple<sutype::dcoord_t,sutype::dcoord_t,sutype::ref_t> tr_t;
    typedef std::vector<sutype::tr_t> trs_t;
    
    //
    typedef std::tuple<sutype::dcoord_t,sutype::dcoord_t,sutype::generatorinstances_t> viaoption_t;
    typedef std::vector<viaoption_t> viaoptions_t;
    
    // bool_t
    typedef enum bool_t {
      
      bool_false = 0,
      bool_true,
      bool_undefined,
      
    } bool_t;
    
    typedef std::vector<bool_t> bools_t;

    // object_t
    typedef enum object_t {

      obj_none = 0,
      obj_generator_instance,
      obj_wire,
      
    } object_t;

    typedef std::vector<object_t> objects_t;

    // side_t
    typedef enum side_t {
      
      side_undefined = -1,
      side_west,
      side_east,
      side_north,
      side_south,
      
    } side_t;

    // logic_func_t
    typedef enum logic_func_t {
      
      logic_func_and = 0,
      logic_func_or,
      
    } logic_func_t;

    // counter_t
    typedef enum boolean_counter_t {

      bc_greater = 1,
      bc_greater_or_equal,
      bc_less,
      bc_less_or_equal,
      bc_equal,
      
    } boolean_counter_t;
    
    // unary_sign_t
    typedef enum unary_sign_t {
      
      unary_sign_not = 0,
      unary_sign_just,
      
    } unary_sign_t;

    //
    typedef enum opt_mode_t {

      om_minimize = 0,
      om_maximize,
      
    } opt_mode_t;

    //
    typedef enum rect_corner_t {
      
      rc_xl = 0,
      rc_yl,
      rc_xh,
      rc_yh,
      
    } rect_corner_t;

    //
    typedef enum point_coord_t {
      
      pc_x = 0,
      pc_y,
      
    } point_coord_t;
    
    //
    typedef enum min_max_t {
      
      mm_min = 0,
      mm_max,

    } min_max_t;

    //
    typedef enum grid_orientation_t {

      go_ver = 0,
      go_hor,
      
    } grid_orientation_t;

    //
    typedef enum grid_direction_t {

      gd_pgd = 0,
      gd_ogd,
      
    } grid_direction_t;
    
    // don't forget to update suStatic::layer_type_2_str
    typedef enum layer_type_t {

      lt_undefined = 0,
      lt_wire      = 1,
      lt_via       = 1 << 1,
      lt_metal     = 1 << 2,
      lt_poly      = 1 << 3,
      lt_diffusion = 1 << 4,
      lt_well      = 1 << 5,
      
    } layer_type_t;

    // wire direction
    typedef enum wire_dir_t {
      
      wd_lh  = 0,
      wd_hl  = 1,
      wd_any = 2,
      
    } wire_dir_t;
    
    // don't forget to update suStatic::wire_type_2_str
    typedef enum wire_type_t {

      wt_undefined = 0,
      wt_preroute  = 1,
      wt_trunk     = 1 << 1,
      wt_shunt     = 1 << 2,
      wt_route     = 1 << 3,
      wt_fill      = 1 << 4,
      wt_cut       = 1 << 5,
      wt_enclosure = 1 << 6,
      wt_pattern   = 1 << 7,
      wt_num_types = 1 << 8,
      
    } wire_type_t;

    // don't forget to update suStatic::wire_conflict_2_str
    typedef enum wire_conflict_t {

      wc_undefined = 0,
      wc_short     = 1,
      wc_minete    = 1 << 1,
      wc_trunks    = 1 << 2,
      wc_colors    = 1 << 3,
      wc_routing   = 1 << 4,
      wc_all_types = (1 << 5) - 1,
      wc_num_types = 1 << 5,
      
    } wire_conflict_t;

    //
    typedef enum via_generator_layer_t {

      vgl_cut = 0,
      vgl_layer1,
      vgl_layer2,
      vgl_num_types,
      
    } via_generator_layer_t;

    //
    typedef enum rule_type_t {

      rt_minete = 0,
      rt_minlength,
      rt_minencl,
      rt_num_types,
      
    } rule_type_t;

    //
    typedef enum log_event_t {

      le_fatal = 0,
      le_error,
      le_issue,
      le_label,
      le_info,
      le_num_types,
      
    } log_event_t;

    //
    typedef enum time_spend_t {

      ts_total_time = 0,
      ts_calibration_time,
      ts_antenna_total_time,
      ts_sat_total_time,
      ts_sat_antenna,
      ts_sat_initial_solving,
      ts_sat_assumptions,
      ts_sat_bound_estimation,
      ts_sat_other_time,
      ts_num_types,

    } time_spend_t;
    
  } // end of namespace sutype
  
} // end of namespace amsr

#endif // _suTypedefs_h_

// end of suTypedefs.h

