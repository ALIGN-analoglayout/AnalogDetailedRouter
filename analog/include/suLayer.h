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
//! \date   Thu Oct  5 13:43:54 2017

//! \file   suLayer.h
//! \brief  A header of the class suLayer.

#ifndef _suLayer_h_
#define _suLayer_h_

// system includes
#include <algorithm>

// std includes
#include <string>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suTokenOwner.h>

namespace amsr
{
  // classes
  class suColoredLayer;
  class suToken;
  
  //! 
  class suLayer : public suTokenOwner
  {
    friend class suLayerManager;
    
  private:

    //
    static sutype::id_t _uniqueId;

  protected:
    
    //
    std::string _name;

    // synonyms and regular expressions
    sutype::strings_t _names;
    
  private:

    //
    sutype::coloredlayers_tc _coloredLayers;
    
    //
    sutype::id_t _id;

    //
    int _level;

    //
    sutype::bitid_t _type;

    //
    sutype::grid_orientation_t _pgd;
    
    // a list of layers
    sutype::layers_tc _electricallyConnected;

    // is_electrically_connected_with is frequent; so I use this precomputed vector of bools (index = layer->pers_id())
    std::vector<bool> _electricallyConnectedFlag;
    
    //
    bool _fixed;

    //
    bool _hasPgd;
    
  public:

    //! default constructor
    suLayer ()
    {
      init_ ();
      
    } // end of suLayer

    //! custom constructor
    suLayer (const std::string & name)
    {
      init_ ();

      _name = name;
      
    } // end of suLayer

    //! copy constructor
    suLayer (const suLayer & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suLayer

    //! assignment operator
    suLayer & operator = (const suLayer & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suLayer ();

  private:

    //! init all class members
    inline void init_ ()
    {
      _id = suLayer::_uniqueId;
      ++suLayer::_uniqueId;

      _level = 0;
      _type = 0;
      _pgd = sutype::go_ver;
      _fixed = false;
      _hasPgd = false;
      
    } // end of init_

    //! copy all class members
    inline void copy_ (const suLayer & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    //
    struct cmp_const_ptr
    {
     inline bool operator() (const suLayer * a, const suLayer * b)
        const
      {
        return (a->pers_id() < b->pers_id());
      } 
    }; // end of struct cmp_const_ptr

    //
    struct cmp_const_ptr_by_level
    {
      inline bool operator() (const suLayer * a, const suLayer * b)
        const
      {
        return (a->level() < b->level());
      }
    }; // end of struct cmp_const_ptr_by_level

    //
    struct cmp_const_ptr_by_level_reverse
    {
      inline bool operator() (const suLayer * a, const suLayer * b)
        const
      {
        return (a->level() > b->level());
      }
    }; // end of struct cmp_const_ptr_by_level_reverse

    // accessors (setters)
    //


    // accessors (getters)
    //

    //
    inline const std::string & name () const { return _name; }

    //
    inline bool fixed () const { return _fixed; }

    // return personal id
    inline sutype::id_t pers_id () const { return _id; }
    
    //
    inline const sutype::coloredlayers_tc & coloredLayers () const { return _coloredLayers; }
    
    // virtual accessors (getters)
    //
    
    //
    virtual inline bool is_base () const { return true; }

    //
    virtual inline bool is_color () const { return false; }
    
    //
    virtual inline const suLayer * base () const { return this; }

    //
    virtual inline const suColoredLayer * to_color () const { return 0; }

    //
    virtual inline sutype::bitid_t type () const { return _type; }
    
    //
    virtual inline int level () const { return _level; }

    //
    virtual inline sutype::id_t base_id () const { return _id; }

    //
    virtual inline bool has_pgd () const { return _hasPgd; }
    
    //
    virtual inline sutype::grid_orientation_t pgd () const { SUASSERT (has_pgd(), ""); return _pgd; }
    
    //
    virtual inline sutype::grid_orientation_t ogd () const { SUASSERT (has_pgd(), ""); return ((sutype::grid_orientation_t) (1 - (int)pgd())); }

    //
    virtual inline bool is_ver () const { return (pgd() == sutype::go_ver); }

    //
    virtual inline bool is_hor () const { return (pgd() == sutype::go_hor); }
    
    //
    virtual inline bool is (sutype::bitid_t t) const { return ((_type & t) == t); }
    
//     //
//     virtual inline bool is_electrically_connected_with (const suLayer * layer)
//       const
//     {
//       return (layer->base() == this->base() || std::find (_electricallyConnected.begin(), _electricallyConnected.end(), layer->base()) != _electricallyConnected.end());
      
//     } // end of is_electrically_connected_with


    virtual inline bool is_electrically_connected_with (const suLayer * layer)
      const
    {
      return _electricallyConnectedFlag [layer->base()->pers_id()];
      
    } // end of is_electrically_connected_with

    //
    virtual inline const sutype::layers_tc & electricallyConnected () const { return _electricallyConnected; }
    
  public:

    //
    bool has_name (const std::string & str)
      const;

    //
    void add_name (const std::string & str);
        
    //
    const suLayer * get_colored_layer_by_type (const std::string & colortype)
      const;

    //
    const suLayer * get_colored_layer_by_name (const std::string & colorname)
      const;
    
    //
    void add_colored_layer (const suColoredLayer * coloredlayer);

    //
    std::string to_str ()
      const;
    
  }; // end of class suLayer

} // end of namespace amsr

#endif // _suLayer_h_

// end of suLayer.h

