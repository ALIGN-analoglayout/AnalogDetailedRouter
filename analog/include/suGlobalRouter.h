// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Oct 10 13:59:50 2017

//! \file   suGlobalRouter.h
//! \brief  A header of the class suGlobalRouter.

#ifndef _suGlobalRouter_h_
#define _suGlobalRouter_h_

// system includes

// std includes
#include <string>
#include <vector>

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
  class suGlobalRoute;
  class suRegion;
  class suTokenParser;

  //! 
  class suGlobalRouter
  {
  private:

    //
    static suGlobalRouter * _instance;

    //
    suTokenParser * _grTokenParser;

    //
    suRectangle _bbox;
    
    //
    sutype::globalroutes_t _globalRoutes;
    
    //
    std::vector<sutype::regions_t> _regions;
    
    //
    sutype::gredges_t _gredges;

    // to get regions by id
    sutype::regions_t _idToRegion;

    // start id
    int _minRegionId;
    
    //
    sutype::uvi_t _numcols;
    
    //
    sutype::uvi_t _numrows;

    //
    sutype::dcoord_t _regionw;
    
    //
    sutype::dcoord_t _regionh;
    
  private:

    //! default constructor
    suGlobalRouter ()
    {
      init_ ();

    } // end of suGlobalRouter

    //! copy constructor
    suGlobalRouter (const suGlobalRouter & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suGlobalRouter

    //! assignment operator
    suGlobalRouter & operator = (const suGlobalRouter & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suGlobalRouter ();

  private:

    //! init all class members
    inline void init_ ()
    {
      _grTokenParser = 0;
      _minRegionId = -1;
      _numcols = 0;
      _numrows = 0;
      _regionw = 0;
      _regionh = 0;
      
    } // end of init_

    //! copy all class members
    inline void copy_ (const suGlobalRouter & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

    //
    inline int reasonable_gr_length () const { return 2; }
    
    //
    const sutype::regions_t & regions () const { return _idToRegion; }

    //
    const sutype::globalroutes_t & globalRoutes () const { return _globalRoutes; }

    //
    sutype::globalroutes_t & globalRoutes () { return _globalRoutes; }

    //
    const suTokenParser * grTokenParser () const { return _grTokenParser; }
    
  public:

    // static methods
    //
    
    //
    static void delete_instance ();
    
    //
    static inline suGlobalRouter * instance ()
    {
      if (suGlobalRouter::_instance == 0)
        suGlobalRouter::_instance = new suGlobalRouter ();

      return suGlobalRouter::_instance;
      
    } // end of instance

  public:

    //
    inline suRegion * get_region (sutype::svi_t x,
                                  sutype::svi_t y)
      const
    {
      //SUASSERT (x >= 0 && x < (sutype::svi_t)_numcols, "x=" << x << "; _numcols=" << _numcols);
      //SUASSERT (y >= 0 && y < (sutype::svi_t)_numrows, "y=" << y << "; _numrows=" << _numrows);

      if (x >= 0 && x < (sutype::svi_t)_numcols &&
          y >= 0 && y < (sutype::svi_t)_numrows) {

        return _regions[x][y];
      }
      else {
       
        SUISSUE("Could not get region") << "(x=" << x << ",y=" << y << "): min_x=0; max_x=" << (_numcols-1) << "; min_y=0 max_y=" << (_numrows-1) << std::endl;
        return 0;
      }
      
    } // end of get_region

    //
    void create_global_routing_grid ();

    //
    void read_global_routing ();

    //
    sutype::ids_t get_terminal_gids ()
      const;

    //
    void parse_global_routing ();

    //
    void create_fake_global_routes ()
      const;

    //
    void dump_global_routing (const std::string & filename)
      const;

    // procedure supports regular grid at this moment
    int dcoord_to_region_coord (sutype::dcoord_t dcoord,
                                sutype::grid_orientation_t pgd,
                                sutype::side_t priorityside)
      const;

  private:

    //
    void parse_global_routings_ (const suToken * roottoken);
    
    //
    void create_regions_ ();

    //
    void create_edges_ ();

    //
    void get_region_bounds_ (const suRectangle & rect,
                             int & wcol,
                             int & ecol,
                             int & srow,
                             int & nrow)
      const;
      
    //
    void get_regions_ (const suRectangle & rect,
                       const suRegion * & swregion,
                       const suRegion * & nwregion,
                       const suRegion * & seregion,
                       const suRegion * & neregion)
      const;

    //
    void merge_global_routes_of_one_net_ (sutype::globalroutes_t & grs)
      const;

    //
    void merge_global_routes_of_one_layer_ (sutype::globalroutes_t & grs)
      const;

    //
    void sanity_check_ ()
      const;
    
  }; // end of class suGlobalRouter

} // end of namespace amsr

#endif // _suGlobalRouter_h_

// end of suGlobalRouter.h

