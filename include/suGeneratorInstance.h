// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Fri Nov  3 13:43:48 2017

//! \file   suGeneratorInstance.h
//! \brief  A header of the class suGeneratorInstance.

#ifndef _suGeneratorInstance_h_
#define _suGeneratorInstance_h_

// system includes

// std includes
#include <string>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suPoint.h>
#include <suRoute.h>

namespace amsr
{
  // classes
  class suGenerator;
  class suNet;
  
  //! 
  class suGeneratorInstance : public suRoute
  {
  private:

    // transformations (rotations, mirroring) are not supported yet
    
    //
    suPoint _point;

    //
    const suNet * _net;
    
    //
    const suGenerator * _generator;

    //
    sutype::dcoord_t _coverage;

    //
    bool _legal;

  public:

    //! custom constructor
    suGeneratorInstance (const suGenerator * generator,
                         const suNet * net,
                         sutype::dcoord_t dx,
                         sutype::dcoord_t dy,
                         sutype::wire_type_t cutwiretype);

  private:

    //! default constructor
    suGeneratorInstance ()
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();

    } // end of suGeneratorInstance

    //! copy constructor
    suGeneratorInstance (const suGeneratorInstance & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suGeneratorInstance

    //! assignment operator
    suGeneratorInstance & operator = (const suGeneratorInstance & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suGeneratorInstance ()
    {
    } // end of ~suGeneratorInstance

  private:

    //! init all class members
    inline void init_ ()
    {
      _net = 0;
      _generator = 0;
      _coverage = -1;
      _legal = true;

    } // end of init_

    //! copy all class members
    inline void copy_ (const suGeneratorInstance & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //

    //
    inline void set_minimal_coverage (sutype::dcoord_t v) { if (_coverage < 0 || v < _coverage) _coverage = v; }

    //
    inline void set_illegal () { _legal = false; }
    
    
    // accessors (getters)
    //

    //
    inline const suNet * net () const { return _net; }

    //
    inline const suGenerator * generator () const { return _generator; }

    //
    inline const suPoint & point () const { return _point; }

    //
    inline sutype::dcoord_t coverage () const { return _coverage; }

    //
    inline bool legal () const { return _legal; }

    
  public:

    //
    void add_wire (suWire * wire) { _wires.push_back (wire); }

    //
    suWire * get_wire (sutype::via_generator_layer_t vgl)
      const;

    //
    inline suWire * get_cut_wire ()
      const
    {
      return get_wire (sutype::vgl_cut);
      
    } // end of get_cut_wire
    
    //
    suGeneratorInstance * create_clone (sutype::wire_type_t cutwiretype)
      const;

    //
    std::string to_str ()
      const;

  private:

  }; // end of class suGeneratorInstance

} // end of namespace amsr

#endif // _suGeneratorInstance_h_

// end of suGeneratorInstance.h

