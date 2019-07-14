// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Thu Oct  5 16:04:37 2017

//! \file   suCommandLineManager.h
//! \brief  A header of the class suCommandLineManager.

#ifndef _suCommandLineManager_h_
#define _suCommandLineManager_h_

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
  class suCommandLineManager
  {
  private:

    //
    static suCommandLineManager * _instance;

    //
    std::string _runFile;
    
  private:

    //! default constructor
    suCommandLineManager ()
    {
      init_ ();

    } // end of suCommandLineManager

    //! copy constructor
    suCommandLineManager (const suCommandLineManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suCommandLineManager

    //! assignment operator
    suCommandLineManager & operator = (const suCommandLineManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suCommandLineManager ()
    {
    } // end of ~suCommandLineManager

  private:

    //! init all class members
    inline void init_ ()
    {
    } // end of init_

    //! copy all class members
    inline void copy_ (const suCommandLineManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

    //
    inline const std::string & runFile () const { return _runFile; }
    
  public:

    // static methods
    //
    
    //
    static void delete_instance ();
    
    //
    static inline suCommandLineManager * instance ()
    {
      if (suCommandLineManager::_instance == 0) {
        suCommandLineManager::_instance = new suCommandLineManager ();
      }
      
      return suCommandLineManager::_instance;
      
    } // end of instance
    
  public:

    //
    void parse_options (const sutype::strings_t & strs);

  private:

    //
    const std::string & get_string_ (const sutype::strings_t & strs,
                                     sutype::uvi_t index)
      const;
    
  }; // end of class suCommandLineManager

} // end of namespace amsr

#endif // _suCommandLineManager_h_

// end of suCommandLineManager.h

