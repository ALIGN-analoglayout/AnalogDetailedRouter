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
//! \date   Mon Oct 16 13:21:15 2017

//! \file   suWireManager.h
//! \brief  A header of the class suWireManager.

#ifndef _suWireManager_h_
#define _suWireManager_h_

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
  class suWire;

  //! 
  class suWireManager
  {
  private:

    //! static instance
    static suWireManager * _instance;

    //                     sidel                       sideh                       edgel             wires
    std::vector <std::map <sutype::dcoord_t, std::map <sutype::dcoord_t, std::map <sutype::dcoord_t, sutype::wires_t> > > > _wiresPerLayers;
     
  private:

    //
    sutype::uvi_t _numCreatedWires;
    
    // debug only
    sutype::uvi_t _num_releases;
    
    // debug only
    sutype::id_t _debugid;
    
    //
    sutype::wires_t _wires;
    
    //
    sutype::wires_t _gidToWire;
    
    //
    std::vector<sutype::svi_t> _wireUsage;

    //
    std::vector<bool> _wireObsolete;

    //
    std::vector<bool> _wireIllegal;

    //
    std::map<sutype::id_t,suWire*> _reservedGids;
    
  private:

    //! default constructor
    suWireManager ()
    {
      init_ ();

    } // end of suWireManager

  private:

    //! copy constructor
    suWireManager (const suWireManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suWireManager

    //! assignment operator
    suWireManager & operator = (const suWireManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suWireManager ();

  private:

    //! init all class members
    inline void init_ ()
    {
      _numCreatedWires = 0;
      _num_releases = 0;
      _debugid = -1;
      
    } // end of init_

    //! copy all class members
    inline void copy_ (const suWireManager & rs)
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
    static inline suWireManager * instance ()
    {
      if (suWireManager::_instance == 0)
        suWireManager::_instance = new suWireManager ();

      return suWireManager::_instance;

    } // end of instance

  public:

    //
    suWire * create_wire_from_dcoords                       (const suNet * net,
                                                             const suLayer * layer,
                                                             sutype::dcoord_t x1,
                                                             sutype::dcoord_t y1,
                                                             sutype::dcoord_t x2,
                                                             sutype::dcoord_t y2,
                                                             sutype::wire_type_t wiretype = sutype::wt_undefined);
    
    //
    suWire * create_wire_from_edge_side                     (const suNet * net,
                                                             const suLayer * layer,
                                                             sutype::dcoord_t edgel,
                                                             sutype::dcoord_t edgeh,
                                                             sutype::dcoord_t sidel,
                                                             sutype::dcoord_t sideh,
                                                             sutype::wire_type_t wiretype = sutype::wt_undefined);

    //
    suWire * create_wire_from_wire                          (const suNet * net,
                                                             const suWire * refwire,
                                                             sutype::wire_type_t wiretype = sutype::wt_undefined);
       
//     //
//     suWire * create_wire_from_wire_then_expand_and_subtrack (const suNet * net,
//                                                              const suWire * refwire,
//                                                              const suWire * wiretocover,
//                                                              sutype::wire_type_t wiretype = sutype::wt_undefined);

   //
    suWire * create_wire_from_wire_then_shift               (const suNet * net,
                                                             const suWire * refwire,
                                                             sutype::dcoord_t dx,
                                                             sutype::dcoord_t dy,
                                                             sutype::wire_type_t wiretype = sutype::wt_undefined);

    //
    sutype::svi_t get_wire_usage (const suWire * wire)
      const;

    //
    bool wire_is_obsolete (const suWire * wire)
      const;

    //
    bool wire_is_illegal (const suWire * wire)
      const;

    //
    void mark_wire_as_obsolete (const suWire * wire);

    //
    void mark_wire_as_illegal (const suWire * wire);

    //
    void keep_wire_as_illegal_if_it_is_really_illegal (const suWire * wire);
    
    //
    void release_wire (suWire * wire,
                       bool verbose = true);
    
    //
    inline void release_wires (sutype::wires_t & wires)
    {
      for (sutype::uvi_t i=0; i < wires.size(); ++i)
        release_wire (wires[i]);
      
      wires.clear();
      
    } // end of release_wires

    //
    inline void release_wires (std::vector<sutype::wires_t> & wires)
    {
      for (sutype::uvi_t i=0; i < wires.size(); ++i)
        release_wires (wires[i]);

      wires.clear();
      
    } // end of release_wires

    //
    void increment_wire_usage (suWire * wire);
    
    //
    void merge_wires (sutype::wires_t & wires);

    //
    inline void reserve_gid_for_wires (const sutype::ids_t & ids)
    {
      for (const auto & iter : ids) {
        sutype::id_t gid = iter;
        _reservedGids [gid] = 0;
      }
      
    } // end of reserve_gid_for_wires
    
    //
    void reserve_gid_for_wire (suWire * wire,
                               sutype::id_t gid,
                               bool checkIfPreReserved);
    
    //
    suWire * get_wire_by_reserved_gid (sutype::id_t gid)
      const;
    
    //
    void set_permanent_gid (suWire * wire,
                            sutype::id_t gid);
    
    //
    sutype::wires_t get_wires_by_permanent_gid (const sutype::ids_t & gids,
                                                const std::string & msg = "")
      const;
    
  private:

    //
    suWire * create_or_reuse_a_wire_ (const suNet * net,
                                      const suLayer * layer);
    
    //
    void merge_wires_ (sutype::wires_t & wires);

    //
    bool merge_two_wires_if_possible_to_the_first_wire_ (sutype::svi_t i,
                                                         sutype::svi_t k,
                                                         sutype::wires_t & wires);

    //
    void unregister_wire_ (suWire * wire0);

    //
    suWire * register_or_replace_wire_ (suWire * wire0,
                                        bool replace = true);
    
    //
    void init_wires_per_layers_ ();
    
    //
    void decrement_wire_usage_ (suWire * wire);

    //
    void release_wire_ (suWire * wire,
                        bool verbose);

    //
    void inherit_gid_ (suWire * wire1,
                       suWire * wire2);
    
  }; // end of class suWireManager

} // end of namespace amsr

#endif // _suWireManager_h_

// end of suWireManager.h

