// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Oct 17 10:29:10 2017

//! \file   suTie.h
//! \brief  A header of the class suTie.

#ifndef _suTie_h_
#define _suTie_h_

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
  class suConnectedEntity;
  class suNet;
  class suRoute;

  //! 
  class suTie
  {
  private:

    //
    static sutype::id_t _uniqueId;

    //
    std::array <const suConnectedEntity *, 2> _connectedEntities;

    //
    sutype::routes_t _routes;

    //
    const suNet * _net;

    //
    sutype::id_t _id;

    //
    sutype::satindex_t _satindex;

    //
    sutype::satindex_t _openSatindex; // when 1 then tie can be open
    
  public:

    //! custom constructor
    suTie (const suConnectedEntity * ce1,
           const suConnectedEntity * ce2);
    
  private:

    //! default constructor
    suTie ()
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();

    } // end of suTie

    //! copy constructor
    suTie (const suTie & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suTie

    //! assignment operator
    suTie & operator = (const suTie & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suTie ();

  private:

    //! init all class members
    inline void init_ ()
    {
      _id = suTie::_uniqueId;
      ++suTie::_uniqueId;

      _connectedEntities [0] = 0;
      _connectedEntities [1] = 0;
      
      _satindex = sutype::UNDEFINED_SAT_INDEX;
      _openSatindex = sutype::UNDEFINED_SAT_INDEX;
      
    } // end of init_

    //! copy all class members
    inline void copy_ (const suTie & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    struct cmp_ptr
    {
      inline bool operator() (suTie * a, suTie * b)
        const
      {
        return (a->id() < b->id());
      }
    };

    // accessors (setters)
    //

    //
    inline void satindex (sutype::satindex_t v) { _satindex = v; }

    //
    inline void openSatindex (sutype::satindex_t v) { _openSatindex = v; }
    
    
    // accessors (getters)
    //
    
    //
    inline const suNet * net () const { return _net; }

    //
    inline sutype::id_t id () const { return _id; }

    //
    inline const suConnectedEntity * entity0 () const { return _connectedEntities [0]; }
    
    //
    inline const suConnectedEntity * entity1 () const { return _connectedEntities [1]; }

    //
    inline const suConnectedEntity * entity (sutype::uvi_t index) const { return _connectedEntities [index] ; }

    //
    inline const sutype::routes_t & routes () const { return _routes; }

    //
    inline sutype::routes_t & routes () { return _routes; }
    
    //
    inline sutype::satindex_t satindex () const { return _satindex; }

    //
    inline sutype::satindex_t openSatindex () const { return _openSatindex; }
    
  public:

    //
    inline void add_route (suRoute * route) { _routes.push_back (route); }

    //
    inline void clear_routes () { _routes.clear (); }

    //
    void delete_routes ();

    //
    void delete_route (suRoute * route);
    
    //
    const suConnectedEntity * get_another_entity (const suConnectedEntity * entity)
      const;

    //
    inline const bool has_entity (const suConnectedEntity * entity)
      const 
    {
      return (entity == entity0() || entity == entity1());
      
    } // end of has_entity

    //
    unsigned calculate_the_number_of_connected_entities_of_a_particular_type (sutype::wire_type_t wt)
      const;
    
    //
    std::string to_str ()
      const;
    
  private:

  }; // end of class suTie

} // end of namespace amsr

#endif // _suTie_h_

// end of suTie.h

