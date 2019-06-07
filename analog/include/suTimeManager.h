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
//! \date   Wed Oct  4 11:42:49 2017

//! \file   suTimeManager.h
//! \brief  A header of the class suTimeManager.

#ifndef _suTimeManager_h_
#define _suTimeManager_h_

// system includes
#include <time.h>

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

  //! 
  class suTimeManager
  {
  private:

    //
    static suTimeManager * _instance;
    
    // made static not to call constructor/destructor for this object every time
    static timespec _timeSpec;
    
    //
    double _startCpuTime;
    
  public:

    //! default constructor
    suTimeManager ();
    
    //! copy constructor
    suTimeManager (const suTimeManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suTimeManager

    //! assignment operator
    suTimeManager & operator = (const suTimeManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suTimeManager ()
    {
    } // end of ~suTimeManager

  private:

    //! init all class members
    inline void init_ ()
    {
      _startCpuTime = 0.0;

    } // end of init_

    //! copy all class members
    inline void copy_ (const suTimeManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //

  public:

    // accessors (getters)
    //
    
    // static methods
    //
    
    //
    static void create_instance ();

    //
    static void delete_instance ();
    
    //
    static inline suTimeManager * instance ()
    {
      return suTimeManager::_instance;
      
    } // end of instance
    
  public:

    double get_cpu_time ()
      const;
    
  }; // end of class suTimeManager

} // end of namespace amsr

#endif // _suTimeManager_h_

// end of suTimeManager.h

