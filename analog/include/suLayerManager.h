// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Thu Oct  5 15:44:50 2017

//! \file   suLayerManager.h
//! \brief  A header of the class suLayerManager.

#ifndef _suLayerManager_h_
#define _suLayerManager_h_

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
  class suTokenParser;

  //! 
  class suLayerManager
  {
  private:

    //
    static suLayerManager * _instance;
    
    //
    sutype::layers_tc _layers;

    //
    sutype::layers_tc _idToLayer;

    // token are used as option managers then
    std::vector<const suTokenParser *> _tokenParsers;

    //
    const suLayer * _lowestDRLayer; // expected m1

    //
    const suLayer * _lowestGRLayer; // expected m2

    //
    const suLayer * _highestDRLayer; // can be null
    
  private:

    //! default constructor
    suLayerManager ()
    {
      init_ ();

    } // end of suLayerManager

    //! copy constructor
    suLayerManager (const suLayerManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suLayerManager

    //! assignment operator
    suLayerManager & operator = (const suLayerManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suLayerManager ();

  private:

    //! init all class members
    inline void init_ ()
    {
      _lowestDRLayer  = 0;
      _lowestGRLayer  = 0;
      _highestDRLayer = 0;
      
    } // end of init_

    //! copy all class members
    inline void copy_ (const suLayerManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

    // sorted by level
    inline const sutype::layers_tc & layers () const { return _layers; }

    // sorted by id
    inline const sutype::layers_tc & idToLayer () const { return _idToLayer; }

    //
    inline const suLayer * lowestDRLayer () const { return _lowestDRLayer; }

    //
    inline const suLayer * highestDRLayer () const { return _highestDRLayer; }
    
    //
    inline const suLayer * lowestGRLayer () const { return _lowestGRLayer; }
    
  public:

    // static methods
    //
    
    //
    static void delete_instance ();
    
    //
    static inline suLayerManager * instance ()
    {
      if (suLayerManager::_instance == 0)
        suLayerManager::_instance = new suLayerManager ();

      return suLayerManager::_instance;
      
    } // end of instance

  public:

    //
    const suLayer * get_base_layer_by_name (const std::string & name)
      const;
    
    //
    const suLayer * get_colored_layer_by_name (const std::string & name)
      const;
    
    //
    const suLayer * get_base_layer_by_level (int level)
      const;
    
    //! \return the first layer of this type
    const suLayer * get_base_layer_by_type (int type)
      const;

    //
    inline const suLayer * get_base_layer_by_id (sutype::id_t id)
      const
    {
      SUASSERT (id >= 0 && id < (sutype::id_t)_idToLayer.size(), "");
      SUASSERT (_idToLayer[id], "");
      
      return _idToLayer[id];
      
    } // end of 

    //
    const suLayer * get_base_adjacent_layer (const suLayer * layer,
                                             int type,
                                             bool upper,
                                             unsigned numSkipTracks)
      const;

    //
    bool layers_are_adjacent_metal_layers (const suLayer * layer1,
                                           const suLayer * layer2)
      const;
    
    
    //
    void read_layer_file (const std::string & filname);

    //
    const suLayer * get_base_via_layer (const suLayer * layer1,
                                        const suLayer * layer2)
      const;

    //
    sutype::layers_tc get_colors (const suLayer * baselayer)
      const;

  private:

    //
    void sanity_check_ ();

    //
    void post_reading_ ();

    //
    void calculate_lowest_routing_wire_layers_ ();

    //
    void calculate_highest_routing_wire_layers_ ();
    
  }; // end of class suLayerManager

} // end of namespace amsr

#endif // _suLayerManager_h_

// end of suLayerManager.h

