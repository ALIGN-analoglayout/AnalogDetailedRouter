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
//! \date   Thu Oct 12 10:24:54 2017

//! \file   suMetalTemplate.h
//! \brief  A header of the class suMetalTemplate.

#ifndef _suMetalTemplate_h_
#define _suMetalTemplate_h_

// system includes

// std includes
#include <string>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes

namespace amsr
{
  // classes
  class suLayer;
  class suMetalTemplateManager;

  //! 
  class suMetalTemplate
  {
    friend class suMetalTemplateManager;

  private:

    //
    static sutype::id_t _uniqueId;

    //
    std::string _name;

    //
    const suLayer * _baseLayer;

    // vertical tracks are enumerated from west to east
    // horizontal tracks are enumerated from south to north
    // inverted date is precalculated used for Metal Template Instances, but anyway even "inverted"
    //   vertical tracks are enumerated from west to east
    //   horizontal tracks are enumerated from south to north
    
    // number of layers, widths, spaces, and colors must be equal
    sutype::layers_tc _layers [2]; // 0 = direct, 1 = inverted
    sutype::dcoords_t _widths [2]; // 0 = direct, 1 = inverted
    sutype::dcoords_t _spaces [2]; // 0 = direct, 1 = inverted
    sutype::strings_t _colors [2]; // 0 = direct, 1 = inverted
    sutype::dcoords_t _offset [2]; // 0 = direct, 1 = inverted
    
    sutype::dcoords_t _stops; // repeating set of wire lengths
    
    // pgd/ogd period
    sutype::dcoord_t _dperiod [2];
    
    //
    sutype::id_t _id;

  public:

    //! default constructor
    suMetalTemplate ()
    {
      init_ ();

    } // end of suMetalTemplate

  private:

    //! copy constructor
    suMetalTemplate (const suMetalTemplate & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suMetalTemplate

    //! assignment operator
    suMetalTemplate & operator = (const suMetalTemplate & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suMetalTemplate ()
    {
    } // end of ~suMetalTemplate

  private:

    //! init all class members
    inline void init_ ()
    {
      _id = suMetalTemplate::_uniqueId;
      ++suMetalTemplate::_uniqueId;

      SUASSERT ((int)sutype::gd_pgd == 0, "");
      SUASSERT ((int)sutype::gd_ogd == 1, "");
      
      _dperiod[sutype::gd_pgd] = 0;
      _dperiod[sutype::gd_ogd] = 0;
      
    } // end of init_

    //! copy all class members
    inline void copy_ (const suMetalTemplate & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

    //
    inline sutype::id_t id () const { return _id; }

    //
    inline const std::string & name () const { return _name; }
    
    //
    inline const suLayer * baseLayer () const { return _baseLayer; }
    
    //
    inline sutype::dcoord_t dperiod (sutype::grid_direction_t gd) const { return _dperiod [gd]; }

    //
    inline sutype::dcoord_t pitch         () const { return dperiod (sutype::gd_pgd); }
    inline sutype::dcoord_t lineEndPeriod () const { return dperiod (sutype::gd_ogd); }
    
    //
    inline sutype::dcoords_t stops () const { return _stops; }
    
  public:
    
    //
    inline const sutype::layers_tc & get_layers (bool inverted) const { return _layers [int(inverted)]; }

    //
    inline const sutype::dcoords_t & get_widths (bool inverted) const { return _widths [int(inverted)]; }
    
    //
    inline const sutype::dcoords_t & get_spaces (bool inverted) const { return _spaces [int(inverted)]; }

    //
    inline const sutype::strings_t & get_colors (bool inverted) const { return _colors [int(inverted)]; }
    
    // \return minimal positive offset
    sutype::dcoord_t convert_to_canonical_offset (sutype::grid_direction_t gd,
                                                  sutype::dcoord_t shift)
      const;

    //
    inline sutype::dcoord_t get_track_offset (sutype::svi_t index,
                                              bool inverted)
      const
    {
      SUASSERT (index >= 0, "");
      SUASSERT (index < (sutype::svi_t)_offset[int(inverted)].size(), "");
      
      return _offset[inverted][index];
      
    } // end of get_track_offset
    
  private:
    
    //
    inline sutype::layers_tc & get_layers_ (bool inverted) { return _layers [int(inverted)]; }

    //
    inline sutype::dcoords_t & get_widths_ (bool inverted) { return _widths [int(inverted)]; }
    
    //
    inline sutype::dcoords_t & get_spaces_ (bool inverted) { return _spaces [int(inverted)]; }

    //
    inline sutype::strings_t & get_colors_ (bool inverted) { return _colors [int(inverted)]; }

    //
    inline sutype::dcoords_t & get_offset_ (bool inverted) { return _offset [int(inverted)]; }
    

  }; // end of class suMetalTemplate

} // end of namespace amsr

#endif // _suMetalTemplate_h_

// end of suMetalTemplate.h

