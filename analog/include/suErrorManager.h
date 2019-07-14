// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Aug 21 14:52:23 2018

//! \file   suErrorManager.h
//! \brief  A header of the class suErrorManager.

#ifndef _suErrorManager_h_
#define _suErrorManager_h_

// system includes

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
  class suErrorManager
  {
  private:

    //! static instance
    static suErrorManager * _instance;

    //
    sutype::errors_t _errors;

    //
    sutype::dcoord_t _conversionFactor;

    //
    std::string _fileExtension;

    //
    std::string _cellName;

  private:

  private:

    //! default constructor
    suErrorManager ()
    {
      init_ ();

    } // end of suErrorManager

  private:

    //! copy constructor
    suErrorManager (const suErrorManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suErrorManager

    //! assignment operator
    suErrorManager & operator = (const suErrorManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suErrorManager ();

  private:

    //! init all class members
    inline void init_ ()
    {
      _conversionFactor = 10000;
      _fileExtension = "playerr";
      _cellName = "";

    } // end of init_

    //! copy all class members
    inline void copy_ (const suErrorManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //

    //
    inline void cellName (const std::string & v) { _cellName = v; }

    // accessors (getters)
    //

  public:

    // static methods
    //

    //
    static void delete_instance ();
    
    //
    static inline suErrorManager * instance ()
    {
      if (suErrorManager::_instance == 0)
        suErrorManager::_instance = new suErrorManager ();

      return suErrorManager::_instance;

    } // end of instance

  public:

    //
    void add_error (const std::string & errorname,
                    const suRectangle & rect);
    
    //
    void add_error (const std::string & errorname,
                    sutype::dcoord_t x1,
                    sutype::dcoord_t y1,
                    sutype::dcoord_t x2,
                    sutype::dcoord_t y2);

    //
    void dump_error_file (const std::string & dirname)
      const;

  private:

  }; // end of class suErrorManager

} // end of namespace amsr

#endif // _suErrorManager_h_

// end of suErrorManager.h

