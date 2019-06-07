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
//! \date   Mon Oct 30 14:04:16 2017

//! \file   suColoredLayer.h
//! \brief  A header of the class suColoredLayer.

#ifndef _suColoredLayer_h_
#define _suColoredLayer_h_

// system includes

// std includes

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suLayer.h>

namespace amsr
{
  // classes

  //! 
  class suColoredLayer : public suLayer
  {
    
  private:

    //
    const suLayer * _base;

    // type may be a subject to change
    std::string _type;

  public:

    //! custom constructor
    suColoredLayer (const suLayer * b,
                    const std::string & t,
                    const std::string & n)
    {
      init_ ();

      _base = b;
      _type = t;
      _name = n;

      SUASSERT (_base, "");
      SUASSERT (_base->is_base(), "");
      SUASSERT (!_type.empty(), "");
      SUASSERT (!_name.empty(), "");
      
    } // end of suColoredLayer
    
  private:

    //! default constructor
    suColoredLayer ()
    {
      SUASSERT (false, "The method is not expected to be called.");
      
      init_ ();

    } // end of suColoredLayer

    //! copy constructor
    suColoredLayer (const suColoredLayer & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suColoredLayer

    //! assignment operator
    suColoredLayer & operator = (const suColoredLayer & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suColoredLayer ()
    {
    } // end of ~suColoredLayer

  private:

    //! init all class members
    inline void init_ ()
    {
      _base = 0;
      
    } // end of init_
    
    //! copy all class members
    inline void copy_ (const suColoredLayer & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //

    // accessors (getters)
    //

    //
    const std::string & color_type () const { return _type; }
    
    // virtual accessors (getters)
    //

    // virtual
    inline bool is_base () const { return false; }

    // virtual
    inline bool is_color () const { return true; }
    
    // virtual
    inline const suLayer * base () const { return _base; }

    // virtual
    inline const suColoredLayer * to_color () const { return this; }

    // virtual
    inline sutype::bitid_t type () const { return _base->type(); }
    
    // virtual
    inline int level () const { return _base->level(); }

    // virtual
    inline sutype::id_t base_id () const { return _base->base_id(); }

    // virtual
    inline bool has_pgd () const { return _base->has_pgd(); }
    
    // virtual
    inline sutype::grid_orientation_t pgd () const { return _base->pgd(); }

    // virtual
    inline sutype::grid_orientation_t ogd () const { return _base->ogd(); }
    
    // virtual
    inline bool is (sutype::bitid_t t) const { return _base->is (t); }
    
    // virtual
    inline bool is_electrically_connected_with (const suLayer * layer) const { return _base->is_electrically_connected_with (layer); }

    // virtual
    inline const sutype::layers_tc & electricallyConnected () const { return _base->electricallyConnected(); }
    
  public:
    
  private:

  }; // end of class suColoredLayer

} // end of namespace amsr

#endif // _suColoredLayer_h_

// end of suColoredLayer.h

