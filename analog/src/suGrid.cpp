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
//! \date   Wed Jan 31 12:39:00 2018

//! \file   suGrid.cpp
//! \brief  A collection of methods of the class suGrid.

// std includes
#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suGenerator.h>
#include <suLayer.h>
#include <suLayerManager.h>
#include <suMetalTemplate.h>
#include <suMetalTemplateInstance.h>
#include <suRectangle.h>

// module include
#include <suGrid.h>

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
  suGrid::suGrid (const suMetalTemplateInstance * vermti,
                  const suMetalTemplateInstance * hormti,
                  const sutype::dcoords_t & xdcoords1,
                  const sutype::dcoords_t & ydcoords1,
                  sutype::dcoord_t xperiod,
                  sutype::dcoord_t yperiod)
  {
    init_ ();

    _mtis [sutype::go_ver] = vermti;
    _mtis [sutype::go_hor] = hormti;
      
    sutype::dcoords_t & xdcoords0 = _dcoords [sutype::go_ver];
    sutype::dcoords_t & ydcoords0 = _dcoords [sutype::go_hor];
      
    for (const auto & iter : xdcoords1) { xdcoords0.push_back (iter); }
    for (const auto & iter : ydcoords1) { ydcoords0.push_back (iter); }

    _dperiod [sutype::go_ver] = xperiod;
    _dperiod [sutype::go_hor] = yperiod;

    SUASSERT (sanity_check (true), "");
      
    std::vector <sutype::generators_tc> tmpvector (ydcoords0.size());
    _generators.resize (xdcoords0.size(), tmpvector);
    
  } // end of suGrid::suGrid

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
  void suGrid::get_dcoords (const suRectangle & queryrect,
                            sutype::dcoords_t & xdcoords,
                            sutype::dcoords_t & ydcoords,
                            int dxl,
                            int dyl,
                            int dxh,
                            int dyh)
    
    const
  {
    SUASSERT (xdcoords.empty(), "");
    SUASSERT (ydcoords.empty(), "");

    // mode 0: ver
    // mode 1: hor
    for (int mode = 0; mode <= 1; ++mode) {

      sutype::grid_orientation_t gd = (sutype::grid_orientation_t)mode;

      const sutype::dcoords_t & dcoords = _dcoords [mode];
      SUASSERT (!dcoords.empty(), "");
      sutype::dcoord_t dcoord0 = dcoords.front();
      
      sutype::dcoord_t dperiod = _dperiod [mode];
      SUASSERT (dperiod > 0, "");

      sutype::dcoord_t rangel = (mode == 0) ? queryrect.xl() : queryrect.yl();
      sutype::dcoord_t rangeh = (mode == 0) ? queryrect.xh() : queryrect.yh();

      int numperiods = (rangel - dcoord0) / dperiod;
      if ((dcoord0 + numperiods * dperiod) > rangel)
        --numperiods;
      SUASSERT ((dcoord0 + numperiods * dperiod) <= rangel, "");

      sutype::dcoords_t & outdcoords = (mode == 0) ? xdcoords : ydcoords;

      sutype::gcoord_t mingcoord = 0;
      sutype::gcoord_t maxgcoord = 0;
      
      // default
      while (1) {
        bool repeat = true;
        for (sutype::uvi_t i=0; i < dcoords.size(); ++i) {
          
          sutype::gcoord_t refgcoord = i;
          sutype::dcoord_t refdcoord = dcoords[i];

          sutype::gcoord_t gcoord = refgcoord + numperiods * dcoords.size();
          
          sutype::dcoord_t tstdcoord = get_dcoord_by_gcoord (gd, gcoord);
          
          sutype::dcoord_t dcoord = refdcoord + numperiods * dperiod;
          
          SUASSERT (dcoord == tstdcoord, "");
          
          if (dcoord < rangel) continue;
          if (dcoord > rangeh) { repeat = false; break; }

          outdcoords.push_back (dcoord);

          if (outdcoords.size() == 1) {
            mingcoord = gcoord;
          }
          maxgcoord = gcoord;
        }
        if (!repeat) break;
        ++numperiods;
      }

      // nothing to trim or add
      if (outdcoords.empty()) continue;

      // trim dcoords inside queryrect of add more dcoords outdcoords queryrect
      int dl = (gd == sutype::go_ver) ? dxl : dyl;
      int dh = (gd == sutype::go_ver) ? dxh : dyh;
      
      // nothing to trim or to add
      if (dl == 0 && dh == 0) continue;
      
      mingcoord += dl;
      maxgcoord += dh;
      
      outdcoords.clear();
      
      for (sutype::gcoord_t gcoord = mingcoord; gcoord <= maxgcoord; ++gcoord) {
        sutype::dcoord_t dcoord = get_dcoord_by_gcoord (gd, gcoord);
        outdcoords.push_back (dcoord);
      }
    }
    
  } // end of suGrid::get_dcoords

  //
  bool suGrid::sanity_check (bool assertOnError)
    const
  {
    for (int i=0; i <= 1; ++i) {
      
      sutype::grid_orientation_t gd = (sutype::grid_orientation_t)i;

      if (_dcoords[i].empty()) { SUASSERT (!assertOnError, ""); return false; }

      const sutype::dcoord_t dperiod = get_dperiod (gd);
      if (dperiod <= 0) { SUASSERT (!assertOnError, ""); return false; }
      
      for (sutype::uvi_t k=1; k < _dcoords[i].size(); ++k) {
        if (_dcoords[i][k] <= _dcoords[i][k-1]) { SUASSERT (!assertOnError, ""); return false; }
      }
      
      sutype::dcoord_t dcoord0 = _dcoords[i].front();
      sutype::dcoord_t dcoord1 = _dcoords[i].back();
      sutype::dcoord_t dcoord2 = dcoord0 + dperiod;
      
      if (dcoord1 >= dcoord2) { SUASSERT (!assertOnError, ""); return false; }
    }
    
    return true;
    
  } // end of suGrid::sanity_check

  //
  sutype::dcoord_t suGrid::get_dcoord_by_gcoord (sutype::grid_orientation_t gd,
                                                 sutype::gcoord_t gcoord)
    const
  {
    SUASSERT (gd == sutype::go_ver || gd == sutype::go_hor, "");

    const sutype::gcoord_t gperiod = get_gperiod (gd);
    const sutype::dcoord_t dperiod = get_dperiod (gd);
    
    sutype::gcoord_t numperiods = gcoord / gperiod;
    sutype::gcoord_t refindex   = gcoord % gperiod;
    
    if (refindex < 0) {
      refindex += gperiod;
      --numperiods;
    }
    
    SUASSERT (refindex >= 0 && refindex < gperiod, "");
    SUASSERT (numperiods * gperiod + refindex == gcoord, "");

    sutype::dcoord_t refdcoord = _dcoords [gd][refindex];
    sutype::dcoord_t absdcoord = refdcoord + numperiods * dperiod;
    
    return absdcoord;
    
  } // end of suGrid::get_dcoord_by_gcoord

  //
  sutype::dcoords_t suGrid::get_grid_line_indices (sutype::grid_orientation_t gd,
                                                   const suGenerator * generator)
    const
  {
    SUASSERT (gd == sutype::go_ver || gd == sutype::go_hor, "");
    
    sutype::gcoord_t mingx = 0;
    sutype::gcoord_t maxgx = 0;
    sutype::gcoord_t mingy = 0;
    sutype::gcoord_t maxgy = 0;

    if (gd == sutype::go_ver) maxgy = get_y_gperiod() - 1;
    if (gd == sutype::go_hor) maxgx = get_x_gperiod() - 1;

    SUASSERT (mingx <= maxgx, "");
    SUASSERT (mingy <= maxgy, "");

    sutype::dcoords_t gridLineIndices;

    for (sutype::gcoord_t gx=0; gx <= maxgx; ++gx) {
      for (sutype::gcoord_t gy=0; gy <= maxgy; ++gy) {
        const sutype::generators_tc & generators = _generators[gx][gy];
        if (std::find (generators.begin(), generators.end(), generator) == generators.end()) continue;
        sutype::gcoord_t g = (gd == sutype::go_ver) ? gy : gx;
        SUASSERT (std::find (gridLineIndices.begin(), gridLineIndices.end(), g) == gridLineIndices.end(), "");
        gridLineIndices.push_back (g);
      }
    }

    return gridLineIndices;
    
  } // end of suGrid::get_grid_line_indices

  //
  sutype::gcoord_t suGrid::get_grid_line_index_by_dcoord (sutype::grid_orientation_t gd,
                                                          sutype::dcoord_t dcoord1)
    const
  {
    const sutype::dcoord_t dperiod = get_dperiod (gd);
    const sutype::dcoord_t dcoord0 = get_dcoord_by_gcoord (gd, 0);
    
    sutype::dcoord_t refdcoord0 = (dcoord1 - dcoord0);

    sutype::dcoord_t numperiods = refdcoord0 / dperiod;
    sutype::dcoord_t refdcoord1 = refdcoord0 % dperiod;
    if (refdcoord1 < 0) {
      refdcoord1 += dperiod;
      --numperiods;
    }

    SUASSERT (refdcoord1 >= 0 && refdcoord1 < dperiod, "");
    SUASSERT (numperiods * dperiod + refdcoord1 == refdcoord0, "");

    sutype::dcoord_t refdcoord2 = refdcoord1 + dcoord0;
    
    for (sutype::uvi_t k=0; k < _dcoords[gd].size(); ++k) {
      sutype::dcoord_t dcoord2 = _dcoords[gd][k];
      if (dcoord2 == refdcoord2) return (sutype::gcoord_t)k;
    }
    
    SUASSERT (false, "dcoord=" << dcoord1 << " doesn't match any grid line");
    
    return -1;
    
  } // end of suGrid::get_grid_line_index_by_dcoord

  //
  sutype::gcoord_t suGrid::get_grid_line_index_by_gcoord (sutype::grid_orientation_t gd,
                                                          sutype::dcoord_t gcoord)
    const
  {
    const sutype::gcoord_t gperiod = get_gperiod (gd);
    SUASSERT (gperiod > 0, "");
    
    sutype::gcoord_t refgcoord = gcoord % gperiod;
    if (refgcoord < 0) {
      refgcoord += gperiod;
    }
    
    SUASSERT (refgcoord >= 0 && refgcoord < gperiod, "");
    
    return refgcoord;
    
  } // end of suGrid::get_grid_line_index_by_gcoord

  // very simple implementation for now
  void suGrid::add_generator (const suGenerator * generator,
                              sutype::dcoord_t xdcoord,
                              sutype::dcoord_t ydcoord)
  {
    sutype::gcoord_t xgli = get_grid_line_index_by_x_dcoord (xdcoord);
    sutype::gcoord_t ygli = get_grid_line_index_by_y_dcoord (ydcoord);
    
    SUASSERT (xgli >= 0 && xgli < get_x_gperiod(), "");
    SUASSERT (ygli >= 0 && ygli < get_y_gperiod(), "");
    
    SUASSERT (xgli >= 0 && xgli < (sutype::gcoord_t)_generators.size(), "");
    SUASSERT (ygli >= 0 && ygli < (sutype::gcoord_t)_generators[xgli].size(), "");
    
    (_generators[xgli][ygli]).push_back (generator);
    
  } // end of suGrid::add_generator

  //
  const sutype::generators_tc & suGrid::get_generators_at_gpoint (sutype::gcoord_t xgcoord,
                                                                  sutype::gcoord_t ygcoord)
    const
  {
    sutype::gcoord_t xgli = get_grid_line_index_by_gcoord (sutype::go_ver, xgcoord);
    sutype::gcoord_t ygli = get_grid_line_index_by_gcoord (sutype::go_hor, ygcoord);
    
    return _generators [xgli][ygli];
    
  } // end of suGrid::get_generators_at_gpoint

  //
  const sutype::generators_tc & suGrid::get_generators_at_dpoint (sutype::dcoord_t xdcoord,
                                                                  sutype::dcoord_t ydcoord)
    const
  {
    sutype::gcoord_t xgli = get_grid_line_index_by_dcoord (sutype::go_ver, xdcoord);
    sutype::gcoord_t ygli = get_grid_line_index_by_dcoord (sutype::go_hor, ydcoord);
    
    return _generators [xgli][ygli];
    
  } // end of suGrid::get_generators_at_dpoint

  //
  const sutype::generators_tc & suGrid::get_generators_of_base_layer (const suLayer * layer)
    const
  {
    SUASSERT (layer, "");
    SUASSERT (layer->is_base(), "");
    SUASSERT (!_generatorsOfBaseLayer.empty(), "");

    sutype::id_t id = layer->pers_id();
    SUASSERT (id >= 0, "");
    SUASSERT (id < (sutype::id_t)_generatorsOfBaseLayer.size(), "");
    
    return _generatorsOfBaseLayer[id];
    
  } // end of suGrid::get_generators_of_base_layer
  
  //
  suRectangle suGrid::get_generator_shape (const suGenerator * generator,
                                           sutype::gcoord_t gx,
                                           sutype::gcoord_t gy,
                                           sutype::via_generator_layer_t vgl)
    const
  {
    sutype::dcoord_t dx = get_dcoord_by_gcoord (sutype::go_ver, gx);
    sutype::dcoord_t dy = get_dcoord_by_gcoord (sutype::go_hor, gy);
    
    suRectangle shape = generator->get_shape (dx, dy);
    
    return shape;
    
  } // end of suGrid::get_generator_shape

  //
  void suGrid::print ()
    const
  {
    SUASSERT (_mtis [sutype::go_ver], "");
    SUASSERT (_mtis [sutype::go_hor], "");

    SUINFO(1) << "vermti: " << _mtis [sutype::go_ver]->to_str() << "; " << _mtis [sutype::go_ver]->metalTemplate()->baseLayer()->name() << std::endl;
    SUINFO(1) << "hormti: " << _mtis [sutype::go_hor]->to_str() << "; " << _mtis [sutype::go_hor]->metalTemplate()->baseLayer()->name() << std::endl;
    
    SUINFO(1) << "gxperiod: " << get_x_gperiod() << std::endl;
    SUINFO(1) << "gyperiod: " << get_y_gperiod() << std::endl;

    SUINFO(1) << "dxperiod: " << get_x_dperiod() << std::endl;
    SUINFO(1) << "dyperiod: " << get_y_dperiod() << std::endl;
    
    for (sutype::uvi_t i=0; i < _generators.size(); ++i) {

      for (sutype::uvi_t k=0; k < _generators[i].size(); ++k) {

        const sutype::generators_tc & generators = _generators[i][k];

        SUINFO(1) << "(" << i << "," << k << "):";
        
        for (const auto & iter : generators) {

          const suGenerator * generator = iter;
          SUOUT(1) << " " << generator->name();
        }
        SUOUT(1) << std::endl;
      }
    }
    
  } // end of suGrid::print

  //
  void suGrid::precompute_data ()
  {
    SUASSERT (_generatorsOfBaseLayer.empty(), "");

    calculate_generators_of_base_layers_ ();
    
  } // end of suGrid::precompute_data
  
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

  //
  void suGrid::calculate_generators_of_base_layers_ ()
  {
    SUASSERT (_generatorsOfBaseLayer.empty(), "");

    const sutype::layers_tc & layers = suLayerManager::instance()->layers();
    SUASSERT (!layers.empty(), "");

    if (layers.empty()) return;

    sutype::id_t maxindex = 0;
    sutype::layers_tc baselayers;

    for (const auto & iter : layers) {

      const suLayer * layer = iter;
      if (!layer->is_base()) continue;

      sutype::id_t id = layer->pers_id();
      SUASSERT (id >= 0, "");

      maxindex = std::max (maxindex, id);
      baselayers.push_back (layer);
    }

    SUASSERT (maxindex >= 0, "");

    _generatorsOfBaseLayer.resize (maxindex+1);

    for (const auto & iter : baselayers) {

      const suLayer * layer = iter;
      SUASSERT (layer->is_base(), "");
      
      sutype::id_t id = layer->pers_id();
      SUASSERT (id >= 0 && id < (sutype::id_t)_generatorsOfBaseLayer.size(), "");

      _generatorsOfBaseLayer[id] = get_generators_by_base_layer_ (layer);
    }
    
  } // end of calculate_generators_of_base_layers_

  //
  sutype::generators_tc suGrid::get_generators_by_base_layer_ (const suLayer * targetbaselayer)
    const
  {
    SUASSERT (targetbaselayer, "");
    SUASSERT (targetbaselayer->is_base(), "");

    std::set<const suGenerator*, suGenerator::cmp_const_ptr> setOfGenerators;

    for (const auto & iter1 : _generators) {
      for (const auto & iter2 : iter1) {
        for (const auto & iter3 : iter2) {
          const suGenerator * generator = iter3;
          const suLayer * generatorbaselayer = generator->get_layer(sutype::vgl_cut)->base();
          if (generatorbaselayer != targetbaselayer) continue;
          setOfGenerators.insert (generator);
        }
      }
    }

    sutype::generators_tc generators;

    for (const auto & iter : setOfGenerators) {
      const suGenerator * generator = iter;
      generators.push_back (generator);
    }
    
    return generators;
    
  } // end of suGrid::get_generators_by_base_layer_
  
} // end of namespace amsr

// end of suGrid.cpp
