// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Mon Oct 16 14:44:32 2017

//! \file   suRectangle.cpp
//! \brief  A collection of methods of the class suRectangle.

// std includes
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suStatic.h>

// module include
#include <suRectangle.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  sutype::rect_corner_t suRectangle::_gridDirection2Edge [2][2] = {{sutype::rc_yl, sutype::rc_yh}, {sutype::rc_xl, sutype::rc_xh}};
  sutype::rect_corner_t suRectangle::_gridDirection2Side [2][2] = {{sutype::rc_xl, sutype::rc_xh}, {sutype::rc_yl, sutype::rc_yh}};
  sutype::rect_corner_t suRectangle::_gridDirection2Coor [2][2] = {{sutype::rc_yl, sutype::rc_yh}, {sutype::rc_xl, sutype::rc_xh}}; // coor == edge
  
  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------


  // ------------------------------------------------------------
  // -
  // --- Public static methods
  // -
  // ------------------------------------------------------------


  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  //
  bool suRectangle::has_at_least_common_line (const suRectangle & rect)
    const
  {
    sutype::dcoord_t bboxxl = std::min (xl(), rect.xl());
    sutype::dcoord_t bboxxh = std::max (xh(), rect.xh());
    sutype::dcoord_t bboxyl = std::min (yl(), rect.yl());
    sutype::dcoord_t bboxyh = std::max (yh(), rect.yh());

    sutype::dcoord_t bboxw = bboxxh - bboxxl;
    sutype::dcoord_t bboxh = bboxyh - bboxyl;
    
    sutype::dcoord_t totlw = w() + rect.w();
    sutype::dcoord_t totlh = h() + rect.h();

    return ((bboxw <  totlw && bboxh <  totlh) ||
            (bboxw <= totlw && bboxh <  totlh) ||
            (bboxw <  totlw && bboxh <= totlh));
    
  } // end of suRectangle::has_at_least_common_line

  //
  bool suRectangle::has_a_common_rect (const suRectangle & rect)
    const
  {
    sutype::dcoord_t bboxxl = std::min (xl(), rect.xl());
    sutype::dcoord_t bboxxh = std::max (xh(), rect.xh());
    sutype::dcoord_t bboxyl = std::min (yl(), rect.yl());
    sutype::dcoord_t bboxyh = std::max (yh(), rect.yh());

    sutype::dcoord_t bboxw = bboxxh - bboxxl;
    sutype::dcoord_t bboxh = bboxyh - bboxyl;
    
    sutype::dcoord_t totlw = w() + rect.w();
    sutype::dcoord_t totlh = h() + rect.h();
    
    return (bboxw < totlw && bboxh < totlh);
    
  } // end of suRectangle::has_a_common_rect
  
  // gd=sutype::go_ver --> distance between y-coords
  // gd=sutype::go_hor --> distance between x-coords
  sutype::dcoord_t suRectangle::distance_to (const suRectangle & rect,
                                             sutype::grid_orientation_t gd)
    const
  {
    sutype::dcoord_t cl1 = coorl (gd);
    sutype::dcoord_t ch1 = coorh (gd);
    sutype::dcoord_t cl2 = rect.coorl (gd);
    sutype::dcoord_t ch2 = rect.coorh (gd);

    if        (ch1 < cl2) { return (cl2 - ch1); 
    } else if (ch2 < cl1) { return (cl1 - ch2);
    } else {
      return 0;
    }
    
  } // end of distance_to suRectangle::distance_to

  //
  void suRectangle::subtrack (const suRectangle & rect,
                              bool assertOnError)
  {
    if (yl() == rect.yl() && yh() == rect.yh()) {

      if        (rect.xl() <= xl() && rect.xh() < xh()) { xl (rect.xh());
      } else if (rect.xh() >= xh() && rect.xl() > xl()) { xh (rect.xl());
      } else {
        if (assertOnError) { SUASSERT (false, "Unsupported combination of rectangles: this=" << to_str() << "; rect=" << rect.to_str()); }
        return;
      }
    }
    else if (xl() == rect.xl() && xh() == rect.xh()) {

      if        (rect.yl() <= yl() && rect.yh() < yh()) { yl (rect.yh());
      } else if (rect.yh() >= yh() && rect.yl() > yl()) { yh (rect.yl());
      } else {
        if (assertOnError) { SUASSERT (false, "Unsupported combination of rectangles: this=" << to_str() << "; rect=" << rect.to_str()); }
        return;
      }
    }
    else {
      // always assert
      SUASSERT (false, "Unsupported combination of rectangles: this=" << to_str() << "; rect=" << rect.to_str());
    }
      
  } // end of suRectangle::subtrack

  //
  void suRectangle::apply_transformation (const sutype::tr_t & tr)
  {
    const sutype::dcoord_t dx  = suStatic::get_dx  (tr);
    const sutype::dcoord_t dy  = suStatic::get_dy  (tr);
    const sutype::ref_t    ref = suStatic::get_ref (tr);

    const sutype::dcoord_t prevw = w();
    const sutype::dcoord_t prevh = h();

    const sutype::dcoord_t prevxl = xl();
    const sutype::dcoord_t prevxh = xh();
    const sutype::dcoord_t prevyl = yl();
    const sutype::dcoord_t prevyh = yh();
    
    //  +--- 
    //  |
    //  +--
    //  |
    //  |
    if (ref == sutype::ref_0) {

      xl (dx + xl());
      yl (dy + yl());

      xh (xl() + prevw);
      yh (yl() + prevh);

      SUASSERT (xl() == dx + prevxl, "");
      SUASSERT (xh() == dx + prevxh, "");
      
      SUASSERT (yl() == dy + prevyl, "");
      SUASSERT (yh() == dy + prevyh, "");
    }

    //  |
    //  |
    //  +--
    //  |
    //  +---
    else if (ref == sutype::ref_x) {

      xl (dx + xl());
      yh (dy - yl());

      xh (xl() + prevw);
      yl (yh() - prevh);

      SUASSERT (xl() == dx + prevxl, "");
      SUASSERT (xh() == dx + prevxh, "");

      SUASSERT (yl() == dy - prevyh, "");
      SUASSERT (yh() == dy - prevyl, "");
    }

    //  ---+
    //     |
    //   --+
    //     |
    //     |
    else if (ref == sutype::ref_y) {

      xh (dx - xl());
      yl (dy + yl());

      xl (xh() - prevw);
      yh (yl() + prevh);

      SUASSERT (xl() == dx - prevxh, "");
      SUASSERT (xh() == dx - prevxl, "");

      SUASSERT (yl() == dy + prevyl, "");
      SUASSERT (yh() == dy + prevyh, "");
    }

    //     |
    //     |
    //   --+
    //     |
    //  ---+    
    else if (ref == sutype::ref_xy) {

      xh (dx - xl());
      yh (dy - yl());

      xl (xh() - prevw);
      yl (yh() - prevh);

      SUASSERT (xl() == dx - prevxh, "");
      SUASSERT (xh() == dx - prevxl, "");

      SUASSERT (yl() == dy - prevyh, "");
      SUASSERT (yh() == dy - prevyl, "");
    }

    else {
      SUASSERT (false, "");
    }
    
  } // end of suRectangle::apply_transformation
  
  // calculate dx & dy got given ref
  // rect = this + transformation
  void suRectangle::calculate_transformation (const suRectangle & rect,
                                              sutype::dcoord_t & dx,   // to calculate
                                              sutype::dcoord_t & dy,   // to calculate
                                              const sutype::ref_t ref) // given
    const
  {
    dx = 0;
    dy = 0;
    
    //  +--- 
    //  |
    //  +--
    //  |
    //  |
    if (ref == sutype::ref_0) {

      SUASSERT (rect.w() == w(), "");
      SUASSERT (rect.h() == h(), "");

      dx = rect.xl() - xl();
      dy = rect.yl() - yl();
    }

    //  |
    //  |
    //  +--
    //  |
    //  +---
    else if (ref == sutype::ref_x) {

      SUASSERT (rect.w() == w(), "");
      SUASSERT (rect.h() == h(), "");

      dx = rect.xl() - xl();
      dy = rect.yh() + yl();

      SUASSERT (dy == rect.yl() + yh(), "");
    }

    //  ---+
    //     |
    //   --+
    //     |
    //     |
    else if (ref == sutype::ref_y) {

      SUASSERT (rect.w() == w(), "");
      SUASSERT (rect.h() == h(), "");

      dx = rect.xh() + xl();
      dy = rect.yl() - yl();

      SUASSERT (dx == rect.xl() + xh(), "");
    }

    //     |
    //     |
    //   --+
    //     |
    //  ---+    
    else if (ref == sutype::ref_xy) {

      SUASSERT (rect.w() == w(), "");
      SUASSERT (rect.h() == h(), "");

      dx = rect.xh() + xl();
      dy = rect.yh() + yl();

      SUASSERT (dy == rect.yl() + yh(), "");
      SUASSERT (dx == rect.xl() + xh(), "");
    }

    else {
      SUASSERT (false, "Rect " << to_str() << " can't transformed into " << rect.to_str());
    }

    //return (sutype::tr_t (dx, dy, ref));
    
  } // end of suRectangle::calculate_transformation

  //
  void suRectangle::calculate_overlap (const suRectangle & rect,
                                       suRectangle & outrect)
    const
  {
    sutype::dcoord_t outxl = 0;
    sutype::dcoord_t outyl = 0;
    sutype::dcoord_t outxh = 0;
    sutype::dcoord_t outyh = 0;

    calculate_overlap (rect,
                       outxl, outyl, outxh, outyh);

    outrect.xl (outxl);
    outrect.yl (outyl);
    outrect.xh (outxh);
    outrect.yh (outyh);
  
  } // end of suRectangle::calculate_overlap
  
  //
  void suRectangle::calculate_overlap (const suRectangle & rect,
                                       sutype::dcoord_t & outxl,
                                       sutype::dcoord_t & outyl,
                                       sutype::dcoord_t & outxh,
                                       sutype::dcoord_t & outyh)
    const
  {
    outxl = std::max (xl(), rect.xl());
    outyl = std::max (yl(), rect.yl());
    outxh = std::min (xh(), rect.xh());
    outyh = std::min (yh(), rect.yh());
    
  } // end of suRectangle::calculate_overlap


  // ------------------------------------------------------------
  // -
  // --- Private static methods
  // -
  // ------------------------------------------------------------


  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------


} // end of namespace amsr

// end of suRectangle.cpp
