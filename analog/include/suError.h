// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Aug 21 14:56:00 2018

//! \file   suError.h
//! \brief  A header of the class suError.

#ifndef _suError_h_
#define _suError_h_

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
  class suError
  {
  private:

    //
    std::string _name;

    //
    std::string _type;

    //
    sutype::strings_t _descriptions;
    
    //
    sutype::rectangles_t _rectangles;
    
  public:

    //! default constructor
    suError ()
    {
      SUASSERT (false, "The method is not expected to be called.");
      
      init_ ();

    } // end of suError

    //! custom constructor
    suError (const std::string & n)
    {
      init_ ();

      _name = n;

      _descriptions.push_back (n);
      
    } // end of suError

  private:

    //! copy constructor
    suError (const suError & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suError

    //! assignment operator
    suError & operator = (const suError & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suError ();

  private:

    //! init all class members
    inline void init_ ()
    {
      // I don't know other types
      _type = "MINC";

    } // end of init_

    //! copy all class members
    inline void copy_ (const suError & rs)
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
    inline const std::string & type () const { return _type; }

    //
    inline const sutype::strings_t & descriptions () const { return _descriptions; }

    //
    inline const sutype::rectangles_t & rectangles () const { return _rectangles; }
    
  public:

    //
    void add_rectangle (sutype::dcoord_t x1,
                        sutype::dcoord_t y1,
                        sutype::dcoord_t x2,
                        sutype::dcoord_t y2);
    
  private:

  }; // end of class suError

} // end of namespace amsr

#endif // _suError_h_

// end of suError.h

