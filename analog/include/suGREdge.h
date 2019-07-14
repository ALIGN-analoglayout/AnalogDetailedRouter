// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Mon May  6 14:16:28 2019

//! \file   suGREdge.h
//! \brief  A header of the class suGREdge.

#ifndef _suGREdge_h_
#define _suGREdge_h_

// system includes

// std includes

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suRectangle.h>

namespace amsr
{
  // classes

  //! 
  class suGREdge
  {
  private:

    //
    suRectangle _rect;

    //
    sutype::grid_orientation_t _orientation;

    //
    sutype::dcoord_t _demand;

    //
    sutype::dcoord_t _capacity;

  public:

    //! default constructor
    suGREdge ()
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();

    } // end of suGREdge

    //! custom constructor
    suGREdge (sutype::dcoord_t xl,
              sutype::dcoord_t yl,
              sutype::dcoord_t xh,
              sutype::dcoord_t yh,
              sutype::grid_orientation_t orientation)
    {
      init_ ();

      _rect.xl (xl);
      _rect.yl (yl);
      _rect.xh (xh);
      _rect.yh (yh);

      _orientation = orientation;
      
    } // end of suGREdge
    

  private:

    //! copy constructor
    suGREdge (const suGREdge & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suGREdge

    //! assignment operator
    suGREdge & operator = (const suGREdge & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suGREdge ()
    {
    } // end of ~suGREdge

  private:

    //! init all class members
    inline void init_ ()
    {
      _orientation = sutype::go_ver;
      _demand = 0;
      _capacity = 0;
      
    } // end of init_

    //! copy all class members
    inline void copy_ (const suGREdge & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

    //
    inline bool is_ver () const { return (_orientation == sutype::go_ver); }

    //
    inline bool is_hor () const { return (_orientation == sutype::go_hor); }

  public:

    //
    void calculate_capacity ();

  private:

  }; // end of class suGREdge

} // end of namespace amsr

#endif // _suGREdge_h_

// end of suGREdge.h

