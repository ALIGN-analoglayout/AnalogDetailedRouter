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
//! \date   Thu Oct  5 16:35:16 2017

//! \file   suOptionManager.h
//! \brief  A header of the class suOptionManager.

#ifndef _suOptionManager_h_
#define _suOptionManager_h_

// system includes

// std includes
#include <string>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suTokenOwner.h>

namespace amsr
{
  // classes
  class suTokenParser;

  //! 
  class suOptionManager : public suTokenOwner
  {
  private:
    
    //
    static suOptionManager * _instance;

    //
    static std::string _emptyString;
    
    //
    suTokenParser * _tokenParser;
    
  private:
    
    //! default constructor
    suOptionManager ()
    {
      init_ ();

    } // end of suOptionManager

    //! copy constructor
    suOptionManager (const suOptionManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suOptionManager

    //! assignment operator
    suOptionManager & operator = (const suOptionManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suOptionManager ();
    
  private:

    //! init all class members
    inline void init_ ()
    {
      _tokenParser = 0;

    } // end of init_

    //! copy all class members
    inline void copy_ (const suOptionManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

    //
    const suTokenParser * tokenParser () const { return _tokenParser; }

  public:

    // static methods
    //
    
    //
    static void delete_instance ();
    
    //
    static inline suOptionManager * instance ()
    {
      if (suOptionManager::_instance == 0)
        suOptionManager::_instance = new suOptionManager ();
      
      return suOptionManager::_instance;
      
    } // end of instance
    
  public:

    //
    void read_run_file (const std::string & filename);

    //
    void read_external_files ();
    
    //
    sutype::strings_t get_string_options (const std::string & optioname)
      const;
    
    //
    const std::string & get_string_option (const std::string & optioname)
      const;
    
    //
    const std::string & get_string_option (const std::string & optioname,
                                           const std::string & defaultValue)
      const;
    
    //
    bool get_boolean_option (const std::string & optioname,
                             const bool defaultValue = false)
      const;

    //
    int get_integer_option (const std::string & optioname,
                            const int defaultValue = 0)
      const;

    //
    float get_float_option (const std::string & optioname,
                            const float defaultValue = 0.0)
      const;
    
  private:

    //
    sutype::tokens_t get_option_tokens_ (const std::string & optioname)
      const;
    
    //
    const suToken * get_option_token_ (const std::string & optioname)
      const;

  }; // end of class suOptionManager

} // end of namespace amsr

#endif // _suOptionManager_h_

// end of suOptionManager.h

