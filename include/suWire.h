// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Mon Oct  9 12:12:49 2017

//! \file   suWire.h
//! \brief  A header of the class suWire.

#ifndef _suWire_h_
#define _suWire_h_

// system includes

// std includes
#include <string>

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
  class suNet;
  class suLayer;
  class suWireManager;
  
  //! 
  class suWire
  {
    friend class suWireManager;

  protected:

    //
    const suLayer * _layer;
    
    //
    suRectangle _rect;
    
  private:

    //
    static sutype::id_t _uniqueId;
    
    //
    const suNet * _net;

    // 
    sutype::id_t _id;

    //
    sutype::id_t _gid; // global id
    
    //
    sutype::satindex_t _satindex;
    
    //
    sutype::bitid_t _type;

    //
    sutype::bitid_t _dir;
    
  private:
    
    //! default constructor
    suWire ()
    {      
      init_ ();

    } // end of suWire

    //! copy constructor
    suWire (const suWire & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suWire

    //! assignment operator
    suWire & operator = (const suWire & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator
    
    //! destructor
    virtual ~suWire ()
    {
    } // end of ~suWire

  private:

    //! init all class members
    inline void init_ ()
    {
      _id = suWire::_uniqueId;
      ++suWire::_uniqueId;
      
      _net = 0;
      _layer = 0;
      _satindex = sutype::UNDEFINED_SAT_INDEX;
      _type = 0;
      _dir = sutype::wd_any;
      _gid = sutype::UNDEFINED_GLOBAL_ID;
      
    } // end of init_

    //! copy all class members
    inline void copy_ (const suWire & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    struct cmp_ptr
    {
      inline bool operator() (suWire * a, suWire * b)
        const
      {
        return (a->id() < b->id());
      }
    };

    struct cmp_ptr_by_sideh
    {
      inline bool operator() (suWire * a, suWire * b)
        const
      {
        sutype::dcoord_t sideh0 = a->sideh();
        sutype::dcoord_t sideh1 = b->sideh();

        if (sideh0 != sideh1)
          return (sideh0 < sideh1);
        
        return (a->id() < b->id());
      }
    };

    // accessors (setters)
    //
    
    //
    inline void satindex (sutype::satindex_t v) { _satindex = v; }
    
    
    // accessors (getters)
    //

    //
    inline sutype::id_t id () const { return _id; }

    //
    inline sutype::id_t gid () const { return _gid; }

    //
    inline const suNet * net () const { return _net; }

    //
    inline const suLayer * layer () const { return _layer; }

    //
    inline const suRectangle & rect () const { return _rect; }
    
    //
    inline sutype::satindex_t satindex () const { return _satindex; }

    //
    inline bool is (sutype::bitid_t t) const { return ((_type & t) == t); }

    //
    inline sutype::bitid_t type () const { return _type; }

  public:

    //
    sutype::wire_type_t get_wire_type ()
      const;

    //
    sutype::dcoord_t sidel () const;
    sutype::dcoord_t sideh () const;
    sutype::dcoord_t sidec () const { return ((sidel() + sideh()) / 2); }
    
    sutype::dcoord_t edgel () const;
    sutype::dcoord_t edgeh () const;
    sutype::dcoord_t edgec () const { return ((edgel() + edgeh()) / 2); }
    
    // wire width; not a rectangle width
    inline sutype::dcoord_t width () const { return (sideh() - sidel()); }

    // wire length; not a rectangle length
    inline sutype::dcoord_t length () const { return (edgeh() - edgel()); }
    
    //
    std::string to_str ()
      const;

  }; // end of class suWire

} // end of namespace amsr

#endif // _suWire_h_

// end of suWire.h

