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
//! \date   Thu Oct 26 12:43:06 2017

//! \file   suGenerator.h
//! \brief  A header of the class suGenerator.

#ifndef _suGenerator_h_
#define _suGenerator_h_

// system includes

// std includes
#include <string>

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
  class suRectangle;
  class suWire;

  //! 
  class suGenerator
  {
  private:

    //
    static sutype::id_t _uniqueId;

    //
    std::string _name;

    //
    std::array <sutype::wires_t,   sutype::vgl_num_types> _wires;
    
    //
    std::array <const suLayer *,   sutype::vgl_num_types> _layers;
    
    // cut layer doesn't use it
    // no widths is OK; it means that the generator matches any width
    std::array <sutype::dcoords_t, sutype::vgl_num_types> _widths;

    //
    sutype::id_t _id;
    
  public:

    //! custom constructor
    suGenerator (const std::string & n)
    {
      init_ ();

      _name = n;
      
    } // end of suGenerator
    
  private:

    //! default constructor
    suGenerator ()
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();

    } // end of suGenerator

    //! copy constructor
    suGenerator (const suGenerator & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suGenerator

    //! assignment operator
    suGenerator & operator = (const suGenerator & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suGenerator ();

  private:

    //! init all class members
    inline void init_ ()
    {
      _id = suGenerator::_uniqueId;
      ++suGenerator::_uniqueId;

      for (int i=0; i < (int)sutype::vgl_num_types; ++i) {
        _layers [i] = 0;
      }

    } // end of init_

    //! copy all class members
    inline void copy_ (const suGenerator & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

    //
    inline const std::string & name () const { return _name; }
    
    //
    inline sutype::id_t id () const { return _id; }
    
  public:

    //
    void add_wire (suWire * wire);
    
    //
    void set_layer (sutype::via_generator_layer_t vgl,
                    const suLayer * layer);

    //
    inline const suLayer * get_cut_layer () const { return get_layer (sutype::vgl_cut); }
    
    //
    inline const suLayer * get_layer (sutype::via_generator_layer_t vgl)
      const
    {
      SUASSERT (vgl == sutype::vgl_cut || vgl == sutype::vgl_layer1 || vgl == sutype::vgl_layer2, "");

      return _layers [vgl];
      
    } // end of get_layer
    
    //
    inline void add_width (sutype::via_generator_layer_t vgl,
                           sutype::dcoord_t width)
    {
      SUASSERT (vgl == sutype::vgl_layer1 || vgl == sutype::vgl_layer2, "");
      
      _widths [vgl].push_back (width);
      
    } // end of add_width

    //
    inline const sutype::dcoords_t & get_widths (sutype::via_generator_layer_t vgl)
      const
    {
      SUASSERT (vgl == sutype::vgl_layer1 || vgl == sutype::vgl_layer2, "");

      return _widths [vgl];
      
    } // end of get_widths

    //
    void make_canonical ();

    //
    bool matches (const suLayer * layer1,
                  const suLayer * layer2,
                  sutype::dcoord_t width1,
                  sutype::dcoord_t width2)
      const;

    //
    void clone_wires (sutype::wires_t & wires,
                      const suNet * net,
                      sutype::dcoord_t dx,
                      sutype::dcoord_t dy,
                      sutype::wire_type_t cutwiretype)
      const;

    //
    suRectangle get_shape (sutype::dcoord_t dx,
                           sutype::dcoord_t dy,
                           sutype::via_generator_layer_t vgl = sutype::vgl_cut)
      const;
    
    //
    sutype::dcoord_t get_shape_width (sutype::via_generator_layer_t vgl = sutype::vgl_cut)
      const;

    //
    sutype::dcoord_t get_shape_height (sutype::via_generator_layer_t vgl = sutype::vgl_cut)
      const;
    
    //
    struct cmp_ptr
    {
      inline bool operator() (suGenerator * a, suGenerator * b)
        const
      {
        return (a->id() < b->id());
      }
    };

    //
    struct cmp_const_ptr
    {
      inline bool operator() (const suGenerator * a, const suGenerator * b)
        const
      {
        return (a->id() < b->id());
      }
    };
    
  private:

    
    //
    const suWire * get_wire_ (sutype::via_generator_layer_t vgl)
      const;


  }; // end of class suGenerator

} // end of namespace amsr

#endif // _suGenerator_h_

// end of suGenerator.h

