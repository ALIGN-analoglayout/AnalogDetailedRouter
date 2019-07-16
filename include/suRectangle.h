// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Thu Oct  5 13:45:03 2017

//! \file   suRectangle.h
//! \brief  A header of the class suRectangle.

#ifndef _suRectangle_h_
#define _suRectangle_h_

// system includes
#include <array>

// std includes
#include <sstream>
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
  class suRectangle
  {
  private:

    static sutype::rect_corner_t _gridDirection2Edge [2][2];
    static sutype::rect_corner_t _gridDirection2Side [2][2];
    static sutype::rect_corner_t _gridDirection2Coor [2][2];
    
  private:

    // xl, yl, xh, yh
    std::array <sutype::dcoord_t, 4> _dcoords;
    
  public:

    //! default constructor
    suRectangle ()
    {
      init_ ();

    } // end of suRectangle

    //! custom constructor
    //! vx1 may be > vx2; vy1 may be > vy2; 
    suRectangle (sutype::dcoord_t vx1,
                 sutype::dcoord_t vy1,
                 sutype::dcoord_t vx2,
                 sutype::dcoord_t vy2)
    {
      init_ ();
      
      xl (std::min (vx1, vx2));
      yl (std::min (vy1, vy2));
      xh (std::max (vx1, vx2));
      yh (std::max (vy1, vy2));
      
    } // end of suRectangle

    //! copy constructor
    suRectangle (const suRectangle & rs)
    {
      init_ ();
      copy_ (rs);

    } // end of suRectangle

    //! assignment operator
    suRectangle & operator = (const suRectangle & rs)
    {
      copy_ (rs);
      
      return * this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suRectangle ()
    {
    } // end of ~suRectangle

    //
    bool operator == (const suRectangle & rs)
    {
      return (_dcoords[0] == rs._dcoords[0] &&
              _dcoords[1] == rs._dcoords[1] &&
              _dcoords[2] == rs._dcoords[2] &&
              _dcoords[3] == rs._dcoords[3]);
      
    } // end of operator ==

    //
    bool operator != (const suRectangle & rs)
    {
      return (_dcoords[0] != rs._dcoords[0] ||
              _dcoords[1] != rs._dcoords[1] ||
              _dcoords[2] != rs._dcoords[2] ||
              _dcoords[3] != rs._dcoords[3]);
      
    } // end of operator !=
    
  private:

    //! init all class members
    inline void init_ ()
    {
      xl (0);
      yl (0);
      xh (0);
      yh (0);

    } // end of init_

    //! copy all class members
    inline void copy_ (const suRectangle & rs)
    {
      xl (rs.xl());
      yl (rs.yl());
      xh (rs.xh());
      yh (rs.yh());
      
    } // end of copy_

  public:

    // accessors (setters)
    //

    inline void xl (sutype::dcoord_t v) { _dcoords [sutype::rc_xl] = v; }
    inline void yl (sutype::dcoord_t v) { _dcoords [sutype::rc_yl] = v; }
    inline void xh (sutype::dcoord_t v) { _dcoords [sutype::rc_xh] = v; }
    inline void yh (sutype::dcoord_t v) { _dcoords [sutype::rc_yh] = v; }

    // accessors (getters)
    //
    
    //
    inline sutype::dcoord_t xl () const { return _dcoords [sutype::rc_xl]; }
    inline sutype::dcoord_t yl () const { return _dcoords [sutype::rc_yl]; }
    inline sutype::dcoord_t xh () const { return _dcoords [sutype::rc_xh]; }
    inline sutype::dcoord_t yh () const { return _dcoords [sutype::rc_yh]; }

    //
    inline void incr_xl (sutype::dcoord_t v) { xl (xl() + v); SUASSERT (xl() <= xh(), ""); }
    inline void incr_yl (sutype::dcoord_t v) { yl (yl() + v); SUASSERT (yl() <= yh(), ""); }
    inline void incr_xh (sutype::dcoord_t v) { xh (xh() + v); SUASSERT (xl() <= xh(), ""); }
    inline void incr_yh (sutype::dcoord_t v) { yh (yh() + v); SUASSERT (yl() <= yh(), ""); }
    
    //
    inline sutype::dcoord_t xc () const { return ((xl() + xh()) / 2); } 
    inline sutype::dcoord_t yc () const { return ((yl() + yh()) / 2); }

    //
    inline sutype::dcoord_t w () const { return (xh() - xl()); }
    inline sutype::dcoord_t h () const { return (yh() - yl()); }

    // edgel: yl for vertical wires; xl for horizontal wires
    // edgeh: yh for vertical wires; xh for horizontal wires
    // sidel: xl for vertical wires; yl for horizontal wires
    // sideh: xh for vertical wires; yh for horizontal wires

    //
    inline sutype::dcoord_t length (sutype::grid_orientation_t gd) const { return (edgeh (gd) - edgel (gd)); }
    inline sutype::dcoord_t width  (sutype::grid_orientation_t gd) const { return (sideh (gd) - sidel (gd)); }
    
    //
    inline sutype::dcoord_t edgel (sutype::grid_orientation_t gd) const { return _dcoords [suRectangle::_gridDirection2Edge [gd][sutype::mm_min]]; }
    inline sutype::dcoord_t edgeh (sutype::grid_orientation_t gd) const { return _dcoords [suRectangle::_gridDirection2Edge [gd][sutype::mm_max]]; }
    inline sutype::dcoord_t sidel (sutype::grid_orientation_t gd) const { return _dcoords [suRectangle::_gridDirection2Side [gd][sutype::mm_min]]; }
    inline sutype::dcoord_t sideh (sutype::grid_orientation_t gd) const { return _dcoords [suRectangle::_gridDirection2Side [gd][sutype::mm_max]]; }
    inline sutype::dcoord_t sidec (sutype::grid_orientation_t gd) const { return ((sidel (gd) + sideh (gd)) / 2); }
    
    //
    inline void edgel (sutype::grid_orientation_t gd, sutype::dcoord_t v) { _dcoords [suRectangle::_gridDirection2Edge [gd][sutype::mm_min]] = v ; }
    inline void edgeh (sutype::grid_orientation_t gd, sutype::dcoord_t v) { _dcoords [suRectangle::_gridDirection2Edge [gd][sutype::mm_max]] = v ; }
    inline void sidel (sutype::grid_orientation_t gd, sutype::dcoord_t v) { _dcoords [suRectangle::_gridDirection2Side [gd][sutype::mm_min]] = v ; }
    inline void sideh (sutype::grid_orientation_t gd, sutype::dcoord_t v) { _dcoords [suRectangle::_gridDirection2Side [gd][sutype::mm_max]] = v ; }

    //
    inline sutype::dcoord_t coorl (sutype::grid_orientation_t gd) const { return _dcoords [suRectangle::_gridDirection2Coor [gd][sutype::mm_min]]; }
    inline sutype::dcoord_t coorh (sutype::grid_orientation_t gd) const { return _dcoords [suRectangle::_gridDirection2Coor [gd][sutype::mm_max]]; }
    
  public:

    //
    static void sanity_check ()
    {
      SUASSERT (suRectangle::_gridDirection2Edge [sutype::go_ver] [sutype::mm_min] == sutype::rc_yl, suRectangle::_gridDirection2Edge [sutype::go_ver] [sutype::mm_min]);
      SUASSERT (suRectangle::_gridDirection2Edge [sutype::go_ver] [sutype::mm_max] == sutype::rc_yh, suRectangle::_gridDirection2Edge [sutype::go_ver] [sutype::mm_max]);
      SUASSERT (suRectangle::_gridDirection2Edge [sutype::go_hor] [sutype::mm_min] == sutype::rc_xl, suRectangle::_gridDirection2Edge [sutype::go_hor] [sutype::mm_min]);
      SUASSERT (suRectangle::_gridDirection2Edge [sutype::go_hor] [sutype::mm_max] == sutype::rc_xh, suRectangle::_gridDirection2Edge [sutype::go_hor] [sutype::mm_max]);
      
      SUASSERT (suRectangle::_gridDirection2Side [sutype::go_ver] [sutype::mm_min] == sutype::rc_xl, suRectangle::_gridDirection2Side [sutype::go_ver] [sutype::mm_min]);
      SUASSERT (suRectangle::_gridDirection2Side [sutype::go_ver] [sutype::mm_max] == sutype::rc_xh, suRectangle::_gridDirection2Side [sutype::go_ver] [sutype::mm_max]);
      SUASSERT (suRectangle::_gridDirection2Side [sutype::go_hor] [sutype::mm_min] == sutype::rc_yl, suRectangle::_gridDirection2Side [sutype::go_hor] [sutype::mm_min]);
      SUASSERT (suRectangle::_gridDirection2Side [sutype::go_hor] [sutype::mm_max] == sutype::rc_yh, suRectangle::_gridDirection2Side [sutype::go_hor] [sutype::mm_max]);

      SUASSERT (suRectangle::_gridDirection2Coor [sutype::go_ver] [sutype::mm_min] == sutype::rc_yl, suRectangle::_gridDirection2Coor [sutype::go_ver] [sutype::mm_min]);
      SUASSERT (suRectangle::_gridDirection2Coor [sutype::go_ver] [sutype::mm_max] == sutype::rc_yh, suRectangle::_gridDirection2Coor [sutype::go_ver] [sutype::mm_max]);
      SUASSERT (suRectangle::_gridDirection2Coor [sutype::go_hor] [sutype::mm_min] == sutype::rc_xl, suRectangle::_gridDirection2Coor [sutype::go_hor] [sutype::mm_min]);
      SUASSERT (suRectangle::_gridDirection2Coor [sutype::go_hor] [sutype::mm_max] == sutype::rc_xh, suRectangle::_gridDirection2Coor [sutype::go_hor] [sutype::mm_max]);
      
    } // end of sanity_check

    //
    inline void copy (const suRectangle & rect)
    {
      copy_ (rect);
      
    } // end of copy

    //
    inline void clear ()
    {
      xl (0);
      yl (0);
      xh (0);
      yh (0);
      
    } // emd of clear

    //
    inline void expand (const suRectangle & rect)
    {
      xl (std::min (xl(), rect.xl()));
      yl (std::min (yl(), rect.yl()));
      xh (std::max (xh(), rect.xh()));
      yh (std::max (yh(), rect.yh()));
      
    } // end of expand

    //
    void subtrack (const suRectangle & rect,
                   bool assertOnError = true);

    // rotation/mirror is not supported yet
    inline void shift (sutype::dcoord_t dx,
                       sutype::dcoord_t dy)
    {
      _dcoords [sutype::rc_xl] += dx;
      _dcoords [sutype::rc_xh] += dx;
      _dcoords [sutype::rc_yl] += dy;
      _dcoords [sutype::rc_yh] += dy;
      
    } // end of apply_transformation

    //
    void apply_transformation (const sutype::tr_t & tr);
    
    // rect = this + transformation
    void calculate_transformation (const suRectangle & rect,
                                   sutype::dcoord_t & dx,
                                   sutype::dcoord_t & dy,
                                   const sutype::ref_t ref)
      const;

    //
    inline std::string to_str ()
      const
    {
      return to_str (":");
      
    } // end of to_str

    //
    inline std::string to_str (const std::string & delim)
      const
    {
      std::ostringstream oss;

      oss
        << xl()
        << delim
        << yl()
        << delim
        << xh()
        << delim
        << yh();

      return oss.str();
      
    } // end of to_str

    //
    inline bool is_point ()
      const
    {
      return ((xl() == xh()) && (yl() == yh()));
      
    } // end is_point
    
    //
    inline bool is_line ()
      const
    {
      return ((xl() == xh() || yl() == yh()) && !is_point());
      
    } // end is_line

    //
    inline bool has_point (sutype::dcoord_t x,
                           sutype::dcoord_t y)
      const
    {
      return (x >= xl() && x <= xh() && y >= yl() && y <= yh());
      
    } // end of has_point

    //
    inline bool covers_compeletely (const suRectangle & rect)
      const
    {
      return (xl() <= rect.xl() && yl() <= rect.yl() && xh() >= rect.xh() && yh() >= rect.yh());
      
    } // end of covers_compeletely

    // overlap is at least a point
    inline bool has_at_least_common_point (const suRectangle & rect)
      const
    {
      return !(xh() < rect.xl() || yh() < rect.yl() || xl() > rect.xh() || yl() > rect.yh());
      
    } // end of has_at_least_common_point
    
    // overlap is more than a single point 
    bool has_at_least_common_line (const suRectangle & rect)
      const;

    // overlap is more than a single point or a line
    bool has_a_common_rect (const suRectangle & rect)
      const;
    
    //
    sutype::dcoord_t distance_to (const suRectangle & rect,
                                  sutype::grid_orientation_t gd)
      const;

    //
    void calculate_overlap (const suRectangle & rect,
                            suRectangle & outrect)
      const;
    
    //
    void calculate_overlap (const suRectangle & rect,
                            sutype::dcoord_t & outxl,
                            sutype::dcoord_t & outyl,
                            sutype::dcoord_t & outxh,
                            sutype::dcoord_t & outyh)
      const;
    
  }; // end of class suRectangle

} // end of namespace amsr

#endif // _suRectangle_h_

// end of suRectangle.h

