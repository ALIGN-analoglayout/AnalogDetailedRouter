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
//! \date   Thu Feb  1 12:50:29 2018

//! \file   suRuleManager.h
//! \brief  A header of the class suRuleManager.

#ifndef _suRuleManager_h_
#define _suRuleManager_h_

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
  class suRuleManager
  {
  private:

    //! static instance
    static suRuleManager * _instance;

    //
    std::vector <std::vector <std::vector <bool> > > _ruleIsDefined;

    //
    std::vector <std::vector <sutype::dcoords_t> > _ruleValue;
    
  private:

  private:

    //! default constructor
    suRuleManager ();

  private:

    //! copy constructor
    suRuleManager (const suRuleManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suRuleManager

    //! assignment operator
    suRuleManager & operator = (const suRuleManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suRuleManager ()
    {
    } // end of ~suRuleManager

  private:

    //! init all class members
    inline void init_ ()
    {
    } // end of init_

    //! copy all class members
    inline void copy_ (const suRuleManager & rs)
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
    static inline suRuleManager * instance ()
    {
      if (suRuleManager::_instance == 0)
        suRuleManager::_instance = new suRuleManager ();

      return suRuleManager::_instance;

    } // end of instance

  public:

    //
    bool rule_is_defined (sutype::rule_type_t rt,
                          const suLayer * layer1)
      const;

    //
    bool rule_is_defined (sutype::rule_type_t rt,
                          const suLayer * layer1,
                          const suLayer * layer2)
      
      const;
    
    //
    sutype::dcoord_t get_rule_value (sutype::rule_type_t rt,
                                     const suLayer * layer1)
      const;

    //
    sutype::dcoord_t get_rule_value (sutype::rule_type_t rt,
                                     const suLayer * layer1,
                                     const suLayer * layer2)
      const;
    
  private:

  }; // end of class suRuleManager

} // end of namespace amsr

#endif // _suRuleManager_h_

// end of suRuleManager.h

