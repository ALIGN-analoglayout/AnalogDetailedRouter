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
//! \date   Tue Nov 14 11:55:23 2017

//! \file   suPattern.h
//! \brief  A header of the class suPattern.

#ifndef _suPattern_h_
#define _suPattern_h_

// system includes

// std includes
#include <string>
#include <ostream>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes

namespace amsr
{
  // classes
  class suLayoutFunc;
  class suLayoutLeaf;

  //! 
  class suPattern
  {
  private:

    //
    static sutype::id_t _uniqueId;

    //
    std::string _name;

    //
    std::string _shortname;

    //
    std::string _comment;
    
    //
    suLayoutFunc * _layoutFunc;

    //
    sutype::id_t _id;
    
  public:

    //! default constructor
    suPattern ()
    {
      init_ ();
      
    } // end of suPattern

  private:

    //! copy constructor
    suPattern (const suPattern & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suPattern

    //! assignment operator
    suPattern & operator = (const suPattern & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suPattern ();

  private:

    //! init all class members
    inline void init_ ()
    {
      _id = suPattern::_uniqueId;
      ++suPattern::_uniqueId;

      _layoutFunc = 0;
      
    } // end of init_

    //! copy all class members
    inline void copy_ (const suPattern & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //

    //
    inline void name (const std::string & v) { _name = v; }

    //
    inline void shortname (const std::string & v) { _shortname = v; }

    //
    inline void comment (const std::string & v) { _comment = v; }

    //
    inline void layoutFunc (suLayoutFunc * v) { _layoutFunc = v; }
    
    
    // accessors (getters)
    //

    //
    inline sutype::id_t id () const { return _id; }

    //
    inline suLayoutFunc * layoutFunc() const { return _layoutFunc; }

    //
    inline const std::string & name () const { return _name; }

    //
    inline const std::string & shortname () const { return _shortname; }
    
    //
    inline const std::string & comment () const { return _comment; }
    
  public:
    
    //
    void create_pattern_instances (const suWire * wire,
                                   sutype::patterninstances_tc & pis)
      const;

    //
    void shift_to_0x0 ();

    //
    void apply_transfomation (const sutype::tr_t & tr);

    //
    void dump_as_flat_lgf_file (const std::string & directory)
      const;

    //
    void collect_unique_layers (std::map<const suLayer *, sutype::patterns_tc> & patternsPerLayer)
      const;
    
    //
    void dump (std::ostream & oss)
      const;
    
  private:

    //
    void dump_pattern_layout_node_ (std::ostream & oss,
                                    suLayoutNode * node,
                                    const std::string & offset)
      const;

    //
    void collect_wires_ (const suLayoutFunc * layoutfunc,
                         sutype::wires_t & wires)
      const;

    //
    void apply_transfomation_ (suLayoutFunc * layoutfunc,
                               const sutype::tr_t & tr);
      
    //
    void create_pattern_instances_ (const suWire * wire,
                                    sutype::patterninstances_tc & pis,
                                    const suLayoutFunc * layoutfunc,
                                    sutype::clause_t & depth)
      const;

    //
    void create_pattern_instances_ (const suWire * wire,
                                    sutype::patterninstances_tc & pis,
                                    const suLayoutLeaf * layoutleaf,
                                    sutype::clause_t & depth)
      const;

  }; // end of class suPattern

} // end of namespace amsr

#endif // _suPattern_h_

// end of suPattern.h

