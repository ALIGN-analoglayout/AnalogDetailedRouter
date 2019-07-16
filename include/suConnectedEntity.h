// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Mon Oct 16 10:14:59 2017

//! \file   suConnectedEntity.h
//! \brief  A header of the class suConnectedEntity.

#ifndef _suConnectedEntity_h_
#define _suConnectedEntity_h_

// system includes

// std includes

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suRoute.h>

namespace amsr
{
  // classes
  class suGlobalRoute;
  class suLayoutFunc;
  class suNet;
  
  //! 
  class suConnectedEntity : public suRoute
  {
  private:

    //
    static sutype::id_t _uniqueId;
        
    //
    const suNet * _net;

    //
    sutype::wires_t _interfaceWires;
    
    //
    const suLayer * _interfaceLayer;
    
    //
    sutype::bitid_t _type;
        
    //
    sutype::id_t _id;
    
    // may be null
    suGlobalRoute * _trunkGlobalRoute;
    
    //
    sutype::satindex_t _openSatindex; // when 1 then CE can be open
    
  public:

    //! custom constructor
    suConnectedEntity (const suNet * net)
    {
      init_ ();

      _net = net;
      
    } // end of suConnectedEntity
    
  private:

    //! default constructor
    suConnectedEntity ()
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      
    } // end of suConnectedEntity

    //! copy constructor
    suConnectedEntity (const suConnectedEntity & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suConnectedEntity

    //! assignment operator
    suConnectedEntity & operator = (const suConnectedEntity & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    ~suConnectedEntity ();
    
  private:

    //! init all class members
    inline void init_ ()
    {
      _id = suConnectedEntity::_uniqueId;
      ++suConnectedEntity::_uniqueId;
     
      _net = 0;
      _type = 0;
      _trunkGlobalRoute = 0;
      _interfaceLayer = 0;
      _openSatindex = sutype::UNDEFINED_SAT_INDEX;
      
    } // end of init_

    //! copy all class members
    inline void copy_ (const suConnectedEntity & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //

    //
    inline void add_type (sutype::bitid_t t) { _type |= t; }

    //
    inline void type (sutype::bitid_t t) { _type = t; }

    //
    inline void trunkGlobalRoute (suGlobalRoute * v) { _trunkGlobalRoute = v; }

    //
    inline void openSatindex (sutype::satindex_t v) { _openSatindex = v; }

    //
    inline void set_interface_layer (const suLayer * v) { _interfaceLayer = v; }
    
    
    // accessors (getters)
    //
    
    //
    inline  const suNet * net () const { return _net; }
    
    //
    inline sutype::id_t id () const { return _id; }

    //
    inline bool is (sutype::bitid_t t) const { return ((_type & t) == t); }

    //
    inline sutype::bitid_t type () const { return _type; }

    //
    inline suGlobalRoute * trunkGlobalRoute () const { return _trunkGlobalRoute; }

    //
    inline const sutype::wires_t & interfaceWires () const { return _interfaceWires; }

    //
    inline sutype::satindex_t openSatindex () const { return _openSatindex; }

    //
    inline const suLayer * get_interface_layer () const { return _interfaceLayer; }
    
  public:
    
    struct cmp_const_ptr
    {
      inline bool operator() (const suConnectedEntity * a, const suConnectedEntity * b)
        const
      {
        return (a->id() < b->id());
      }
    };

    //
    void add_interface_wire (suWire * wire) { _interfaceWires.push_back (wire); }

    //
    void clear_interface_wires () { _interfaceWires.clear(); }
        
    //
    std::string to_str ()
      const;

    //
    bool layout_function_is_legal ()
      const;

    //
    suLayoutFunc * get_main_layout_function ()
      const;

  private:

  }; // end of class suConnectedEntity

} // end of namespace amsr

#endif // _suConnectedEntity_h_

// end of suConnectedEntity.h

