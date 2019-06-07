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
//! \date   Thu Oct 12 09:31:43 2017

//! \file   suMetalTemplateManager.h
//! \brief  A header of the class suMetalTemplateManager.

#ifndef _suMetalTemplateManager_h_
#define _suMetalTemplateManager_h_

// system includes

// std includes
#include <map>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes

namespace amsr
{
  // classes
  class suLayer;
  class suMetalTemplate;
  class suNet;
  class suWire;

  //! 
  class suMetalTemplateManager
  {
  private:

    //! static instance
    static suMetalTemplateManager * _instance;

  private:

    //
    sutype::metaltemplates_tc _metalTemplates;
    
    //
    sutype::metaltemplateinstances_t _allMetalTemplateInstances;

    // to create some reasonable visible wires in GUI for global routes
    std::vector<sutype::dcoord_t> _layerIdToMinimalWireWidth;
    
  private:

    //! default constructor
    suMetalTemplateManager ();

  private:

    //! copy constructor
    suMetalTemplateManager (const suMetalTemplateManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suMetalTemplateManager

    //! assignment operator
    suMetalTemplateManager & operator = (const suMetalTemplateManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suMetalTemplateManager ();

  private:

    //! init all class members
    inline void init_ ()
    {
    } // end of init_

    //! copy all class members
    inline void copy_ (const suMetalTemplateManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

    //
    const sutype::metaltemplateinstances_t & allMetalTemplateInstances () const { return _allMetalTemplateInstances; }

  public:

    // static methods
    //

    //
    static void delete_instance ();
    
    //
    static inline suMetalTemplateManager  * instance ()
    {
      if (suMetalTemplateManager::_instance == 0)
        suMetalTemplateManager::_instance = new suMetalTemplateManager ();

      return suMetalTemplateManager::_instance;

    } // end of instance

  public:

    //
    void read_metal_template_file (const std::string & filename);

    //
    void dump_mock_via_generators (const std::string & filename)
      const;
    
    //
    void create_metal_template_instances ();

    //
    void merge_metal_template_instances ();
    
    //
    sutype::metaltemplates_tc get_metal_templates_by_layer (const suLayer * layer)
      const;

    //
    const suMetalTemplate * get_metal_template_by_name (const std::string & mtname)
      const;
    
    //
    void print_metal_templates ()
      const;

    //
    bool check_regions_of_metal_template_instances ()
      const;

    //
    void check_metal_template_instances ()
      const;

    //
    const suMetalTemplateInstance * get_best_mti_for_wire (suWire * wire)
      const;
    
    //
    suWire * create_wire_to_match_metal_template_instance (const suNet * net,
                                                           const suLayer * layer,
                                                           const sutype::dcoord_t edgel,
                                                           const sutype::dcoord_t edgeh,
                                                           const sutype::dcoord_t sidel,
                                                           const sutype::dcoord_t sideh,
                                                           const sutype::wire_type_t wiretype)
      const;
    
    //
    suWire * create_wire_to_match_metal_template_instance (suWire * wire,
                                                           const sutype::wire_type_t wiretype)
      const;
    
    // \return true if all ok
    bool check_wire_with_metal_template_instances (suWire * wire)
      const;

    //
    void create_all_possible_mtis (suWire * wire,
                                   sutype::metaltemplateinstances_t & externalmtis)
      const;

    //
    sutype::dcoord_t get_minimal_wire_width (const suLayer * layer)
      const;
    
  private:

    //
    void get_best_matching_metal_template_instance_ (suWire * wire,
                                                     const suMetalTemplateInstance * & bestMti,
                                                     const suLayer * & bestLayer,
                                                     sutype::dcoord_t & bestSideL,
                                                     sutype::dcoord_t & bestSideH,
                                                     sutype::dcoord_t & bestDiff)
      const;

    //
    void get_best_matching_metal_template_instance_ (const suLayer * wirelayer,
                                                     const sutype::dcoord_t wireedgel,
                                                     const sutype::dcoord_t wireedgeh,
                                                     const sutype::dcoord_t wiresidel,
                                                     const sutype::dcoord_t wiresideh,
                                                     const suMetalTemplateInstance * & bestMti,
                                                     const suLayer * & bestLayer,
                                                     sutype::dcoord_t & bestSideL,
                                                     sutype::dcoord_t & bestSideH,
                                                     sutype::dcoord_t & bestDiff)
      const;

    //
    void get_best_matching_metal_template_instance_ (const suMetalTemplateInstance * mti,
                                                     const suLayer * wirelayer,
                                                     const sutype::dcoord_t wireedgel,
                                                     const sutype::dcoord_t wireedgeh,
                                                     const sutype::dcoord_t wiresidel,
                                                     const sutype::dcoord_t wiresideh,
                                                     const suMetalTemplateInstance * & bestMti,
                                                     const suLayer * & bestLayer,
                                                     sutype::dcoord_t & bestSideL,
                                                     sutype::dcoord_t & bestSideH,
                                                     sutype::dcoord_t & bestDiff)
      const;

  }; // end of class suMetalTemplateManager

} // end of namespace amsr

#endif // _suMetalTemplateManager_h_

// end of suMetalTemplateManager.h

