// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Wed Oct 18 16:12:46 2017

//! \file   suPoint.h
//! \brief  A header of the class suPoint.

#ifndef _suPoint_h_
#define _suPoint_h_

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
  class suPoint
  {
  private:

    // x, y
    std::array <sutype::dcoord_t, 2> _dcoords;

  public:

    //! custom constructor
    suPoint (sutype::dcoord_t dx,
             sutype::dcoord_t dy)
    {
      init_ ();

      x (dx);
      y (dy);

    } // end of suPoint

    //! default constructor
    suPoint ()
    {
      init_ ();

    } // end of suPoint

  private:

    //! copy constructor
    suPoint (const suPoint & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");
      
      init_ ();
      copy_ (rs);

    } // end of suPoint

    //! assignment operator
    suPoint & operator = (const suPoint & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suPoint ()
    {
    } // end of ~suPoint

  private:

    //! init all class members
    inline void init_ ()
    {
      _dcoords[0] = _dcoords[1] = 0;

    } // end of init_

    //! copy all class members
    inline void copy_ (const suPoint & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");
      
      _dcoords[0] = rs._dcoords[0];
      _dcoords[1] = rs._dcoords[1];

    } // end of copy_

  public:

    // accessors (setters)
    //

    // 
    void x (sutype::dcoord_t v) { _dcoords [sutype::pc_x] = v; }
    void y (sutype::dcoord_t v) { _dcoords [sutype::pc_y] = v; }


    // accessors (getters)
    //

    //
    sutype::dcoord_t x () const { return _dcoords [sutype::pc_x]; }
    sutype::dcoord_t y () const { return _dcoords [sutype::pc_y]; }

  public:

  private:

  }; // end of class suPoint

} // end of namespace amsr

#endif // _suPoint_h_

// end of suPoint.h

