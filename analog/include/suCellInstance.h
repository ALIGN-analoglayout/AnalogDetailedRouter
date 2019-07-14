// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Mon Oct  9 15:09:58 2017

//! \file   suCellInstance.h
//! \brief  A header of the class suCellInstance.

#ifndef _suCellInstance_h_
#define _suCellInstance_h_

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
  class suCellInstance
  {
  private:

  public:

    //! default constructor
    suCellInstance ()
    {
      init_ ();

    } // end of suCellInstance

    //! copy constructor
    suCellInstance (const suCellInstance & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suCellInstance

    //! assignment operator
    suCellInstance & operator = (const suCellInstance & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suCellInstance ()
    {
    } // end of ~suCellInstance

  private:

    //! init all class members
    inline void init_ ()
    {
    } // end of init_

    //! copy all class members
    inline void copy_ (const suCellInstance & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

  public:

  }; // end of class suCellInstance

} // end of namespace amsr

#endif // _suCellInstance_h_

// end of suCellInstance.h

