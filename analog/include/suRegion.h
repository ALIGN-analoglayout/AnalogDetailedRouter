// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Thu Oct  5 13:46:46 2017

//! \file   suRegion.h
//! \brief  A header of the class suRegion.

#ifndef _suRegion_h_
#define _suRegion_h_

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
  class suGlobalRouter;
  class suGREdge;
  class suMetalTemplateInstance;
  
  //! 
  class suRegion
  {
    friend class suGlobalRouter;

  private:

    //
    static sutype::id_t _uniqueId;

    //
    suRectangle _bbox;

    //
    std::array <suGREdge *, 4> _gredges;
    
    //
    sutype::id_t _id;

    //
    int _col;
    
    //
    int _row;
    
  public:

    //! custom constructor
    suRegion (sutype::dcoord_t xl,
              sutype::dcoord_t yl,
              sutype::dcoord_t xh,
              sutype::dcoord_t yh,
              int c,
              int r)
              
    {
      init_ ();

      _bbox.xl (xl);
      _bbox.yl (yl);
      _bbox.xh (xh);
      _bbox.yh (yh);

      _col = c;
      _row = r;
      
    } // end of suRegion

  private:

    //! default constructor
    suRegion ()
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();

    } // end of suRegion

    //! copy constructor
    suRegion (const suRegion & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suRegion

    //! assignment operator
    suRegion & operator = (const suRegion & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suRegion ()
    {
      
    } // end of ~suRegion

  private:

    //! init all class members
    inline void init_ ()
    {
      _id = suRegion::_uniqueId;
      ++suRegion::_uniqueId;

      _col = -1;
      _row = -1;

      // side_t: w/e/n/s
      _gredges[0] = 0;
      _gredges[1] = 0;
      _gredges[2] = 0;
      _gredges[3] = 0;
      
    } // end of init_

    //! copy all class members
    inline void copy_ (const suRegion & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //

    //
    inline void set_edge (sutype::side_t side,
                          suGREdge * gredge)
    {
      _gredges [side] = gredge;
      
    } // end of set_edge


    // accessors (getters)
    //

    //
    inline sutype::id_t id () const { return _id; }

    //
    inline const suRectangle & bbox () const { return _bbox; }

    //
    inline int col () const { return _col; }
    
    //
    inline int row () const { return _row; }

    //
    inline suGREdge * get_edge (sutype::side_t side)
      const
    {
      return _gredges[side];
      
    } // end of get_edge
    
  public:

    //
    inline static sutype::id_t get_next_id () { return suRegion::_uniqueId; }

  public:
    
  private:
    
  }; // end of class suRegion

} // end of namespace amsr

#endif // _suRegion_h_

// end of suRegion.h

