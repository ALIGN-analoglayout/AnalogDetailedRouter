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
//! \date   Thu Oct 26 12:52:31 2017

//! \file   suGeneratorManager.h
//! \brief  A header of the class suGeneratorManager.

#ifndef _suGeneratorManager_h_
#define _suGeneratorManager_h_

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

  //! 
  class suGeneratorManager
  {
  private:

    //! static instance
    static suGeneratorManager * _instance;

  private:

    sutype::generators_tc _allgenerators;
    
    // per cut layer id
    std::vector<sutype::generators_tc> _generators;
    
  private:
    
    //! default constructor
    suGeneratorManager ()
    {
      init_ ();

    } // end of suGeneratorManager

  private:

    //! copy constructor
    suGeneratorManager (const suGeneratorManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suGeneratorManager

    //! assignment operator
    suGeneratorManager & operator = (const suGeneratorManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suGeneratorManager ();

  private:

    //! init all class members
    inline void init_ ()
    {
    } // end of init_

    //! copy all class members
    inline void copy_ (const suGeneratorManager & rs)
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
    static inline suGeneratorManager * instance ()
    {
      if (suGeneratorManager::_instance == 0)
        suGeneratorManager::_instance = new suGeneratorManager ();

      return suGeneratorManager::_instance;

    } // end of instance

  public:

    //
    void read_generator_file (const std::string & filename);

    //
    const sutype::generators_tc & get_generators (const suLayer * cutlayer)
      const;

    //
    const suGenerator * get_generator (const suLayer * cutlayer,
                                       sutype::dcoord_t cutw,
                                       sutype::dcoord_t cuth)
      const;
    
  private:

    //
    void add_generator_ (const suGenerator * generator);
    
  }; // end of class suGeneratorManager

} // end of namespace amsr

#endif // _suGeneratorManager_h_

// end of suGeneratorManager.h

