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
//! \date   Mon Oct 16 10:33:19 2017

//! \file   suLayoutNode.h
//! \brief  A header of the class suLayoutNode.

#ifndef _suLayoutNode_h_
#define _suLayoutNode_h_

// system includes

// std includes

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
  class suLayoutNode
  {

  private:

    static sutype::layoutfuncs_t _layoutFuncs;

    // positive
    static sutype::layoutleaves_t _layoutLeavesPosSign;

    // negative
    static sutype::layoutleaves_t _layoutLeavesNegSign;
    
  protected:

    //
    static std::string _emptyString;

    sutype::satindex_t _satindex;
    
  public:

    //! default constructor
    suLayoutNode ()
    {
      init_ ();

    } // end of suLayoutNode

  private:

    //! copy constructor
    suLayoutNode (const suLayoutNode & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suLayoutNode

    //! assignment operator
    suLayoutNode & operator = (const suLayoutNode & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suLayoutNode ()
    {
    } // end of ~suLayoutNode

  private:

    //! init all class members
    inline void init_ ()
    {
    } // end of init_

    //! copy all class members
    inline void copy_ (const suLayoutNode & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //
    
    
    // accessors (getters)
    //

    //
    inline virtual sutype::satindex_t satindex () const { return _satindex; }
    
    //
    inline virtual bool is_leaf () const { return false; }

    //
    inline virtual bool is_func () const { return false; }

    //
    inline virtual const suLayoutNode * to_node () const { return this; }

    //
    inline virtual suLayoutNode * to_node () { return this; }
    
    //
    inline virtual const suLayoutLeaf * to_leaf () const { return 0; }

    //
    inline virtual suLayoutLeaf * to_leaf () { return 0; }

    //
    inline virtual const suLayoutFunc * to_func () const { return 0; }

    //
    inline virtual suLayoutFunc * to_func () { return 0; }

    //
    inline virtual sutype::satindex_t model_satindex (sutype::satindex_t satindexToCheck,
                                                      sutype::satindex_t modeledValue)
      const
    {
      if      (_satindex ==  satindexToCheck) return  modeledValue;
      else if (_satindex == -satindexToCheck) return -modeledValue;
      else {
        return _satindex;
      }
      
    } // end of model_satindex
    
    //
    inline virtual sutype::satindex_t calculate_satindex (bool rootcall = true) { return _satindex; }
    
    //
    inline virtual suLayoutNode * create_clone () const { SUASSERT (false, ""); return 0; }

    //
    inline virtual bool is_constant (int index) const { SUASSERT (false, ""); return false; }
    
  public:
    
    // may invert nodes in some cases
    static suLayoutFunc * create_counter (sutype::boolean_counter_t ct,
                                          sutype::unary_sign_t us,
                                          sutype::layoutnodes_t & nodes,
                                          sutype::svi_t number);
    
    //
    static suLayoutFunc * create_func (sutype::logic_func_t lf,
                                       sutype::unary_sign_t us,
                                       const std::string & filename = suLayoutNode::_emptyString,
                                       const int fileline = -1);
    
    //
    static void delete_func (suLayoutFunc * func);

    //
    static suLayoutLeaf * create_leaf (sutype::satindex_t v);

    //
    static void delete_leaves ();
    
    //
    static void delete_funcs ();
    
  private:

    //
    static suLayoutFunc * build_default_gte_counter_ (const sutype::layoutnodes_t & nodes,
                                                      const sutype::svi_t number);

    //
    static void build_unary_counter_ (suLayoutFunc * lnfunc,
                                      int stage,
                                      int level,
                                      const std::vector <suLayoutNode *> & nodes,
                                      sutype::uvi_t & numfuncs);
    
  }; // end of class suLayoutNode

} // end of namespace amsr

#endif // _suLayoutNode_h_

// end of suLayoutNode.h

