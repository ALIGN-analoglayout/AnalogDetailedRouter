//
//
//        INTEL CONFIDENTIAL - INTERNAL USE ONLY
//
//         Copyright by Intel Corporation, 2017
//                 All rights reserved.
//         Copyright does not imply publication.
//
//
//! \since  Analog/Mixed Signal Router (prototype); AMSR 0.00
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Mon Oct  9 12:12:42 2017

//! \file   suNet.h
//! \brief  A header of the class suNet.

#ifndef _suNet_h_
#define _suNet_h_

// system includes

// std includes
#include <set>
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
  class suGeneratorInstance;
  class suWire;

  //! 
  class suNet
  {
  private:

    //
    static sutype::id_t _uniqueId;
    
    //
    std::string _name;

    //
    sutype::wires_t _wires;

    // layer->pers_id      sidel                       sideh                       edgel                      sideh
    std::vector <std::map <sutype::dcoord_t, std::map <sutype::dcoord_t, std::map <sutype::dcoord_t, std::map<sutype::dcoord_t, sutype::wires_t> > > > > _wiresPerLayers;
    
    //
    sutype::generatorinstances_t _generatorinstances;

    //
    sutype::id_t _id;
    
  public:

    //! custom constructor
    suNet (const std::string & name)
    {
      init_ ();
      
      _name = name;
      
    } // end of suNet
    
  private:

    //! default constructor
    suNet ()
    {
      SUASSERT (false, "The method is not expected to be called.");
      
      init_ ();

    } // end of suNet

    //! copy constructor
    suNet (const suNet & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suNet

    //! assignment operator
    suNet & operator = (const suNet & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suNet ();

  private:

    //! init all class members
    inline void init_ ()
    {
      _id = suNet::_uniqueId;
      ++suNet::_uniqueId;
      
    } // end of init_

    //! copy all class members
    inline void copy_ (const suNet & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    struct cmp_ptr
    {
      inline bool operator() (suNet * a, suNet * b)
        const
      {
        return (a->id() < b->id());
      }
    };

    struct cmp_const_ptr
    {
      inline bool operator() (const suNet * a, const suNet * b)
        const
      {
        return (a->id() < b->id());
      }
    };

    // accessors (setters)
    //


    // accessors (getters)
    //

    //
    inline const std::string & name () const { return _name; }

    //
    inline const sutype::wires_t & wires () const { return _wires; }

    //
    inline const sutype::generatorinstances_t & generatorinstances () const { return _generatorinstances; }

    //
    inline sutype::id_t id () const { return _id; }
    
  public:

    //
    inline bool has_wire (suWire * wire)
      const
    {
      return (std::find (_wires.begin(), _wires.end(), wire) != _wires.end());
      
    } // end of has_wire

    //
    bool wire_is_redundant (suWire * wire)
      const;
    
    //
    bool has_equal_generator_instance (suGeneratorInstance * gi)
      const;
    
    //
    void add_wire (suWire * wire);

    //
    void remove_wire (suWire * wire);

    //
    void add_generator_instance (suGeneratorInstance * generatorinstance);

    //
    void remove_generator_instance (suGeneratorInstance * generatorinstance);

    //
    bool wire_is_covered_by_a_preroute (suWire * wire)
      const;
    
    //
    void merge_wires ();

    //
    void remove_useless_wires ();

    //
    bool wire_is_in_conflict (const suWire * wire1,
                              bool verbose = false,
                              bool avoidUselessChecks = true)
      const;

    //
    bool wire_data_model_is_legal ()
      const;

    //
    std::string to_str ()
      const;
    
  private:

    //
    void register_wire_ (suWire * wire,
                         bool action); // true - add; false - remove

    //
    inline void add_wire_ (suWire * wire) { register_wire_ (wire, true); }

    //
    inline void remove_wire_ (suWire * wire) { register_wire_ (wire, false); }
    
  }; // end of class suNet

} // end of namespace amsr

#endif // _suNet_h_

// end of suNet.h

