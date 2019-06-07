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
//! \date   Mon Oct  9 12:12:49 2017

//! \file   suGlobalRoute.h
//! \brief  A header of the class suGlobalRoute.

#ifndef _suGlobalRoute_h_
#define _suGlobalRoute_h_

// system includes

// std includes
#include <string>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suRectangle.h>

namespace amsr
{
  // classes
  class suNet;
  class suLayer;
  
  //! 
  class suGlobalRoute
  {
  private:

    //
    sutype::id_t _gid;

    //
    sutype::id_t _samewas;

    //
    const suNet * _net;

    //
    const suLayer * _layer;

    //
    sutype::dcoord_t _minWireWidth;
    
    //
    suRectangle _bbox;

  public:

    //! custom constructor
    suGlobalRoute (const suNet * n,
                   const suLayer * l,
                   sutype::dcoord_t x1,
                   sutype::dcoord_t y1,
                   sutype::dcoord_t x2,
                   sutype::dcoord_t y2,
                   sutype::dcoord_t minwirewidth,
                   sutype::id_t globalid,
                   sutype::id_t sameWidthAsAnotherGid)
    {
      init_ ();
      
      _net = n;
      _layer = l;
      _bbox.xl (std::min (x1, x2));
      _bbox.xh (_bbox.xl() == x1 ? x2 : x1);
      _bbox.yl (std::min (y1, y2));
      _bbox.yh (_bbox.yl() == y1 ? y2 : y1);

      SUASSERT (!_bbox.is_point(), "");
      SUASSERT (_bbox.is_line(), "");
      SUASSERT (_bbox.xl() >= 0, "");
      SUASSERT (_bbox.yl() >= 0, "");

      _minWireWidth = minwirewidth;
      _gid          = globalid;
      _samewas      = sameWidthAsAnotherGid;

    } // end of suGlobalRoute
    
  private:

    //! default constructor
    suGlobalRoute ()
    {
      init_ ();
      
    } // end of suGlobalRoute

    //! copy constructor
    suGlobalRoute (const suGlobalRoute & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suGlobalRoute

    //! assignment operator
    suGlobalRoute & operator = (const suGlobalRoute & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:
    
    //! destructor
    virtual ~suGlobalRoute ()
    {
    } // end of ~suGlobalRoute

  private:

    //! init all class members
    inline void init_ ()
    {
      _gid = sutype::UNDEFINED_GLOBAL_ID;
      _samewas = sutype::UNDEFINED_GLOBAL_ID;
      _net = 0;
      _layer = 0;
      _minWireWidth = 0;
      
    } // end of init_

    //! copy all class members
    inline void copy_ (const suGlobalRoute & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //
    
    //
    inline void minWireWidth (sutype::dcoord_t v) { _minWireWidth = v; }
    
    // accessors (getters)
    //

    //
    inline sutype::id_t gid () const { return _gid; }

    //
    inline sutype::id_t sameWidthAsAnotherGid () const { return _samewas; }

    //
    inline const suNet * net () const { return _net; }

    //
    inline const suLayer * layer () const { return _layer; }

    //
    inline const suRectangle & bbox () const { return _bbox; }

    //
    inline suRectangle & bbox () { return _bbox; }
    
    //
    inline sutype::dcoord_t minWireWidth () const { return _minWireWidth; }

  public:

    //
    inline bool may_have_the_same_width_as_another_gr () const { return (_gid != sutype::UNDEFINED_GLOBAL_ID || _samewas != sutype::UNDEFINED_GLOBAL_ID); }

    //
    inline bool must_have_the_same_width_as_another_gr (const suGlobalRoute * gr1)
      const
    {
      return ((_gid     != sutype::UNDEFINED_GLOBAL_ID && _gid     == gr1->_samewas) ||
              (_samewas != sutype::UNDEFINED_GLOBAL_ID && _samewas == gr1->_gid));
      
    } // end of must_have_the_same_width_as_another_gr

    // clone only net, layer, bbox
    suGlobalRoute * create_light_clone ()
      const;
    
    //
    sutype::regions_t get_regions ()
      const;

    //
    int length_in_regions ()
      const;

    //
    int width_in_regions ()
      const;
    
    //
    std::string to_str ()
      const;

    // \return true if changed
    bool change_layer (bool upper);
    
  }; // end of class suGlobalRoute

} // end of namespace amsr

#endif // _suGlobalRoute_h_

// end of suGlobalRoute.h

