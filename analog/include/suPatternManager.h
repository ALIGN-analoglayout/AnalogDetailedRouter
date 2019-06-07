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
//! \date   Tue Nov 14 12:04:24 2017

//! \file   suPatternManager.h
//! \brief  A header of the class suPatternManager.

#ifndef _suPatternManager_h_
#define _suPatternManager_h_

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
  class suGenerator;
  class suGrid;
  class suLayer;
  class suPattern;
  class suPatternInstance;
  class suWire;

  //! 
  class suPatternManager
  {
  private:

    //! static instance
    static suPatternManager * _instance;

    //
    sutype::patterns_tc _patterns;

    //
    std::map<const suLayer *, sutype::patterns_tc> _patternsPerLayer;
    
    //
    std::vector<sutype::patterninstances_tc> _patternIdToInstances;

    //
    sutype::wires_t _wires;

  private:

  private:

    //! default constructor
    suPatternManager ()
    {
      init_ ();

    } // end of suPatternManager

  private:

    //! copy constructor
    suPatternManager (const suPatternManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suPatternManager

    //! assignment operator
    suPatternManager & operator = (const suPatternManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suPatternManager ();

  private:

    //! init all class members
    inline void init_ ()
    {
      _wires.resize (1024);
      _wires.clear ();
      _wires.push_back (0);

    } // end of init_

    //! copy all class members
    inline void copy_ (const suPatternManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

    //
    inline const sutype::patterns_tc & patterns () const { return _patterns; }

    //
    inline const sutype::wires_t & wires () const { return _wires; }

    //
    inline const std::vector<sutype::patterninstances_tc> & patternIdToInstances () const { return _patternIdToInstances; }

    //
    inline suWire * get_wire (sutype::uvi_t i)
      const
    {
      SUASSERT (i > 0 && i < _wires.size(), "");
      return _wires [i];
      
    } // end of get_wire
    
  public:

    // static methods
    //

    //
    static void delete_instance ();
    
    //
    static inline suPatternManager * instance ()
    {
      if (suPatternManager::_instance == 0)
        suPatternManager::_instance = new suPatternManager ();

      return suPatternManager::_instance;

    } // end of instance

  public:

    //
    void create_raw_pattern_instances (const suWire * wire);

    //
    void delete_incomplete_pattern_instances ();

    //
    void delete_redundant_pattern_instances ();
    
    //
    inline sutype::uvi_t num_pattern_instances ()
      const
    {
      sutype::uvi_t num = 0;
      
      for (auto iter : _patternIdToInstances) {
        num += iter.size();
      }

      return num;
      
    } // end of num_pattern_instances

    //
    void read_pattern_file (const std::string & filename);
    
    //
    sutype::satindex_t create_wire_index_for_via_cut (const suGrid * grid,
                                                      const suGenerator * generator,
                                                      sutype::gcoord_t gx,
                                                      sutype::gcoord_t gy);
    
    //
    sutype::satindex_t create_wire_index_for_via_cut (const suGenerator * generator,
                                                      sutype::dcoord_t dx,
                                                      sutype::dcoord_t dy);

    //
    sutype::satindex_t create_wire_index_for_transformation (const suWire * wire,
                                                             const sutype::tr_t & tr);

    //
    sutype::satindex_t create_wire_index_for_dcoords (const suLayer * layer,
                                                      sutype::dcoord_t xl,
                                                      sutype::dcoord_t yl,
                                                      sutype::dcoord_t xh,
                                                      sutype::dcoord_t yh);
    
  private:

    //
    sutype::satindex_t add_a_wire_ (suWire * wire);

    //
    void parse_token_as_pattern_node_ (suPattern * pattern,
                                       const suToken * token,
                                       suLayoutFunc * parentFunc);
        
  }; // end of class suPatternManager

} // end of namespace amsr

#endif // _suPatternManager_h_

// end of suPatternManager.h

