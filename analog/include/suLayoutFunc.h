//
//
//        INTEL CONFIDENTIAL - INTERNAL USE ONLY
//
//         Copyright by Intel Corporation, 2017
//                 All rights reserved.
//         Copyright does not imply publication.
//
//
//! \since  Analog/Mixed Signal Router : public suLayoutNode(prototype); AMSR 0.00
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Mon Oct 16 10:25:19 2017

//! \file   suLayoutFunc.h
//! \brief  A header of the class suLayoutFunc.

#ifndef _suLayoutFunc_h_
#define _suLayoutFunc_h_

// system includes

// std includes
#include <bitset>
#include <string>
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
  class suLayoutFunc : public suLayoutNode
  {
  private:

    //
    sutype::layoutnodes_t _nodes;
    
    //
    std::bitset<2> _funcAndSign;

#ifdef _DEBUG_LAYOUT_FUNCTIONS_

    // for debug only
    std::string _fileName;
    int         _fileLine;

#endif

  public:

    //! default constructor
    suLayoutFunc ()
    {
      init_ ();

    } // end of suLayoutFunc

  private:

    //! copy constructor
    suLayoutFunc (const suLayoutFunc & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suLayoutFunc

    //! assignment operator
    suLayoutFunc & operator = (const suLayoutFunc & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suLayoutFunc ()
    {
    } // end of ~suLayoutFunc

  private:

    //! init all class members
    inline void init_ ()
    {
#ifdef _DEBUG_LAYOUT_FUNCTIONS_
      _fileLine = -1;
#endif // _DEBUG_LAYOUT_FUNCTIONS_

    } // end of init_

    //! copy all class members
    inline void copy_ (const suLayoutFunc & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //

    //!
    inline void func (sutype::logic_func_t v) { _funcAndSign[0] = v; }

    //
    inline void sign (sutype::unary_sign_t v) { _funcAndSign[1] = v; }
    
    //
    inline void satindex (sutype::satindex_t v) { _satindex = v; }

    //
    inline void add_nodes (const sutype::layoutnodes_t & v) { _nodes.insert (_nodes.end(), v.begin(), v.end()); }

    //
    inline void fileName (const std::string & v)
    {

#ifdef _DEBUG_LAYOUT_FUNCTIONS_

      _fileName = v;

#endif // _DEBUG_LAYOUT_FUNCTIONS_

    } // end of fileName

    //
    inline void fileLine (int v)
    {

#ifdef _DEBUG_LAYOUT_FUNCTIONS_

      _fileLine = v;

#endif // _DEBUG_LAYOUT_FUNCTIONS_

    } // end of fileLine
    
    // accessors (getters)
    //
    
    //!
    inline sutype::logic_func_t func () const { return (sutype::logic_func_t)_funcAndSign[0]; }
    
    //!
    inline sutype::unary_sign_t sign () const { return (sutype::unary_sign_t)_funcAndSign[1]; }

    //
    inline const sutype::layoutnodes_t & nodes () const { return _nodes; }

    // virtual
    inline sutype::satindex_t satindex () const { return _satindex; }
    
    // virtual
    inline bool is_func () const { return true; }
    
    // virtual
    inline const suLayoutFunc * to_func () const { return this; }
    
    // virtual
    inline suLayoutFunc * to_func () { return this; }

    // virtual
    bool is_constant (int index)
      const;

    // virtual
    sutype::satindex_t model_satindex (sutype::satindex_t satindexToCheck,
                                       sutype::satindex_t modeledValue)
      const;

    // virtual
    sutype::satindex_t calculate_satindex (bool rootcall = true);
    
    // virtual
    suLayoutNode * create_clone ()
      const;

    //
    inline const std::string & fileName ()
      const
    {

#ifdef _DEBUG_LAYOUT_FUNCTIONS_

      return _fileName;

#endif // _DEBUG_LAYOUT_FUNCTIONS_

      return suLayoutNode::_emptyString;
      
    } // end of fileName

    //
    inline int fileLine ()
      const
    {

#ifdef _DEBUG_LAYOUT_FUNCTIONS_

      return _fileLine;

#endif // _DEBUG_LAYOUT_FUNCTIONS_

      return -1;
      
    } // end of fileLine
    
  public:

    //
    sutype::bool_t get_modeled_value ()
      const;
    
    //
    inline void add_node (suLayoutNode * node) { _nodes.push_back (node); }

    // I can update even func but it's not implemented yet; maybe need to delete func
    inline void update_node (sutype::uvi_t index,
                             suLayoutNode * node)
    {
      SUASSERT (index < _nodes.size(), "");
      SUASSERT (_nodes[index]->is_leaf(), "");
      SUASSERT (node->is_leaf(), "");
      
      _nodes [index] = node;
      
    } // end of update_node

    //
    inline suLayoutFunc * add_func (suLayoutFunc * f)
    {
      _nodes.push_back ((suLayoutNode*)f);
      
      return f;

    } // end of add_func

    //
    inline suLayoutFunc * add_func (sutype::logic_func_t lf,
                                    sutype::unary_sign_t us)
    {
      return add_func (lf, us, suLayoutNode::_emptyString, -1);
      
    } // end of add_func

    //
    inline suLayoutFunc * add_func (sutype::logic_func_t lf,
                                    sutype::unary_sign_t us,
                                    const std::string & filename,
                                    const int fileline)
    {
      suLayoutFunc * func = suLayoutNode::create_func (lf, us);

      func->fileName (filename);
      func->fileLine (fileline);
      
      _nodes.push_back ((suLayoutNode*)func);
      
      return func;
      
    } // end of add_func

    //
    inline suLayoutLeaf * add_leaf (sutype::satindex_t satindex)
    {
      suLayoutLeaf * leaf = suLayoutNode::create_leaf (satindex);

      _nodes.push_back ((suLayoutNode*)leaf);
      
      return leaf;
      
    } // end of add_leaf

    //
    suLayoutLeaf * add_leaf (sutype::satindex_t satindex,
                             const sutype::clause_t & depth);
    
    //
    void delete_nodes ();

    //
    inline void clear_nodes () { _nodes.clear(); }

    //
    suLayoutFunc * create_clone_and_replace_leaves_by_empty_or_functions ()
      const;

    //
    bool has_dangling_funcs ()
      const;

    //
    bool is_identical (const suLayoutFunc * func)
      const;

    //
    inline void invert_sign () { sign ((sutype::unary_sign_t)(1 - (int)sign())); }
    
    //
    inline void invert_func () { func ((sutype::logic_func_t)(1 - (int)func())); }
    
    //
    inline void invert_satindex () { satindex (-satindex()); }
    
    //
    void make_canonical ();

    //
    void simplify (bool rootcall = true);

    // satindices are not sorted; satindices are not unique
    void collect_satindices (sutype::clause_t & satindices)
      const;

    //
    bool has_satindex (sutype::satindex_t satindex)
      const;

    //
    bool replace_satindex (sutype::satindex_t satindexOldValue,
                           sutype::satindex_t satindexNewValue);
    
    //
    bool is_valid ()
      const;
    
    //
    std::string to_str ()
      const;
    
  private:
    
    //
    suLayoutLeaf * add_leaf_ (sutype::satindex_t satindex,
                              const sutype::clause_t & depth,
                              sutype::uvi_t depthindex);

  }; // end of class suLayoutFunc

} // end of namespace amsr

#endif // _suLayoutFunc_h_

// end of suLayoutFunc.h

