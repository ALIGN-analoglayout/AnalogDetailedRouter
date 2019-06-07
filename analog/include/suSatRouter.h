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
//! \date   Fri Oct 13 11:44:23 2017

//! \file   suSatRouter.h
//! \brief  A header of the class suSatRouter.

#ifndef _suSatRouter_h_
#define _suSatRouter_h_

// system includes

// std includes
#include <map>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suLayer.h>
#include <suNet.h>

namespace amsr
{
  // classes
  class suConnectedEntity;
  class suGeneratorInstance;
  class suGrid;
  class suLayer;
  class suLayoutFunc;
  class suNet;
  class suRectangle;
  class suTie;
  class suToken;
  
  //! 
  class suSatRouter
  {
    friend class suRouteGenerator;

  private:

    //! static instance
    static suSatRouter * _instance;

  private:

    //!
    sutype::grids_t _grids;
    
    //
    sutype::nets_t _netsToRoute;
    
    // net->id() is used as a key
    std::map<sutype::id_t,sutype::connectedentities_t> _connectedEntities;

    // net->id() is used as a key
    std::map<sutype::id_t,sutype::ties_t> _ties;

    // net->id() is used as a key; precomputed info of connected wires and vias
    std::map<sutype::id_t,std::map<suWire *, sutype::uvi_t> > _wireToGroupIndex; // I save it here for a while
    
    // netid                        layerid                cutwiretype                   dx                           dy
    std::map<sutype::id_t, std::map<sutype::id_t, std::map<sutype::wire_type_t, std::map <sutype::dcoord_t, std::map <sutype::dcoord_t, sutype::generatorinstances_t> > > > > _generatorinstances;
    
    //
    sutype::wires_t _satindexToPointer;
    
    //
    bool _option_may_skip_global_routes;
    bool _option_assert_illegal_input_wires;
    bool _option_simplify_routes;
    
  private:

    //! default constructor
    suSatRouter ()
    {
      init_ ();

    } // end of suSatRouter

  private:

    //! copy constructor
    suSatRouter (const suSatRouter & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suSatRouter

    //! assignment operator
    suSatRouter & operator = (const suSatRouter & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suSatRouter ();

  private:

    //! init all class members
    void init_ ();

    //! copy all class members
    inline void copy_ (const suSatRouter & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

  public:

    // static methods
    //

    //
    static void delete_instance ();
    
    //
    static inline suSatRouter * instance ()
    {
      if (suSatRouter::_instance == 0)
        suSatRouter::_instance = new suSatRouter ();

      return suSatRouter::_instance;

    } // end of instance
    
    //
    static bool wires_are_in_conflict (const sutype::wires_t & wires1,
                                       const sutype::wires_t & wires2,
                                       bool verbose = false);
    
    //
    static bool wires_are_in_conflict (const suWire * wire1,
                                       const sutype::wires_t & wires2,
                                       bool verbose = false);

    //
    static bool wires_are_in_conflict (const suWire * wire1,
                                       const suWire * wire2);

    //
    static bool wires_are_in_conflict (const suWire * wire1,
                                       const suWire * wire2,
                                       sutype::wire_conflict_t & wireconflict);
    
    //
    static bool wires_are_in_conflict (const suWire * wire1,
                                       const suWire * wire2,
                                       sutype::wire_conflict_t & wireconflict,
                                       const sutype::bitid_t targetwireconflict,
                                       bool verbose);

    //
    static bool static_wire_covers_edge (sutype::satindex_t satindex,
                                         sutype::dcoord_t dcoord,
                                         const suLayer * layer,
                                         sutype::clause_t & clause);
       
    //
    static void print_layout_node (std::ostream & oss,
                                   const suLayoutNode * node,
                                   const std::string & offset);
    
  public:

    //
    void unregister_a_sat_wire (suWire * wire);

    //
    void register_a_sat_wire (suWire * wire);
    
    //
    void dump_connected_entities (const std::string & filename)
      const;

    //
    void create_fake_metal_template_instances (bool pgd_tracks = true,
                                               bool ogd_grid_lines = true,
                                               bool pgd_grid_tracks = false)
      const;
    
    //
    void create_fake_connected_entities ()
      const;

    //
    void create_fake_ties ()
      const;

    //
    void create_fake_sat_wires ()
      const;
    
    //
    void prepare_input_wires_and_create_connected_entities ();

    //
    void create_evident_preroutes ();
    
    //
    bool check_input_wires ()
      const;
    
    //
    void merge_input_wires ();
    
    //
    void register_preroutes_as_sat_objects ();

    //
    void convert_global_routing_to_trunks ();
    
    //
    void create_ties ();

    //
    void create_global_routing ();

    //
    void prune_redundant_ties ();

    //
    void create_routes ();
    
    //
    void prune_ties ();
    
    //
    void emit_connected_entities ();

    //
    void emit_trunk_routing ();
    
    //
    void emit_conflicts ()
      const;
    
    //
    void emit_patterns ()
      const;

    //
    void emit_same_width_for_trunks ()
      const;

    //
    void emit_ties_and_routes ();

    //
    void emit_tie_triangles ();

    //
    void emit_metal_fill_rules ();
    
    // \return true if ok
    bool solve_the_problem ()
      const;

    //
    void optimize_routing ();

    //
    void post_routing_fix_wires (const sutype::wires_t & wires)
      const;

    //
    void post_routing_fix_trunks ()
      const;

    //
    void post_routing_delete_useless_generator_instances (bool checkCurrentModeledValue,
                                                          const suNet * targetnet = 0);

    //
    void post_routing_fix_routes (bool deleteAbsentRoutes);

    //
    void eliminate_antennas ();

    //
    void eliminate_antennas (sutype::wire_type_t inputtargetwiretype);
    
    //
    void report_disconnected_entities ()
      const;
        
    //
    void apply_solution ();

    //
    sutype::object_t get_object_type_by_satindex (sutype::satindex_t satindex)
      const;

    //
    suWire * get_wire_by_satindex (sutype::satindex_t satindex)
      const;

    //
    suGeneratorInstance * get_generator_instance_by_satindex (sutype::satindex_t satindex)
      const;

    //
    void print_applied_ties ()
      const;
    
    //
    void print_applied_routes ()
      const;
    
    //
    void create_grids_for_generators (sutype::grids_t & grids)
      const;

    //
    bool wire_is_legal_slow_check (const suWire * wire)
      const;
    
    //
    bool wire_is_legal_slow_check (const suWire * wire,
                                   bool verbose)
      const;
    
    //
    bool wire_is_legal_slow_check (const suWire * wire1,
                                   bool verbose,
                                   suNet * & conflictingnet)
      const;
    
  private:

    //
    bool wire_is_out_of_boundaries_ (const suWire * wire)
      const;

    //
    bool potential_tie_is_most_likely_redundant_ (const suConnectedEntity * ce1,
                                                  const suConnectedEntity * ce2,
                                                  const sutype::connectedentities_t & ces)
      const;
      
    //
    suTie * get_tie_ (suConnectedEntity * ce0,
                      suConnectedEntity * ce1)
      const;

    //
    void emit_min_length_ ();

    //
    void emit_conflicts_for_wires_ (const sutype::wires_t & inputwires)
      const;

    //
    void emit_conflicts_for_vias_ (const sutype::wires_t & inputwires)
      const;

    //
    void detect_mandatory_satindices_for_fixed_routes_ (const sutype::routes_t & routes)
      const;

    //
    void remove_constanst_wires_ (sutype::clause_t & satindices)
      const;

    //
    std::vector<sutype::wires_t> calculate_connected_entities_ (const suNet * net)
      const;
    
    //
    void calculate_connected_entities_ (sutype::wires_t & allwires,
                                        std::vector<sutype::wires_t> & ces)
      const;

    //
    static void print_layout_func_ (std::ostream & oss,
                                    const suLayoutFunc * func,
                                    const std::string & offset);
    
    //
    static void print_layout_leaf_ (std::ostream & oss,
                                    const suLayoutLeaf * leaf,
                                    const std::string & offset);

    //
    suTie * create_prerouted_tie_ (const suConnectedEntity * ce1,
                                   const suConnectedEntity * ce2);

    //
    bool wire_is_obsolete_ (const suWire * wire)
      const;
    
    //
    suGeneratorInstance * create_generator_instance_ (const suGenerator * generator,
                                                      const suNet * net,
                                                      sutype::dcoord_t dx,
                                                      sutype::dcoord_t dy,
                                                      sutype::wire_type_t cutwiretype,
                                                      bool verbose);

    //
    void emit_generator_instance_ (suGeneratorInstance * generatorinstance);

    //
    void emit_conflicts_ (const suWire * wire1)
      const;
    
    //
    long int emit_conflict_ (const suWire * wire1,
                             const suWire * wire2)
      const;
    
    //
    bool wire_covers_edge_ (sutype::satindex_t satindex,
                            sutype::dcoord_t edge,
                            const suLayer * layer,
                            sutype::clause_t & clause)
      const;
        
    //
    void add_connected_entity_ (suConnectedEntity * ce);

    //
    void delete_unfeasible_net_ties_ (sutype::id_t netid);
    
    //
    void add_tie_ (suTie * tie);

    //
    sutype::dcoord_t calculate_distance_ (const sutype::wires_t & wires1,
                                          const sutype::wires_t & wires2,
                                          sutype::grid_orientation_t gd)
      const;

    //
    void create_routes_ (suTie * tie);

    //
    void calculate_net_groups_ (const sutype::tokens_t & cetokens);

    // \return created route
    suRoute * create_route_between_two_wires_ (suWire * wire1,
                                               suWire * wire2,
                                               bool usePreroutesOnly);
    
    //
    void emit_net_ties_ (const sutype::connectedentities_t & connectedEntities,
                         const sutype::ties_t & ties)
      const;
    
    //
    sutype::clause_t & get_sat_indices_ (const sutype::ties_t & ties)
      const;
    
    //
    sutype::satindex_t emit_or_ties_ (const sutype::ties_t & ties)
      const;
    
    //
    sutype::ties_t get_common_ties_ (const sutype::ties_t & ties1,
                                     const sutype::ties_t & ties2)
      const;

    //
    void prepare_antennas_ (const sutype::clause_t & satindicesToCheck,
                            sutype::clause_t & satindicesToCheck0, // likely optional
                            sutype::clause_t & satindicesToCheck1, // likely mandatory
                            std::map<const suNet *, std::map <const suLayer *, sutype::wires_t, suLayer::cmp_const_ptr>, suNet::cmp_const_ptr> & fixedWiresPerLayerPerNet)
      const;

    //
    void eliminate_antennas_by_bisection_ (sutype::clause_t & satindicesToCheck,
                                           sutype::clause_t & satindicesToCheckLazily,
                                           sutype::clause_t & assumptions,
                                           sutype::uvi_t & nummandatory,
                                           std::map<const suNet *, std::map <const suLayer *, sutype::wires_t, suLayer::cmp_const_ptr>, suNet::cmp_const_ptr> & fixedWiresPerLayerPerNet,
                                           std::map<sutype::satindex_t,sutype::routes_t> & satindexToRoutes);

    //
    void release_useless_wires_and_delete_useless_routes_ (const sutype::clause_t & wiresatindices);

    //
    void release_useless_wire_and_delete_useless_routes_ (suWire * wire);
    
    //
    suLayoutFunc * create_a_counter_for_total_wire_width_ (const sutype::wires_t & wires,
                                                           sutype::dcoord_t minWireWidth)
    const;
    
    //
    void enumerate_possible_sets_of_wires_ (const std::vector<sutype::wires_t> & wiresPerWidth,
                                            sutype::uvi_t index,
                                            sutype::dcoord_t targetwidth,
                                            sutype::clause_t & currentwave,
                                            sutype::clauses_t & combinationsOfWires)
      const;

    //
    void optimize_the_number_of_open_connected_entities_ ()
      const;
    
    //
    void optimize_the_number_of_open_ties_ ()
      const;

    //
    void optimize_the_number_of_direct_wires_between_preroutes_ (bool createpreroutes);
    
    //
    void optimize_the_number_of_direct_wires_between_preroutes_ (const suLayer * layer,
                                                                 sutype::opt_mode_t optmode,
                                                                 bool createpreroutes);

    //
    void create_direct_wires_ (const sutype::wires_t & netwires);
    
    //
    void optimize_the_number_of_tracks_ (sutype::wire_type_t targetwiretype,
                                         bool fixWiresOnFly,
                                         bool splitWiresPerGrStripes,
                                         int numberOfWiresWeMayLeave)
      const;

    //
    void optimize_the_number_of_tracks_ (const sutype::wires_t & wires,
                                         bool fixWiresOnFly,
                                         bool splitWiresPerGrStripes,
                                         int numberOfWiresWeMayLeave)
      const;

    //
    void optimize_the_number_of_tracks_of_layered_wires_ (const sutype::wires_t & wires,
                                                          bool fixWiresOnFly,
                                                          int numberOfWiresWeMayLeave)
      const;
    
    //
    void minimize_the_number_of_preroute_extensions_ ()
      const;
    
    //
    void optimize_the_number_of_connected_entities_ (int minLengthToConsider)
      const;

    //
    void optimize_the_number_of_bad_wires_ ()
      const;
    
    //
    void optimize_the_number_of_ties_ (unsigned targetNumConnectedEntitiesWithTrunks,
                                       sutype::opt_mode_t optmode,
                                       int minlengthtoconsider,
                                       int maxlengthtoconsider)
      const;

    //
    void optimize_wire_widths_ (sutype::wire_type_t targetwiretype)
      const;

    //
    void optimize_wire_lengths_ (sutype::wire_type_t targetwiretype)
      const;
    
    //
    void optimize_connections_to_preroutes_ ()
      const;
    
    //
    void optimize_the_number_of_routes_ (unsigned targetNumConnectedEntitiesWithTrunks,
                                         sutype::opt_mode_t optmode)
      const;
    
    //
    void optimize_generator_instances_by_coverage_ ()
      const;

    //
    sutype::routes_t get_routes_ ()
      const;
        
    //
    void find_ties_having_a_route_with_this_wire_ (const suWire * wire,
                                                   sutype::ties_t & ties,
                                                   sutype::routes_t & routes)
      const;

    //
    bool path_exists_between_two_connected_entities_ (const suConnectedEntity * ce1,
                                                      const suConnectedEntity * ce2,
                                                      bool checkRoutes,
                                                      bool useTrunksOnly,
                                                      suTie * tieToSkip)
      const;

    //
    void trim_unfeasible_trunk_ties_ (sutype::ties_t & ties)
      const;
    
    //
    bool can_reach_target_connected_entity_ (const suConnectedEntity * currentce,
                                             const suConnectedEntity * targetce,
                                             sutype::ties_t & ties)
      const;

    //
    void update_affected_routes_ (std::map<sutype::satindex_t,sutype::routes_t> & satindexToRoutes)
      const;

    //
    void update_affected_routes_ (std::map<sutype::satindex_t,sutype::routes_t> & satindexToRoutes,
                                  sutype::satindex_t targetsatindex)
      const;

    //
    void update_affected_routes_ (std::map<sutype::satindex_t,sutype::routes_t> & satindexToRoutes,
                                  const sutype::clause_t & satindices)
      const;

    //
    void create_new_global_routes_ (suGlobalRoute * gr,
                                    bool upper,
                                    sutype::globalroutes_t & grs,
                                    int depth)
      const;
    
  }; // end of class suSatRouter
  
} // end of namespace amsr

#endif // _suSatRouter_h_

// end of suSatRouter.h

