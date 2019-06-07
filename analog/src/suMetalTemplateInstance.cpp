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
//! \date   Thu Oct 12 13:02:31 2017

//! \file   suMetalTemplateInstance.cpp
//! \brief  A collection of methods of the class suMetalTemplateInstance.

// std includes
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suLayer.h>
#include <suMetalTemplate.h>
#include <suRectangle.h>
#include <suStatic.h>
#include <suWire.h>
#include <suWireManager.h>

// module include
#include <suMetalTemplateInstance.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------


  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  //! custom constructor
  suMetalTemplateInstance::suMetalTemplateInstance (const suMetalTemplate * mt,
                                                    sutype::dcoord_t pgdoffset_abs,
                                                    sutype::dcoord_t ogdoffset_abs,
                                                    const suRectangle & r,
                                                    bool inverted)
  {
    SUASSERT (mt, "");
    
    init_ ();
    
    _metalTemplate = mt;
    SUASSERT (_metalTemplate->baseLayer(), "");
    SUASSERT (_metalTemplate->baseLayer()->has_pgd(), "");
    
    _absoffset [sutype::gd_pgd] = mt->convert_to_canonical_offset (sutype::gd_pgd, pgdoffset_abs);
    _absoffset [sutype::gd_ogd] = mt->convert_to_canonical_offset (sutype::gd_ogd, ogdoffset_abs);
    
    _region = r;
    SUASSERT (_region.xl() < _region.xh(), "");
    SUASSERT (_region.yl() < _region.yh(), "");

    _inverted = inverted;
    
  } // end of suMetalTemplateInstance::suMetalTemplateInstance


  // ------------------------------------------------------------
  // -
  // --- Public static methods
  // -
  // ------------------------------------------------------------


  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  // 
  sutype::dcoord_t suMetalTemplateInstance::sidel () const { return _region.sidel (_metalTemplate->baseLayer()->pgd()); }
  sutype::dcoord_t suMetalTemplateInstance::sideh () const { return _region.sideh (_metalTemplate->baseLayer()->pgd()); }
  sutype::dcoord_t suMetalTemplateInstance::edgel () const { return _region.edgel (_metalTemplate->baseLayer()->pgd()); }
  sutype::dcoord_t suMetalTemplateInstance::edgeh () const { return _region.edgeh (_metalTemplate->baseLayer()->pgd()); }

  //
  void suMetalTemplateInstance::update_region (sutype::dcoord_t x1,
                                               sutype::dcoord_t y1,
                                               sutype::dcoord_t x2,
                                               sutype::dcoord_t y2)
  {
    sutype::dcoord_t xl = std::min (x1, x2);
    sutype::dcoord_t yl = std::min (y1, y2);
    sutype::dcoord_t xh = std::max (x1, x2);
    sutype::dcoord_t yh = std::max (y1, y2);

    _region.xl (xl);
    _region.yl (yl);
    _region.xh (xh);
    _region.yh (yh);
    
  } // end of suMetalTemplateInstance::update_region

  //
  bool suMetalTemplateInstance::wire_sidel_sideh_is_inside_region (sutype::dcoord_t wiresidel,
                                                                   sutype::dcoord_t wiresideh,
                                                                   const suRectangle & rect)
    const
  {
    return wire_sidel_sideh_is_inside_region_ (wiresidel,
                                               wiresideh,
                                               rect.sidel (_metalTemplate->baseLayer()->pgd()),
                                               rect.sideh (_metalTemplate->baseLayer()->pgd()));
    
  } // end of suMetalTemplateInstance::wire_sidel_sideh_is_inside_region

  //!
  sutype::dcoord_t suMetalTemplateInstance::get_center_line (sutype::svi_t absindex)
    const
  {
    const sutype::dcoords_t & widths = metalTemplate()->get_widths (isInverted());
    const sutype::dcoords_t & spaces = metalTemplate()->get_spaces (isInverted());

    const sutype::svi_t numwidths = (sutype::svi_t)widths.size();
    
    sutype::svi_t numperiods = (absindex / numwidths);
    sutype::svi_t refindex   = (absindex % numwidths);
    
    if (refindex < 0) {
      refindex += numwidths;
      --numperiods;
    }

    SUASSERT (refindex >= 0 && refindex < numwidths, "");
    SUASSERT (refindex + numwidths * numperiods == absindex, "");

    sutype::dcoord_t centerline = 0;

    for (sutype::svi_t i=1; i <= refindex; ++i) {

      sutype::dcoord_t prevwidth = widths [i-1];
      sutype::dcoord_t prevspace = spaces [i-1];
      sutype::dcoord_t currwidth = widths [i];

      SUASSERT (prevwidth % 2 == 0, "");
      SUASSERT (currwidth % 2 == 0, "");
      
      centerline += (prevwidth / 2 + prevspace + currwidth / 2);
    }

    centerline += metalTemplate()->pitch() * numperiods;
    centerline += _absoffset[sutype::gd_pgd];
    
    return centerline;
    
  } // suMetalTemplateInstance::get_center_line

  //
  sutype::dcoords_t suMetalTemplateInstance::get_center_lines (sutype::svi_t period0,
                                                               sutype::svi_t period1)
    const
  {
    SUASSERT (period0 <= period1, "");

    const sutype::dcoords_t & widths = metalTemplate()->get_widths (isInverted());
    const sutype::svi_t numwidths = (sutype::svi_t)widths.size();
    
    const sutype::svi_t absindex0 = numwidths * period0;
    const sutype::svi_t absindex1 = numwidths * (period1 + 1) - 1;
    
    SUASSERT (absindex0 <= absindex1, "");
    
    sutype::dcoords_t centerlines;

    for (sutype::svi_t absindex = absindex0; absindex <= absindex1; ++absindex) {

      centerlines.push_back (get_center_line (absindex));
    }

    return centerlines;
    
  } // end of suMetalTemplateInstance::get_center_lines

  // leftmost edge of the first wire of the template (in case of vertical layer)
  sutype::dcoord_t suMetalTemplateInstance::get_first_track_sidel ()
    const
  {
    const sutype::dcoords_t & widths = metalTemplate()->get_widths (isInverted());
    SUASSERT (!widths.empty(), "");
    const sutype::dcoord_t width0 = widths.front();
    
    return (_absoffset[sutype::gd_pgd] - width0 / 2);
    
  } // end of suMetalTemplateInstance::get_first_track_sidel
  
  // add/subtract integer number of pitches
  sutype::dcoord_t suMetalTemplateInstance::move_coord_inside_first_metal_template_instance (sutype::dcoord_t abscoord)
    const
  {
    // leftmost edge of the first wire of the template
    const sutype::dcoord_t firstracksidel = get_first_track_sidel();
    const sutype::dcoord_t templatepitch = _metalTemplate->pitch();
    
    sutype::dcoord_t refcoord = abscoord - firstracksidel;
    
    sutype::dcoord_t numpitches = refcoord / templatepitch;
    refcoord -= (numpitches * templatepitch);
    
    if (refcoord < 0)
      refcoord += templatepitch;
    
    SUASSERT (refcoord >= 0, "");
    SUASSERT (refcoord < templatepitch, "");
    
    refcoord += firstracksidel;
    
    return refcoord;
    
  } // end of suMetalTemplateInstance::move_coord_inside_first_metal_template_instance
  
  //
  std::string suMetalTemplateInstance::to_str ()
    const
  {
    std::ostringstream oss;

    oss
      << "{"
      << _metalTemplate->name()
      << "; pgdoffset=" << _absoffset[sutype::gd_pgd]
      << "; ogdoffset=" << _absoffset[sutype::gd_ogd]
      << "; region=" << _region.to_str()
      << "; inverted=" << (int)isInverted()
      << "}";
    
    return oss.str();
    
  } // end of suMetalTemplateInstance::to_str

  //
  sutype::dcoord_t suMetalTemplateInstance::get_line_end_on_the_grid (sutype::dcoord_t absregionedgel,
                                                                      bool lower)
    const
  {
    const sutype::dcoords_t & stops = _metalTemplate->stops();
    if (stops.empty()) return absregionedgel; // no LE grid

    // mt
    const sutype::dcoord_t period = _metalTemplate->dperiod (sutype::gd_ogd);
    SUASSERT (period > 0, "");

    // mti
    const sutype::dcoord_t offset = _absoffset [sutype::gd_ogd];
    SUASSERT (offset >= 0, "");
    SUASSERT (offset < period, "");
    
    // calculations
    sutype::dcoord_t relregionedgel = absregionedgel - offset;
    sutype::dcoord_t numperiods = relregionedgel / period;
    sutype::dcoord_t relregionedgel0 = numperiods * period;
    
    if (relregionedgel0 > relregionedgel)
      relregionedgel0 -= period;

    if (relregionedgel0 + period <= relregionedgel)
      relregionedgel0 += period;
    
    sutype::dcoord_t absregionedgel0 = offset + relregionedgel0;
    
    SUASSERT (absregionedgel0 <= absregionedgel, _metalTemplate->name());
    SUASSERT (absregionedgel0 + period > absregionedgel, _metalTemplate->name());

    // input edge falls onto the grid
    if (absregionedgel == absregionedgel0) return absregionedgel;
    
    sutype::dcoord_t minlineend = absregionedgel0;
    sutype::dcoord_t lineend = minlineend;

    SUASSERT (minlineend < absregionedgel, "");
    
    for (sutype::svi_t i = 0; i < (sutype::svi_t)stops.size(); ++i) {
      
      sutype::dcoord_t wirelength = stops[i];
      SUASSERT (wirelength > 0, "");
      lineend += wirelength;

      if (lower) {
        if      (lineend == absregionedgel) return absregionedgel;
        else if (lineend >  absregionedgel) return minlineend;
        else {
          minlineend = lineend;
          continue;
        }
      }
      else {
        if      (lineend == absregionedgel) return absregionedgel;
        else if (lineend >  absregionedgel) return lineend;
        else {
          // do nothing
        }
      }
    }

    SUASSERT (false, "");

    return absregionedgel;
    
  } // end of suMetalTemplateInstance::get_line_end_on_the_grid

  // absregionedgel must match the OGD grid
  sutype::svi_t suMetalTemplateInstance::get_grid_line_index (sutype::dcoord_t absregionedgel)
    const
  {
    const sutype::dcoords_t & stops = _metalTemplate->stops();
    SUASSERT (!stops.empty(), "");

    // mt
    const sutype::dcoord_t period = _metalTemplate->dperiod (sutype::gd_ogd);
    SUASSERT (period > 0, "");

    // mti
    const sutype::dcoord_t offset = _absoffset [sutype::gd_ogd];
    SUASSERT (offset >= 0, "");
    SUASSERT (offset < period, "");
    
    // calculations
    sutype::dcoord_t relregionedgel = absregionedgel - offset;
    sutype::dcoord_t numperiods = relregionedgel / period;
    sutype::dcoord_t relregionedgel0 = numperiods * period;
    
    if (relregionedgel0 > relregionedgel)
      relregionedgel0 -= period;

    if (relregionedgel0 + period <= relregionedgel)
      relregionedgel0 += period;
    
    sutype::dcoord_t absregionedgel0 = offset + relregionedgel0;
    
    SUASSERT (absregionedgel0 <= absregionedgel, _metalTemplate->name());
    SUASSERT (absregionedgel0 + period > absregionedgel, _metalTemplate->name());

    // input edge falls onto the grid
    if (absregionedgel == absregionedgel0) return 0;

    sutype::dcoord_t minlineend = absregionedgel0;
    sutype::dcoord_t maxlineend = minlineend + period;

    SUASSERT (minlineend < absregionedgel, "");
    SUASSERT (maxlineend > absregionedgel, "");
    
    sutype::dcoord_t lineend = minlineend;
    
    for (sutype::svi_t i = 0; i < (sutype::svi_t)stops.size(); ++i) {
      
      sutype::dcoord_t wirelength = stops[i];
      SUASSERT (wirelength > 0, "");
      lineend += wirelength;
      
      if      (lineend == absregionedgel) return (i+1);
      else if (lineend >= maxlineend) {
        SUASSERT (false, "");
      }
    }

    SUASSERT (false, "absregionedgel doesn't match OGD grid");
    
    return -1;
    
  } // end of suMetalTemplateInstance::get_grid_line_index

  // absregionedgel must match the OGD grid
  sutype::dcoord_t suMetalTemplateInstance::get_next_line_end_on_the_grid (sutype::dcoord_t absregionedgel)
    const
  {
    const sutype::dcoords_t & stops = _metalTemplate->stops();
    SUASSERT (!stops.empty(), "");

    sutype::svi_t gridLineIndex = get_grid_line_index (absregionedgel);

    SUASSERT (gridLineIndex >= 0 && gridLineIndex < (sutype::svi_t)stops.size(), "");
    
    return (absregionedgel + stops[gridLineIndex]);
    
  } // end of suMetalTemplateInstance::get_next_line_end_on_the_grid

  //
  sutype::dcoords_t suMetalTemplateInstance::get_line_ends_in_rectangle (const suRectangle & queryrect)
    const
  {
    sutype::dcoords_t lineends;

    const sutype::dcoords_t & stops = _metalTemplate->stops();
    if (stops.empty()) return lineends;
    
    // mt
    const sutype::dcoord_t period = _metalTemplate->dperiod (sutype::gd_ogd);
    SUASSERT (period > 0, "");

    // mti
    const sutype::dcoord_t offset = _absoffset [sutype::gd_ogd];
    SUASSERT (offset >= 0, "");
    SUASSERT (offset < period, "");

    // queryrect
    const sutype::dcoord_t absregionedgel = queryrect.edgel (_metalTemplate->baseLayer()->pgd());
    const sutype::dcoord_t absregionedgeh = queryrect.edgeh (_metalTemplate->baseLayer()->pgd());

    SUASSERT (absregionedgeh >= absregionedgel, "");

    // calculations
    sutype::dcoord_t relregionedgel = absregionedgel - offset;
    sutype::dcoord_t numperiods = relregionedgel / period;
    sutype::dcoord_t relregionedgel0 = numperiods * period;
    
    if (relregionedgel0 > relregionedgel)
      relregionedgel0 -= period;

    if (relregionedgel0 + period <= relregionedgel)
      relregionedgel0 += period;
    
    sutype::dcoord_t absregionedgel0 = offset + relregionedgel0;
    
    SUASSERT (absregionedgel0 <= absregionedgel, _metalTemplate->name());
    SUASSERT (absregionedgel0 + period > absregionedgel, _metalTemplate->name());

    sutype::dcoord_t lineend = absregionedgel0;
    if (lineend >= absregionedgel && lineend <= absregionedgeh) {
      lineends.push_back (lineend);
    }
    
    while (1) {

      if (lineend > absregionedgeh) break;
      
      for (sutype::uvi_t i=0; i < stops.size(); ++i) {
        sutype::dcoord_t wirelength = stops[i];
        SUASSERT (wirelength > 0, "");
        lineend += wirelength;
        if (lineend > absregionedgeh) break;
        if (lineend >= absregionedgel && lineend <= absregionedgeh) {
          lineends.push_back (lineend);
        }
      }
    }

    if (0) {
      SUINFO(1) << "================================================" << std::endl;
      SUINFO(1) << "mt=" << _metalTemplate->name() << std::endl;
      SUINFO(1) << "stops=" << suStatic::to_str(stops) << std::endl;
      SUINFO(1) << "period=" << period << std::endl;
      SUINFO(1) << "offset=" << offset << std::endl;
      SUINFO(1) << "absregionedgel=" << absregionedgel << std::endl;
      SUINFO(1) << "absregionedgeh=" << absregionedgeh << std::endl;
      SUINFO(1) << "relregionedgel=" << relregionedgel << std::endl;
      SUINFO(1) << "numperiods=" << numperiods << std::endl;
      SUINFO(1) << "absregionedgel0=" << absregionedgel0 << std::endl;
      SUINFO(1) << "lineend0=" << lineends.front() << std::endl;
    }
    
    return lineends;
    
  } // end of suMetalTemplateInstance::get_line_ends_in_rectangle

  //
  void suMetalTemplateInstance::create_wires_in_rectangle (const suRectangle & rect,
                                                           const suNet * net,
                                                           const sutype::wire_type_t wiretype,
                                                           bool discretizeWires,
                                                           sutype::wires_t & wires)
    const
  {    
    // side: x for ver ; y for hor
    // edge: y for ver ; x for hor
    const sutype::dcoord_t absregionsidel = rect.sidel (_metalTemplate->baseLayer()->pgd());
    const sutype::dcoord_t absregionsideh = rect.sideh (_metalTemplate->baseLayer()->pgd());
    const sutype::dcoord_t absregionedgel = rect.edgel (_metalTemplate->baseLayer()->pgd());
    const sutype::dcoord_t absregionedgeh = rect.edgeh (_metalTemplate->baseLayer()->pgd());
    
    const sutype::dcoord_t refregionsidel = move_coord_inside_first_metal_template_instance (absregionsidel);
    const sutype::dcoord_t shift = (absregionsidel - refregionsidel);
    SUASSERT (shift % _metalTemplate->pitch() == 0, "");
    
    const sutype::layers_tc & layers = _metalTemplate->get_layers(isInverted());
    const sutype::dcoords_t & widths = _metalTemplate->get_widths(isInverted());
    const sutype::dcoords_t & spaces = _metalTemplate->get_spaces(isInverted());

    SUASSERT (!layers.empty(), "");
    SUASSERT (!widths.empty(), "");
    SUASSERT (!spaces.empty(), "");
    SUASSERT (widths.size() == spaces.size(), "");
    SUASSERT (widths.size() == layers.size(), "");

    sutype::dcoord_t prevtracksidel = shift + get_first_track_sidel();
    sutype::svi_t numperiods = 0;
      
    do {
      
      bool repeat = true;
      
      // do not count the last wire; it must be equal to the first
      for (sutype::uvi_t i=0; i < widths.size(); ++i) {
        
        const suLayer *        layer = layers[i];
        const sutype::dcoord_t width = widths[i];
        const sutype::dcoord_t space = spaces[i];
        
        SUASSERT (layer, "");
        SUASSERT (width > 0, "");
        SUASSERT (space > 0, "");
        SUASSERT (width % 2 == 0, "");
        
        const sutype::dcoord_t tracksidel = prevtracksidel;
        const sutype::dcoord_t tracksideh = tracksidel + width;

        // increment
        prevtracksidel = tracksideh + space;
        
        // double check the center line
        if (1) {
          const sutype::dcoord_t tracksidec = (tracksidel + width / 2);
          sutype::svi_t absindex = (sutype::svi_t)i + (shift / _metalTemplate->pitch() + numperiods) * (sutype::svi_t)widths.size();
          const sutype::dcoord_t centerline = get_center_line (absindex);
          SUASSERT (centerline == tracksidec, "");
        }
        //
        
        // create a wire inside mti's region and inside the input rect
        if (wire_sidel_sideh_is_inside_region (tracksidel, tracksideh) &&
            wire_sidel_sideh_is_inside_region (tracksidel, tracksideh, rect)) {
          
          sutype::dcoord_t wireedgel = absregionedgel;
          sutype::dcoord_t wireedgeh = absregionedgeh;
          sutype::dcoord_t wiresidel = tracksidel;
          sutype::dcoord_t wiresideh = tracksideh;

          // target rect may be outside the mti's region
          if (wireedgel < edgel()) wireedgel = edgel();
          if (wireedgeh > edgeh()) wireedgeh = edgeh();

          wireedgel = get_line_end_on_the_grid (wireedgel, true);
          wireedgeh = get_line_end_on_the_grid (wireedgeh, false);
          
          if (wireedgel < wireedgeh) {

            // create several wire discretes to match OGD
            if (discretizeWires) {
              
              sutype::dcoord_t discretizedwireedgel = wireedgel;
              while (discretizedwireedgel < wireedgeh) {
                sutype::dcoord_t discretizedwireedgeh = get_next_line_end_on_the_grid (discretizedwireedgel);
                suWire * wire = suWireManager::instance()->create_wire_from_edge_side (net, layer,
                                                                                       discretizedwireedgel,
                                                                                       discretizedwireedgeh,
                                                                                       wiresidel, wiresideh, wiretype); // requested side-2-side wire
                wires.push_back (wire);
                discretizedwireedgel = discretizedwireedgeh;
              }
            }
            else {
              suWire * wire = suWireManager::instance()->create_wire_from_edge_side (net, layer, wireedgel, wireedgeh, wiresidel, wiresideh, wiretype); // requested side-2-side wire
              wires.push_back (wire);
            }
            
          }
        }
        
        // tracks went out of the input rect
        if (tracksidel > absregionsideh) {
          repeat = false;
          break;
        }
      }
      
      if (!repeat) break;
      
      ++numperiods;
      
    } while (true);    
        
  } // end of suMetalTemplateInstance::create_wires_in_rectangle


  //
  bool suMetalTemplateInstance::wire_could_be_within_one_of_tracks (const suWire * wire)
    const
  {
    const sutype::dcoord_t firsttracksidel = get_first_track_sidel();
      
    const sutype::layers_tc & layers = metalTemplate()->get_layers (isInverted());
    const sutype::dcoords_t & widths = metalTemplate()->get_widths (isInverted());
    const sutype::dcoords_t & spaces = metalTemplate()->get_spaces (isInverted());
      
    SUASSERT (!layers.empty(), "");
    SUASSERT (!widths.empty(), "");
    SUASSERT (!spaces.empty(), "");
    SUASSERT (widths.size() == spaces.size(), "");
    SUASSERT (widths.size() == layers.size(), "");

    sutype::dcoord_t prevtracksidel = firsttracksidel;

    sutype::dcoord_t refwiresidel = move_coord_inside_first_metal_template_instance (wire->sidel());
    sutype::dcoord_t refwiresideh = refwiresidel + (wire->sideh() - wire->sidel());
    
    // do not count the last wire; it must be equal to the first
    for (sutype::uvi_t i=0; i < widths.size(); ++i) {
        
      const suLayer *        layer = layers[i];
      const sutype::dcoord_t width = widths[i];
      const sutype::dcoord_t space = spaces[i];
        
      SUASSERT (layer, "");
      SUASSERT (width > 0, "");
      SUASSERT (space > 0, "");
        
      const sutype::dcoord_t tracksidel = prevtracksidel;
      const sutype::dcoord_t tracksideh = tracksidel + width;
      
      SUASSERT (refwiresidel >= tracksidel, "");
          
      // time to increment
      prevtracksidel = tracksideh + space;

      if (layer != wire->layer()) continue;
      
      if (refwiresidel >= tracksidel && refwiresideh <= tracksideh) return true;
    }

    return false;

  } // end of suMetalTemplateInstance::wire_could_be_within_one_of_tracks

  // ------------------------------------------------------------
  // -
  // --- Private static methods
  // -
  // ------------------------------------------------------------


  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------


} // end of namespace amsr

// end of suMetalTemplateInstance.cpp
