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
//! \date   Mon Oct 16 10:25:10 2017

//! \file   suLayoutLeaf.h
//! \brief  A header of the class suLayoutLeaf.

#ifndef _suLayoutLeaf_h_
#define _suLayoutLeaf_h_

// system includes

// std includes
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suLayoutNode.h>

namespace amsr
{
  // classes
  
  //! 
  class suLayoutLeaf : public suLayoutNode
  {
  private:
    
  public:

    //! custom constructor
    suLayoutLeaf (sutype::satindex_t v)
    {
      init_ ();
      
      _satindex = v;
      
    } // end of suLayoutLeaf
    
  private:

    //! default constructor
    suLayoutLeaf ()
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();

    } // end of suLayoutLeaf

    //! copy constructor
    suLayoutLeaf (const suLayoutLeaf & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suLayoutLeaf

    //! assignment operator
    suLayoutLeaf & operator = (const suLayoutLeaf & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suLayoutLeaf ()
    {
    } // end of ~suLayoutLeaf

  private:

    //! init all class members
    inline void init_ ()
    {      
    } // end of init_

    //! copy all class members
    inline void copy_ (const suLayoutLeaf & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

    // virtual
    inline bool is_leaf () const { return true; }

    // virtual
    inline const suLayoutLeaf * to_leaf () const { return this; }

    // virtual
    inline suLayoutLeaf * to_leaf () { return this; }
    
    // virtual
    inline suLayoutNode * create_clone () const { return (suLayoutLeaf*)this; }

    // virtual
    bool is_constant (int index)
      const;
    
  public:

  private:

  }; // end of class suLayoutLeaf

} // end of namespace amsr

#endif // _suLayoutLeaf_h_

// end of suLayoutLeaf.h

