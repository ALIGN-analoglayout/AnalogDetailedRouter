// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Thu Mar 29 12:50:57 2018

//! \file   suRouteGenerator.h
//! \brief  A header of the class suRouteGenerator.

#ifndef _suRouteGenerator_h_
#define _suRouteGenerator_h_

// system includes

// std includes

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes

namespace amsr
{
  // classes
  class suLayoutFunc;
  class suNet;
  class suRoute;
  class suViaField;
  class suWire;

  //! 
  class suRouteGenerator
  {
  private:

    //
    static sutype::id_t _uniqueId;

    //
    const suNet * _net;

    //
    sutype::id_t _id;

    //
    sutype::id_t _debugid;
    
    //
    suWire * _wire1;
    
    //
    suWire * _wire2;

    //
    sutype::wires_t _wires1and2;

    // optional
    sutype::regions_t _regions;
    
    //
    const suLayer * _layer1;

    //
    const suLayer * _layer2;

    //
    sutype::layers_tc _vialayers;

    //
    sutype::layers_tc _wirelayers;

    //
    suRoute * _route;

    // root AND
    //   wire1
    //   wire2
    //   a SAT literal to able to make this route constant_0
    //   an OR of possible connections
    suLayoutFunc * _layoutfunc0;

    // here I put all routing options
    // it's an OR of possible connections
    suLayoutFunc * _layoutfunc1;

    //
    sutype::clause_t _routeEnablers;
    
    //
    sutype::viafields_t _viaFields;

    // 0: enumerate routes in a relatively small transition region
    // 1: enumerate all routes in a bounding box around terminals
    bool _optionRouteFullBbox;

    // 0: enumerate all possible routes
    // 1: create a free form
    bool _optionRouteFreeForm;
    
  public:

    //! custom constructor
    suRouteGenerator (const suNet * n,
                      suWire * w1,
                      suWire * w2);
    
    //! default constructor
    suRouteGenerator ()
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();

    } // end of suRouteGenerator

  private:

    //! copy constructor
    suRouteGenerator (const suRouteGenerator & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suRouteGenerator

    //! assignment operator
    suRouteGenerator & operator = (const suRouteGenerator & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suRouteGenerator ();

  private:

    //! init all class members
    inline void init_ ()
    {
      _id = suRouteGenerator::_uniqueId;
      ++suRouteGenerator::_uniqueId;

      _debugid = -1; // used for debug only; set in constructor
      
      _net = 0;
      _wire1 = 0;
      _wire2 = 0;
      _layer1 = 0;
      _layer2 = 0;
      _route = 0;
      _layoutfunc0 = 0;
      _layoutfunc1 = 0;
      _optionRouteFullBbox = false;
      _optionRouteFreeForm = false;

    } // end of init_

    //! copy all class members
    inline void copy_ (const suRouteGenerator & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

    //
    const sutype::id_t id () const { return _id; }

  public:

    //
    suRoute * connect_wires_by_preroutes (const std::map<suWire *, sutype::uvi_t> & wireToGroupIndex); // optional pre-computed info

    //
    suRoute * create_route (const std::map<suWire *, sutype::uvi_t> & wireToGroupIndex); // optional pre-computed info
    
  private:

    // \return false if there're some issues
    bool init_route_ ();

    //
    void simplify_route_ ();
    
    //
    void delete_route_ ();

    //
    void find_a_path_between_wires_ (suWire * startwire,
                                     suWire * tartgetwire,
                                     const sutype::wires_t & netwires,
                                     const sutype::generatorinstances_t & netgeneratorinstances,
                                     sutype::wires_t & visitedwires,
                                     sutype::generatorinstances_t & visitedgeneratorinstances,
                                     std::vector<bool> & wiresInUse,
                                     std::vector<bool> & gisInUse,
                                     bool & reachedTargetWire,
                                     const bool collectAllConnectedWires)
      const;
    
    //
    bool try_to_connect_wires_by_preroutes_ (const std::map<suWire *, sutype::uvi_t> & wireToGroupIndex);

    //
    void create_layout_free_form_ ();

    //
    void create_layout_fill_a_gap_between_input_wires_ ();

    //
    void create_layout_enumerate_shunts_between_wires_of_the_same_layer_ (int depth);
    
    //
    void create_layout_connect_wires_on_different_layers_ ();

    //
    void clear_via_fields_ ();
    
    //
    void remove_via_option_ (sutype::uvi_t v,
                             sutype::dcoord_t dx,
                             sutype::dcoord_t dy);
    
    //
    void remove_via_options_ (sutype::uvi_t v,
                              sutype::dcoord_t dcoord,
                              sutype::grid_orientation_t gd);

    //
    std::map<sutype::dcoordpair_t,sutype::clause_t> get_dcoords_and_satindices_ (sutype::uvi_t v)
      const;
    
    //
    std::map<sutype::dcoord_t,sutype::clause_t> get_dcoords_and_satindices_ (sutype::uvi_t v,
                                                                             sutype::grid_orientation_t gd)
      const;
    
    // \return true if ok
    // constrainBoundaryViaFields=1: create only a line of vias for the first and last via fields
    bool create_via_fields_ (bool constrainBoundaryViaFields);

    // \return true if ok
    bool convert_via_fields_to_sat_problem_ (suLayoutFunc * layoutfunc2);

    // \return true if ok
    bool sat_problem_lower_via_cannot_appear_without_at_least_one_upper_via_at_the_same_dcoord_ (suLayoutFunc * layoutfunc2,
                                                                                                 const bool emit,
                                                                                                 bool & modified);

    // \return true if ok
    bool sat_problem_create_first_and_last_wires_ (suLayoutFunc * layoutfunc2,
                                                   const bool emit,
                                                   bool & modified);

    // \return true if ok
    bool sat_problem_vias_on_adjacent_via_fields_cannot_apper_without_a_wire_in_between_ (suLayoutFunc * layoutfunc2,
                                                                                          const bool emit,
                                                                                          bool & modified);

    // \return true if ok
    bool sat_problem_every_via_field_must_have_at_least_one_via_ (suLayoutFunc * layoutfunc2,
                                                                  bool minNumVias,
                                                                  bool maxNumVias);

    // \return true if ok
    bool sat_problem_lower_via_cannot_appear_without_another_via_ (suLayoutFunc * layoutfunc2, // top-level AND for this route
                                                                   const bool emit,
                                                                   bool & modified);
    
    // \return true if ok
    bool sat_problem_every_layer_may_have_only_one_shunt_wire_ (suLayoutFunc * layoutfunc2);
    
    //
    void calculate_transition_region_ (const suWire * wire1,
                                       const suWire * wire2,
                                       suRectangle & transitionrect)
      const;
    
    //
    void calculate_transition_region_maximal_ (const suWire * wire1,
                                               const suWire * wire2,
                                               suRectangle & transitionrect)
      const;

    //
    void calculate_transition_region_for_perpendicular_wires_ (const suWire * wire1,
                                                               const suWire * wire2,
                                                               suRectangle & transitionrect)
      const;
    
    //
    void calculate_transition_region_for_collineral_wires_ (const suWire * wire1,
                                                            const suWire * wire2,
                                                            suRectangle & transitionrect)
      const;

    //
    sutype::generatorinstances_t create_generator_instances_ (const suGrid * grid,
                                                              const sutype::dcoord_t dx,
                                                              const sutype::dcoord_t dy)
      const;

    //
    suWire * serve_just_created_wire_ (suWire * wire)
      const;

    //
    sutype::clause_t & get_satindices_in_bounds_ (const std::vector<std::map<sutype::dcoordpair_t,sutype::clause_t> > & satindicesPerDcoordAll,
                                                  const suLayer * wirelayer,
                                                  const sutype::svi_t wireLayerIndexToSkip,
                                                  const sutype::dcoord_t sidec,
                                                  const sutype::dcoord_t edgel,
                                                  const sutype::dcoord_t edgeh)
      const;

    //
    suLayoutFunc * create_route_option_ ();

    //
    bool point_is_in_tunnel_ (sutype::dcoord_t dx,
                              sutype::dcoord_t dy)
      const;
    
  }; // end of class suRouteGenerator

} // end of namespace amsr

#endif // _suRouteGenerator_h_

// end of suRouteGenerator.h

