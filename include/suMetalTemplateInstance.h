// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Thu Oct 12 11:04:46 2017

//! \file   suMetalTemplateInstance.h
//! \brief  A header of the class suMetalTemplateInstance.

#ifndef _suMetalTemplateInstance_h_
#define _suMetalTemplateInstance_h_

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
  class suMetalTemplate;
  
  //! 
  class suMetalTemplateInstance
  {
  private:

    //
    const suMetalTemplate * _metalTemplate;

    // active region
    suRectangle _region;
    
    // pgd/ogd
    // shift in OGD from (0,0)
    // x-shift for ver metals
    // y-shift for hor metals
    sutype::dcoord_t _absoffset [2];
    
    //
    bool _inverted;

  public:

    //! custom constructor
    suMetalTemplateInstance (const suMetalTemplate * mt,
                             sutype::dcoord_t pgdoffset_abs,
                             sutype::dcoord_t ogdoffset_abs,
                             const suRectangle & r,
                             bool inverted);

  private:

    //! default constructor
    suMetalTemplateInstance ()
    {
      SUASSERT (false, "The method is not expected to be called.");
      
      init_ ();

    } // end of suMetalTemplateInstance

  private:

    //! copy constructor
    suMetalTemplateInstance (const suMetalTemplateInstance & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suMetalTemplateInstance

    //! assignment operator
    suMetalTemplateInstance & operator = (const suMetalTemplateInstance & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suMetalTemplateInstance ()
    {
    } // end of ~suMetalTemplateInstance

  private:

    //! init all class members
    inline void init_ ()
    {
      SUASSERT ((int)sutype::gd_pgd == 0, "");
      SUASSERT ((int)sutype::gd_ogd == 1, "");
      
      _metalTemplate = 0;
      _absoffset [sutype::gd_pgd] = 0;
      _absoffset [sutype::gd_ogd] = 0;
      _inverted = 0;
      
    } // end of init_
    
    //! copy all class members
    inline void copy_ (const suMetalTemplateInstance & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //

    
    // accessors (getters)
    //

    //
    inline const suMetalTemplate * metalTemplate () const { return _metalTemplate; }
    
    //
    inline sutype::dcoord_t shift () const { return _absoffset [sutype::gd_pgd]; }
    
    //
    inline const suRectangle & region () const { return _region; }

    //
    inline bool isInverted () const { return _inverted; }

  public:

    //
    sutype::dcoord_t sidel () const;
    sutype::dcoord_t sideh () const;
    sutype::dcoord_t edgel () const;
    sutype::dcoord_t edgeh () const;

    //
    void update_region (sutype::dcoord_t x1,
                        sutype::dcoord_t y1,
                        sutype::dcoord_t x2,
                        sutype::dcoord_t y2);

    //
    inline bool wire_sidel_sideh_is_inside_region (sutype::dcoord_t wiresidel,
                                                   sutype::dcoord_t wiresideh)
      const
    {
      return wire_sidel_sideh_is_inside_region_ (wiresidel,
                                                 wiresideh,
                                                 sidel(),
                                                 sideh());
      
    } // end of wire_sidel_sideh_is_inside_region

    //
    bool wire_sidel_sideh_is_inside_region (sutype::dcoord_t wiresidel,
                                            sutype::dcoord_t wiresideh,
                                            const suRectangle & rect)
      const;
    
    //
    inline bool wire_edgel_edgeh_is_inside_region (sutype::dcoord_t wireedgel,
                                                   sutype::dcoord_t wireedgeh)
      const
    {
      return wire_edgel_edgeh_is_inside_region_ (wireedgel,
                                                 wireedgeh,
                                                 edgel(),
                                                 edgeh());
      
    } // end of wire_edgel_edgeh_is_inside_region
       
    //!
    sutype::dcoord_t get_center_line (sutype::svi_t index)
      const;

    //
    sutype::dcoords_t get_center_lines (sutype::svi_t period0,
                                        sutype::svi_t period1)
      const;
    
    //! \return an absolute coordinate of the left side of the leftmost wire
    sutype::dcoord_t get_first_track_sidel ()
      const;
    
    // add/subtract integer number of pitches
    sutype::dcoord_t move_coord_inside_first_metal_template_instance (sutype::dcoord_t abscoord)
      const;
    
    //
    std::string to_str ()
      const;

    // 
    sutype::dcoord_t get_line_end_on_the_grid (sutype::dcoord_t absregionedgel,
                                               bool lower)
      const;

    // absregionedgel must match the OGD grid
    sutype::svi_t get_grid_line_index (sutype::dcoord_t absregionedgel)
      const;

    // absregionedgel must match the OGD grid
    sutype::dcoord_t get_next_line_end_on_the_grid (sutype::dcoord_t absregionedgel)
      const;

    //
    sutype::dcoords_t get_line_ends_in_rectangle (const suRectangle & rect)
      const;
    
    //
    void create_wires_in_rectangle (const suRectangle & rect,
                                    const suNet * net,
                                    const sutype::wire_type_t wiretype,
                                    bool discretizeWires, // create small wires to match OGD
                                    sutype::wires_t & wires)
      const;

    //
    bool wire_could_be_within_one_of_tracks (const suWire * wire)
      const;
    
  private:

    //
    inline bool wire_sidel_sideh_is_inside_region_ (sutype::dcoord_t wiresidel,
                                                    sutype::dcoord_t wiresideh,
                                                    sutype::dcoord_t regionsidel,
                                                    sutype::dcoord_t regionsideh)
      const
    {
      if (wiresideh <= regionsidel) return false;
      if (wiresidel >= regionsideh) return false;

      return true;
      
    } // end of wire_sidel_sideh_is_inside_region_
    
    //
    inline bool wire_edgel_edgeh_is_inside_region_ (sutype::dcoord_t wireedgel,
                                                    sutype::dcoord_t wireedgeh,
                                                    sutype::dcoord_t regionedgel,
                                                    sutype::dcoord_t regionedgeh)
      const
    {
      if (wireedgeh <= regionedgel) return false;
      if (wireedgel >= regionedgeh) return false;
      
      return true;
      
    } // end of wire_edgel_edgeh_is_inside_region_

  }; // end of class suMetalTemplateInstance

} // end of namespace amsr

#endif // _suMetalTemplateInstance_h_

// end of suMetalTemplateInstance.h

