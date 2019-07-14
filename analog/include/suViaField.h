// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Apr 10 13:18:12 2018

//! \file   suViaField.h
//! \brief  A header of the class suViaField.

#ifndef _suViaField_h_
#define _suViaField_h_

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
  class suViaField
  {
  private:

    //
    const suLayer * _layer;
    
    //
    sutype::viaoptions_t _viaOptions;

  public:

    //! custom constructor
    suViaField (const suLayer * l)
    {
      init_ ();
      
      _layer = l;
      
    } // end of suViaField

    //! default constructor
    suViaField ()
    {
      SUASSERT (false, "The method is not expected to be called.");
      
      init_ ();

    } // end of suViaField

  private:

    //! copy constructor
    suViaField (const suViaField & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suViaField

    //! assignment operator
    suViaField & operator = (const suViaField & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suViaField ()
    {
    } // end of ~suViaField

  private:

    //! init all class members
    inline void init_ ()
    {
      _layer = 0;
      
    } // end of init_

    //! copy all class members
    inline void copy_ (const suViaField & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

    //
    inline const suLayer * layer () const { return _layer; }
    
    //
    inline const sutype::viaoptions_t & viaOptions () const { return _viaOptions; }

    //
    inline sutype::viaoptions_t & viaOptions () { return _viaOptions; }
    
  public:

    //
    void add_via_option (const sutype::viaoption_t & vo);

    //
    std::string to_str ()
      const;
    
  private:

  }; // end of class suViaField

} // end of namespace amsr

#endif // _suViaField_h_

// end of suViaField.h

