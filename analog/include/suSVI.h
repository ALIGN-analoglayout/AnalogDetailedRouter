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
//! \date   Wed Oct 24 13:49:28 2018

//! \file   suSVI.h
//! \brief  A header of the class suSVI.

#ifndef _suSVI_h_
#define _suSVI_h_

// system includes

// std includes
#include <string>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// SVI interface
#ifdef _ENABLE_SVI_

#include "svMadFox.h"

#define _SVI_MANAGER_ madfox::svManager

#else // _ENABLE_SVI_

#define _SVI_MANAGER_ void

#endif // _ENABLE_SVI_

namespace amsr
{
  // classes
  class suLayer;
  class suWire;

  //! 
  class suSVI
  {
  private:

    //! static instance
    static suSVI * _instance;
    
    // hardcoded colors
    static sutype::strings_t _mColors;
    static sutype::strings_t _vColors;
    
  private:

  private:

    //! default constructor
    suSVI ()
    {
      init_ ();

    } // end of suSVI

  private:

    //! copy constructor
    suSVI (const suSVI & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);
      
    } // end of suSVI

    //! assignment operator
    suSVI & operator = (const suSVI & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suSVI ();

  private:

    //! init all class members
    inline void init_ ()
    {
      suSVI::init_static_vars_ ();
      
    } // end of init_
    
    //! copy all class members
    inline void copy_ (const suSVI & rs)
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
    static inline suSVI * instance ()
    {
      if (suSVI::_instance == 0)
        suSVI::_instance = new suSVI ();
      
      return suSVI::_instance;

    } // end of instance

  public:

    //
    void dump_xml_file (const std::string & filename)
      const;

  private:

    //
    static void init_static_vars_ ();
    
    //
    void prepare_header_ (_SVI_MANAGER_ * manager)
      const;

    //
    void add_wires_ (_SVI_MANAGER_ * manager)
      const;

    //
    void add_wire_ (_SVI_MANAGER_ * svmanager,
                    const suWire * wire)
      const;

    //
    std::string auto_detect_color_for_layer_ (const suLayer * layer)
      const;
    
  }; // end of class suSVI

} // end of namespace amsr

#endif // _suSVI_h_

// end of suSVI.h

