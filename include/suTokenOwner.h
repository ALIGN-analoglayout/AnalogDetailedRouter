// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Oct 10 14:35:30 2017

//! \file   suTokenOwner.h
//! \brief  A header of the class suTokenOwner.

#ifndef _suTokenOwner_h_
#define _suTokenOwner_h_

// system includes

// std includes

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suToken.h>

namespace amsr
{
  // classes

  //! 
  class suTokenOwner
  {
    protected:

    const suToken * _token;
    
  public:
    
    //! default constructor
    suTokenOwner ()
    {
      init_ ();

    } // end of suTokenOwner

    //! copy constructor
    suTokenOwner (const suTokenOwner & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suTokenOwner

    //! assignment operator
    suTokenOwner & operator = (const suTokenOwner & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suTokenOwner ()
    {
    } // end of ~suTokenOwner

  private:

    //! init all class members
    inline void init_ ()
    {
      _token = 0;
      
    } // end of init_

    //! copy all class members
    inline void copy_ (const suTokenOwner & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //

    //
    void token (const suToken * v) { _token = v; }


    // accessors (getters)
    //

    //
    const suToken * token () const { return _token; }
    
  public:

  }; // end of class suTokenOwner

} // end of namespace amsr

#endif // _suTokenOwner_h_

// end of suTokenOwner.h

