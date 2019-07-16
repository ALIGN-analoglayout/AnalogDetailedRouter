// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Mon Jan 29 17:36:30 2018

//! \file   suGrid.h
//! \brief  A header of the class suGrid.

#ifndef _suGrid_h_
#define _suGrid_h_

// system includes

// std includes
#include <set>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes

namespace amsr
{
  // classes
  class suGenerator;
  class suMetalTemplateInstance;
  class suRectangle;

  //! 
  class suGrid
  {
  private:

    //
    const suMetalTemplateInstance * _mtis [2];
    
    // ver: x-coords
    // hor: y-coords
    sutype::dcoords_t _dcoords [2];

    // ver: period in x units
    // hor: period in y units
    sutype::dcoord_t _dperiod [2];

    //
    std::vector <std::vector <sutype::generators_tc> > _generators;

    //
    std::vector <sutype::generators_tc> _generatorsOfBaseLayer;
    
  public:

    //! custom constructor
    suGrid (const suMetalTemplateInstance * vermti,
            const suMetalTemplateInstance * hormti,
            const sutype::dcoords_t & xdcoords1,
            const sutype::dcoords_t & ydcoords1,
            sutype::dcoord_t xperiod,
            sutype::dcoord_t yperiod);
    
  private:

    //! default constructor
    suGrid ()
    {
      SUASSERT (false, "The method is not expected to be called.");
      
      init_ ();

    } // end of suGrid

  private:

    //! copy constructor
    suGrid (const suGrid & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suGrid

    //! assignment operator
    suGrid & operator = (const suGrid & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suGrid ()
    {
    } // end of ~suGrid

  private:

    //! init all class members
    inline void init_ ()
    {
      for (int i=0; i < 2; ++i)
        _dperiod[i] = 0;
      
    } // end of init_

    //! copy all class members
    inline void copy_ (const suGrid & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //
    
    //
    inline sutype::gcoord_t get_gperiod (sutype::grid_orientation_t gd) const { SUASSERT (gd == sutype::go_ver || gd == sutype::go_hor, ""); return _dcoords[gd].size(); }
    
    //
    inline sutype::dcoord_t get_dperiod (sutype::grid_orientation_t gd) const { SUASSERT (gd == sutype::go_ver || gd == sutype::go_hor, ""); return _dperiod [gd]; }

    //
    inline const suMetalTemplateInstance * get_mti (sutype::grid_orientation_t gd) const { SUASSERT (gd == sutype::go_ver || gd == sutype::go_hor, ""); return _mtis[gd]; }
    
  public:
    
    //
    inline sutype::gcoord_t get_x_gperiod () const { return get_gperiod (sutype::go_ver); }
    inline sutype::gcoord_t get_y_gperiod () const { return get_gperiod (sutype::go_hor); }

    //
    inline sutype::gcoord_t get_x_dperiod () const { return get_dperiod (sutype::go_ver); }
    inline sutype::gcoord_t get_y_dperiod () const { return get_dperiod (sutype::go_hor); }
    
    //
    inline sutype::dcoord_t get_x_dcoord_by_index (sutype::gcoord_t gcoord) const { return get_dcoord_by_gcoord (sutype::go_ver, gcoord); }
    inline sutype::dcoord_t get_y_dcoord_by_index (sutype::gcoord_t gcoord) const { return get_dcoord_by_gcoord (sutype::go_hor, gcoord); }
    
    //
    inline sutype::gcoord_t get_grid_line_index_by_x_dcoord (sutype::dcoord_t dcoord) const { return get_grid_line_index_by_dcoord (sutype::go_ver, dcoord); }
    inline sutype::gcoord_t get_grid_line_index_by_y_dcoord (sutype::dcoord_t dcoord) const { return get_grid_line_index_by_dcoord (sutype::go_hor, dcoord); }

    //
    inline sutype::dcoords_t get_x_grid_line_indices (const suGenerator * generator) const { return get_grid_line_indices (sutype::go_ver, generator); }
    inline sutype::dcoords_t get_y_grid_line_indices (const suGenerator * generator) const { return get_grid_line_indices (sutype::go_hor, generator); }

    //
    void get_dcoords (const suRectangle & queryrect,
                      sutype::dcoords_t & xdcoords,
                      sutype::dcoords_t & ydcoords,
                      int dxl,
                      int dyl,
                      int dxh,
                      int dyh)
      const;
    
    //
    bool sanity_check (bool assertOnError)
      const;
    
    //
    sutype::dcoord_t get_dcoord_by_gcoord (sutype::grid_orientation_t gd,
                                           sutype::gcoord_t gcoord)
      const;
    
    //
    sutype::gcoord_t get_grid_line_index_by_dcoord (sutype::grid_orientation_t gd,
                                                    sutype::dcoord_t dcoord)
      const;
    
    //
    sutype::gcoord_t get_grid_line_index_by_gcoord (sutype::grid_orientation_t gd,
                                                    sutype::dcoord_t gcoord)
      const;

    //
    sutype::dcoords_t get_grid_line_indices (sutype::grid_orientation_t gd,
                                             const suGenerator * generator)
      const;

    //
    void add_generator (const suGenerator * generator,
                        sutype::dcoord_t xdcoord,
                        sutype::dcoord_t ydcoord);
    
    //
    const sutype::generators_tc & get_generators_at_gpoint (sutype::gcoord_t xgcoord,
                                                            sutype::gcoord_t ygcoord)
      const;

    //
    const sutype::generators_tc & get_generators_at_dpoint (sutype::dcoord_t xdcoord,
                                                            sutype::dcoord_t ydcoord)
      const;

    //
    const sutype::generators_tc & get_generators_of_base_layer (const suLayer * layer)
      const;
    
    //
    suRectangle get_generator_shape (const suGenerator * generator,
                                     sutype::gcoord_t xgcoord,
                                     sutype::gcoord_t ygcoord,
                                     sutype::via_generator_layer_t vgl = sutype::vgl_cut)
      const;
    
    //
    void print ()
      const;

    //
    void precompute_data ();

  private:
    
    //
    void calculate_generators_of_base_layers_ ();
    
    //
    sutype::generators_tc get_generators_by_base_layer_ (const suLayer * layer)
      const;

  }; // end of class suGrid

} // end of namespace amsr

#endif // _suGrid_h_

// end of suGrid.h

