// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Thu Oct  5 15:44:59 2017

//! \file   suLayerManager.cpp
//! \brief  A collection of methods of the class suLayerManager.

// std includes
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suColoredLayer.h>
#include <suLayer.h>
#include <suOptionManager.h>
#include <suStatic.h>
#include <suToken.h>
#include <suTokenParser.h>

// module include
#include <suLayerManager.h>

namespace amsr
{
  //
  suLayerManager * suLayerManager::_instance = 0;

  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  suLayerManager::~suLayerManager ()
  {
    for (sutype::uvi_t i=0; i < _layers.size(); ++i) {
      delete _layers[i];
    }

    for (sutype::uvi_t i=0; i < _tokenParsers.size(); ++i) {
      delete _tokenParsers[i];
    }
    
  } // end of suLayerManager::~suLayerManager

  // ------------------------------------------------------------
  // -
  // --- Public static methods
  // -
  // ------------------------------------------------------------
  
  //
  void suLayerManager::delete_instance ()
  {
    if (suLayerManager::_instance)
      delete suLayerManager::_instance;
    
    suLayerManager::_instance = 0;
    
  } // end of suLayerManager::delete_instance

  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  //
  const suLayer * suLayerManager::get_base_layer_by_name (const std::string & name)
    const
  {
    for (sutype::uvi_t i=0; i < _layers.size(); ++i) {
      
      const suLayer * layer = _layers[i];
      if (layer->has_name (name)) return layer;
    }
    
    return 0;
    
  } // end of suLayerManager::get_base_layer_by_name
  
  //
  const suLayer * suLayerManager::get_colored_layer_by_name (const std::string & name)
    const
  {
    for (sutype::uvi_t i=0; i < _layers.size(); ++i) {
      
      const suLayer * layer = _layers[i];
      
      const suLayer * coloredlayer = layer->get_colored_layer_by_name (name);
      if (coloredlayer) return coloredlayer;
    }
    
    return get_base_layer_by_name (name);
    
  } // end of suLayerManager::get_colored_layer_by_name

  //
  const suLayer * suLayerManager::get_base_layer_by_level (int level)
    const
  {
    for (sutype::uvi_t i=0; i < _layers.size(); ++i) {

      const suLayer * layer = _layers[i];
      if (layer->level() == level) return layer;
    }
    
    return 0;
    
  } // end suLayerManager::get_base_layer_by_level

  //
  const suLayer * suLayerManager::get_base_layer_by_type (int type)
    const
  {
    for (sutype::uvi_t i=0; i < _layers.size(); ++i) {
      
      const suLayer * layer = _layers[i];
      if (layer->is (type)) return layer;
    }
    
    return 0;
    
  } // end of suLayerManager::get_base_layer_by_type

   //
  const suLayer * suLayerManager::get_base_adjacent_layer (const suLayer * reflayer,
                                                           int type,
                                                           bool upper,
                                                           unsigned numSkipTracks)
    const
  {
    sutype::svi_t index = -1;
    
    // layers are sorted by level
    for (sutype::svi_t i=0; i < (sutype::svi_t)_layers.size(); ++i) {

      SUASSERT (i == 0 || _layers[i-1]->level() <= _layers[i]->level(), "");

      const suLayer * layer = _layers[i];
      if (layer != reflayer->base()) continue;

      index = i;
      break;
    }

    SUASSERT (index >= 0 && index < (sutype::svi_t)_layers.size(), "");
    
    const sutype::svi_t incr = upper ? 1 : -1;
    unsigned num = 0;
    
    for (sutype::svi_t i = index + incr; i >= 0 && i < (sutype::svi_t)_layers.size(); i += incr) {

      const suLayer * layer = _layers[i];
      if (!layer->is (type)) continue;

      if (num < numSkipTracks) {
        ++num;
        continue;
      }

      return layer;
    }

    return 0;
    
  } // end of suLayerManager::get_base_adjacent_layer

  //
  bool suLayerManager::layers_are_adjacent_metal_layers (const suLayer * layer1,
                                                         const suLayer * layer2)
    const
  {
    SUASSERT (layer1, "");
    SUASSERT (layer2, "");
    SUASSERT (layer1->is (sutype::lt_wire), "");
    SUASSERT (layer2->is (sutype::lt_wire), "");
    SUASSERT (layer1->is (sutype::lt_metal), "");
    SUASSERT (layer2->is (sutype::lt_metal), "");

    const suLayer * lowerlayer = (layer1->level() < layer2->level()) ? layer1 : layer2;
    const suLayer * upperlayer = (lowerlayer == layer1) ? layer2 : layer1;

    bool ok1 = (get_base_adjacent_layer (lowerlayer->base(), sutype::lt_wire | sutype::lt_metal, true,  0) == upperlayer->base());
    bool ok2 = (get_base_adjacent_layer (upperlayer->base(), sutype::lt_wire | sutype::lt_metal, false, 0) == lowerlayer->base());

    SUASSERT (ok1 == ok2, "lowerlayer=" << lowerlayer->to_str() << "; upperlayer=" << upperlayer->to_str());

    return ok1;
    
  } // end of suLayerManager::layers_are_adjacent_metal_layers
  
  //
  void suLayerManager::read_layer_file (const std::string & filename)
  {
    suTokenParser * tokenparser = new suTokenParser ();
    _tokenParsers.push_back (tokenparser);
    
    bool ok = tokenparser->read_file (filename);
    SUASSERT (ok, "");
    SUASSERT (tokenparser->rootToken(), "");

    sutype::tokens_t lmtokens = tokenparser->rootToken()->get_tokens ("LayerManager");
    lmtokens.push_back ((suToken*)tokenparser->rootToken());

    sutype::tokens_t tokens;
        
    // collect all tokens
    for (sutype::uvi_t i=0; i < lmtokens.size(); ++i) {
      
      const suToken * lmtoken = lmtokens[i];
      sutype::tokens_t layertokens = lmtoken->get_tokens ("Layer");
      tokens.insert (tokens.end(), layertokens.begin(), layertokens.end());
    }

    // trim token0 (a base layer) if it's actually a color of another layer
    for (sutype::svi_t i=0; i < (sutype::svi_t)tokens.size(); ++i) {
      
      const suToken * token0 = tokens[i];
      const std::string & name0 = token0->get_string_value ("name");

      bool trimtoken0 = false;
      
      for (sutype::svi_t k=0; k < (sutype::svi_t)tokens.size(); ++k) {
        
        if (i == k) continue;
        
        const suToken * token1 = tokens[k];
        const std::string & name1 = token1->get_string_value ("name");

        SUASSERT (name0.compare (name1) != 0, "Layer " << name0 << " defined twice");

        sutype::tokens_t tokens2 = token1->get_tokens ("Color");
        
        for (sutype::uvi_t j=0; j < tokens2.size(); ++j) {
          
          const suToken * token2 = tokens2[j];
          const std::string & name2 = token2->get_string_value ("name");

          if (name2.compare(name0) == 0) {
            trimtoken0 = true;
            break;
          }
        }
        if (trimtoken0) break;
      }

      if (!trimtoken0) continue;

      SUISSUE("Colored layer is defined as a base layer") << ": " << name0 << ". Skipped." << std::endl;

      for (sutype::svi_t k=i; k+1 < (sutype::svi_t)tokens.size(); ++k) {
        tokens[k] = tokens[k+1];
      }
      tokens.pop_back();
      --i;
    }

    // created base layers
    std::vector<suLayer*> layers;
    
    for (sutype::uvi_t k=0; k < tokens.size(); ++k) {
        
      const suToken * token = tokens[k];
      const std::string & name = token->get_string_value ("name");
      SUASSERT (get_base_layer_by_name (name) == 0, "Layer " << name << " defined twice");
      SUASSERT (get_colored_layer_by_name (name) == 0, "Layer " << name << " defined twice");

      suLayer * layer = new suLayer (name);
      layer->token (token); // suLayer is a suTokenOwner
        
      layers.push_back (layer);
      _layers.push_back (layer);

      SUASSERT (layer->pers_id() == (sutype::id_t)_idToLayer.size(), "");
      _idToLayer.push_back (layer);
    }

    // Color
    for (sutype::uvi_t i=0; i < layers.size(); ++i) {

      suLayer * layer1 = layers[i];
      const suToken * token1 = layer1->token();

      sutype::tokens_t tokens2 = token1->get_tokens ("Color");

      for (sutype::uvi_t k=0; k < tokens2.size(); ++k) {

        const suToken * token2 = tokens2[k];
        const std::string & colortype = token2->get_string_value ("type");
        const std::string & colorname = token2->get_string_value ("name");
        
        suColoredLayer * coloredlayer = new suColoredLayer (layer1, colortype, colorname);
        layer1->add_colored_layer (coloredlayer);

        SUASSERT (coloredlayer->pers_id() == (sutype::id_t)_idToLayer.size(), "");
        _idToLayer.push_back (coloredlayer);
      }
    }
    
    // parse single parameters
    for (sutype::uvi_t i=0; i < layers.size(); ++i) {

      suLayer * layer = layers[i];
      const suToken * token = layer->token();

      // other names
      if (token->is_defined ("rpgname")) layer->add_name (token->get_string_value ("rpgname"));
      if (token->is_defined ("genname")) layer->add_name (token->get_string_value ("genname"));
      
      // level
      if (token->is_defined ("level")) {
        int level = token->get_integer_value ("level");
        SUASSERT (level >= 0, "");
        layer->_level = level;
      }

      // pgd; primary grid direction
      // mode 1: pgd
      // mode 2: direction
      for (int mode=1; mode <= 2; ++mode) {
        std::string tokenname;
        if      (mode == 1) tokenname = "pgd";
        else if (mode == 2) tokenname = "direction";
        else {
          SUASSERT (false, "");
        }
        if (!token->is_defined (tokenname)) continue;
        const std::string & str = token->get_string_value (tokenname);
        layer->_pgd = suStatic::str_2_grid_orientaion (str);
        layer->_hasPgd = true;
        break;
      }
    }

    // Type
    for (sutype::uvi_t i=0; i < layers.size(); ++i) {

      suLayer * layer1 = layers[i];
      const suToken * token1 = layer1->token();

      sutype::tokens_t tokens2 = token1->get_tokens ("Type");

      for (sutype::uvi_t k=0; k < tokens2.size(); ++k) {

        const suToken * token2 = tokens2[k];
        const std::string & str = token2->get_string_value ("value");
        layer1->_type |= suStatic::str_2_layer_type (str);
      }
    }

    // OtherNames
    for (sutype::uvi_t i=0; i < layers.size(); ++i) {

      suLayer * layer1 = layers[i];
      const suToken * token1 = layer1->token();

      sutype::tokens_t tokens2 = token1->get_tokens ("OtherNames");
      
      for (sutype::uvi_t k=0; k < tokens2.size(); ++k) {

        const suToken * token2 = tokens2[k];
        sutype::strings_t strs = token2->get_list_of_strings ("value");
        for (const auto & iter : strs) {
          layer1->add_name (iter);
        }
      }
    }

    // RoutingOption
    for (sutype::uvi_t i=0; i < layers.size(); ++i) {

      suLayer * layer1 = layers[i];
      const suToken * token1 = layer1->token();

      sutype::tokens_t tokens2 = token1->get_tokens ("RoutingOption");

      for (sutype::uvi_t k=0; k < tokens2.size(); ++k) {

        const suToken * token2 = tokens2[k];
        
        if (token2->is_defined ("fixed")) layer1->_fixed = token2->get_boolean_value ("fixed");
      }
    }

    // ElectricallyConnected
    for (sutype::uvi_t i=0; i < layers.size(); ++i) {

      suLayer * layer1 = layers[i];
      const suToken * token1 = layer1->token();

      sutype::tokens_t tokens2 = token1->get_tokens ("ElectricallyConnected");

      for (sutype::uvi_t k=0; k < tokens2.size(); ++k) {

        const suToken * token2 = tokens2[k];
        const std::string & str = token2->get_string_value ("layer");
        
        suLayer * layer2 = (suLayer*)get_base_layer_by_name (str);
        if (layer2 == layer1) continue; // just skip this useless info

        if (layer2 == 0) {
          layer2 = (suLayer*)get_colored_layer_by_name (str);
          SUASSERT (layer2, "Can't find ElectricallyConnected layer " << str);
          continue;
        }

        //suLayer * layer2 = (suLayer*)get_colored_layer_by_name (str);
        //SUASSERT (layer2, "Can't find ElectricallyConnected layer " << str);
        
        // add if unique
        if (std::find (layer1->_electricallyConnected.begin(), layer1->_electricallyConnected.end(), layer2) == layer1->_electricallyConnected.end()) layer1->_electricallyConnected.push_back (layer2);
        if (std::find (layer2->_electricallyConnected.begin(), layer2->_electricallyConnected.end(), layer1) == layer2->_electricallyConnected.end()) layer2->_electricallyConnected.push_back (layer1);
      }
    }
    
    sanity_check_ ();
    
    post_reading_ ();

    calculate_lowest_routing_wire_layers_ ();

    calculate_highest_routing_wire_layers_ ();

    SUINFO(1) << "Lowest DR layer is " << (_lowestDRLayer ? _lowestDRLayer->to_str() : "<null>") << std::endl;

    SUINFO(1) << "Highest DR layer is " << (_highestDRLayer ? _highestDRLayer->to_str() : "<null>") << std::endl;
    
    SUINFO(1) << "Lowest GR layer is " << (_lowestGRLayer ? _lowestGRLayer->to_str() : "<null>") << std::endl;
    
  } // end of suLayerManager::read_layer_file

  //
  const suLayer * suLayerManager::get_base_via_layer (const suLayer * layer1,
                                                      const suLayer * layer2)
    const
  {
    if (layer1->pgd() != sutype::go_ver && layer1->pgd() != sutype::go_hor) return 0;
    if (layer2->pgd() != sutype::go_ver && layer2->pgd() != sutype::go_hor) return 0;
    
    if (layer1->pgd() == layer2->pgd()) return 0;
    
    if (!layer1->is (sutype::lt_wire)) return 0;
    if (!layer2->is (sutype::lt_wire)) return 0;
    
    if (layer1->level() < layer2->level()) {
      if (get_base_adjacent_layer (layer1, sutype::lt_wire, true,  0) != layer2->base()) return 0;
      if (get_base_adjacent_layer (layer2, sutype::lt_wire, false, 0) != layer1->base()) return 0;
    }
    else {
      if (get_base_adjacent_layer (layer2, sutype::lt_wire, true,  0) != layer1->base()) return 0;
      if (get_base_adjacent_layer (layer1, sutype::lt_wire, false, 0) != layer2->base()) return 0;
    }
    
    if (abs (layer1->level() - layer2->level()) != 2) return 0;
    
    const suLayer * cutlayer = suLayerManager::instance()->get_base_layer_by_level (layer1->level() + (layer2->level() - layer1->level()) / 2);
    SUASSERT (cutlayer, "");
    SUASSERT (cutlayer->is (sutype::lt_via), layer1->name() << "; " << layer2->name() << "; " << cutlayer->name());
    SUASSERT (cutlayer->is_base(), "");
    
    return cutlayer;
    
  } // end of suLayerManager::get_base_via_layer

  //
  sutype::layers_tc suLayerManager::get_colors (const suLayer * baselayer)
    const
  {
    SUASSERT (baselayer, "");
    SUASSERT (baselayer->is_base(), "");

    sutype::layers_tc layers;

    for (sutype::uvi_t i=0; i < _idToLayer.size(); ++i) {
      
      const suLayer * layer = _idToLayer[i];
      if (layer->base() != baselayer) continue;
      if (layer == baselayer) continue;
      
      layers.push_back (layer);
    }
    
    return layers;
    
  } // end of suLayerManager::get_colors
  
  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------

  //
  void suLayerManager::sanity_check_ ()
  {
    unsigned numissues = 0;

    // lt_wire
    // lt_via
    // lt_metal
    // lt_poly
    // lt_diffusion
    // lt_well

    // check possible conflicts
    for (sutype::uvi_t i=0; i < _layers.size(); ++i) {
      
      const suLayer * layer1 = _layers[i];
      
      if (layer1->is (sutype::lt_wire      | sutype::lt_via)       ||
          layer1->is (sutype::lt_wire      | sutype::lt_diffusion) ||
          layer1->is (sutype::lt_wire      | sutype::lt_well)      ||
          layer1->is (sutype::lt_via       | sutype::lt_metal)     ||
          layer1->is (sutype::lt_via       | sutype::lt_poly)      ||
          layer1->is (sutype::lt_via       | sutype::lt_diffusion) ||
          layer1->is (sutype::lt_via       | sutype::lt_well)      ||
          layer1->is (sutype::lt_metal     | sutype::lt_poly)      ||
          layer1->is (sutype::lt_metal     | sutype::lt_diffusion) ||
          layer1->is (sutype::lt_metal     | sutype::lt_well)      ||
          layer1->is (sutype::lt_poly      | sutype::lt_diffusion) ||
          layer1->is (sutype::lt_poly      | sutype::lt_well)      ||
          layer1->is (sutype::lt_diffusion | sutype::lt_well)) {

        SUISSUE("Bad combination of layer types") << ": Layer " << layer1->name() << ": " << suStatic::layer_type_2_str(layer1->_type) << std::endl;
        ++numissues;
      }
      
      if (layer1->is (sutype::lt_wire) && !layer1->has_pgd()) {
        SUISSUE("Unexpected layer PGD") << ": Layer " << layer1->name() << ": " << suStatic::layer_type_2_str(layer1->_type) << " doesn't have PGD" << std::endl;
        ++numissues;
      }

      for (sutype::uvi_t k=i+1; k < _layers.size(); ++k) {

        const suLayer * layer2 = _layers[k];

        if (layer1->name().compare (layer2->name()) == 0) {
          SUISSUE("Two layers have same name") << ": " << layer1->name() << std::endl;
          ++numissues;
        }
      }
    }

    SUASSERT (numissues == 0, numissues << " issue" << ((numissues == 0) ? "" : "s") << " found. See error messages above.");
    
  } // end of suLayerManager::sanity_check_

  //
  void suLayerManager::post_reading_ ()
  {
    // precompute _electricallyConnectedFlag
    for (sutype::uvi_t i=0; i < _idToLayer.size(); ++i) {

      suLayer * layer1 = (suLayer*)_idToLayer[i];
      SUASSERT (layer1, "id=" << i);
      SUASSERT (layer1->pers_id() == (sutype::id_t)i, "");
      SUASSERT (layer1->_electricallyConnectedFlag.empty(), "");
      
      layer1->_electricallyConnectedFlag.resize (_idToLayer.size(), false);
    }

    // default
    for (sutype::uvi_t i=0; i < _idToLayer.size(); ++i) {

      suLayer * layer1 = (suLayer*)_idToLayer[i];
      sutype::id_t id1 = layer1->pers_id();
      
      layer1->_electricallyConnectedFlag [id1] = true;
      
      for (sutype::uvi_t k=i+1; k < _idToLayer.size(); ++k) {

        suLayer * layer2 = (suLayer*)_idToLayer[k];
        sutype::id_t id2 = layer2->pers_id();

        if (layer1->base() != layer2->base()) continue;
        
        layer1->_electricallyConnectedFlag [id2] = true;
        layer2->_electricallyConnectedFlag [id1] = true;
      }
    }

    // custom
    for (sutype::uvi_t i=0; i < _idToLayer.size(); ++i) {
      
      suLayer * layer1 = (suLayer*)_idToLayer[i];
      sutype::id_t id1 = layer1->pers_id();

      const sutype::layers_tc & electricallyconnected = layer1->_electricallyConnected;
      
      for (const auto & iter2 : electricallyconnected) {
        
        suLayer * layer2 = (suLayer*)iter2;
        sutype::id_t id2 = layer2->pers_id();
        
        layer1->_electricallyConnectedFlag [id2] = true;
        layer2->_electricallyConnectedFlag [id1] = true;
      }
    }
    
    std::sort (_layers.begin(), _layers.end(), suStatic::compare_layers_by_level);

    const suLayer * lowelayer = 0;

    // dummy check
    for (const auto & iter : _layers) {

      const suLayer * layer = iter;
      
      if (!layer->is (sutype::lt_metal)) continue;
      if (!layer->is (sutype::lt_wire)) continue;

      if (lowelayer == 0) {
        lowelayer = layer;
        continue;
      }

      SUASSERT (layers_are_adjacent_metal_layers (lowelayer, layer), "");

      lowelayer = layer;
    }
    
    for (const auto & iter : _layers) {
      
      const suLayer * layer = iter;
      
      SUINFO(1)
        << "Layer"
        << " name=" << layer->name()
        << "; pgd=" << (layer->has_pgd() ? suStatic::grid_orientation_2_str (layer->pgd()) : std::string("undefined"))
        << "; level=" << layer->level()
        << "; type=" << suStatic::layer_type_2_str (layer->_type)
        << "; pers_id=" << layer->pers_id();

      if (!layer->_names.empty()) {
        SUOUT(1)
          << "; othernames=" << suStatic::to_str(layer->_names);
      }
      
      sutype::strings_t ecs;
      const sutype::layers_tc & electricallyconnected = layer->_electricallyConnected;
      
      for (const auto & iter2 : electricallyconnected) {
        const suLayer * layer2 = iter2;
        ecs.push_back (layer2->name());
      }

      if (!ecs.empty()) {
        SUOUT(1)
          << "; ec=" << suStatic::to_str (ecs);
      }
      
      SUOUT(1) << std::endl;

      const sutype::coloredlayers_tc & coloredlayers = layer->_coloredLayers;

      for (const auto & iter2 : coloredlayers) {

        const suColoredLayer * coloredlayer = iter2;

        SUINFO(1)
          <<  "  "
          << "Color"
          << " name=" << coloredlayer->name()
          << "; type=" << coloredlayer->type()
          << "; pers_id=" << coloredlayer->pers_id()
          << std::endl;
      }
    }
    
  } // end of suLayerManager::post_reading_

  //
  void suLayerManager::calculate_lowest_routing_wire_layers_ ()
  {
    SUASSERT (_lowestDRLayer == 0, "");
    SUASSERT (_lowestGRLayer == 0, "");

    SUINFO(1) << "#layers: " << _idToLayer.size() << std::endl;

    for (const auto & iter : _idToLayer) {

      const suLayer * layer = iter;
      SUINFO(1) << layer->to_str() << std::endl;

      if (!layer->is_base()) continue;
      if (!layer->is (sutype::lt_wire)) continue;
      if (!layer->is (sutype::lt_metal)) continue;
      if (layer->electricallyConnected().empty()) continue;
      if (layer->pgd() != sutype::go_ver) continue;
      
      if (_lowestDRLayer == 0 || layer->level() < _lowestDRLayer->level())
        _lowestDRLayer = layer;
    }

    SUASSERT (_lowestDRLayer, "Could not find lowest DR layer");

    _lowestGRLayer = get_base_adjacent_layer (_lowestDRLayer, sutype::lt_wire | sutype::lt_metal, true, 0);
    SUASSERT (_lowestGRLayer, "");
    
    SUASSERT (_lowestDRLayer->has_pgd(), "");
    SUASSERT (_lowestGRLayer->has_pgd(), "");

    SUASSERT (_lowestDRLayer->pgd() == sutype::go_ver, "Expected vertical metal-1 actually");
    SUASSERT (_lowestGRLayer->pgd() == sutype::go_hor, "Expected horizontal metal-2 actually");
      
  } // end of suLayerManager::calculate_lowest_routing_wire_layers_

  //
  void suLayerManager::calculate_highest_routing_wire_layers_ ()
  {
    if (suOptionManager::instance()->get_string_option("upper_layer").empty()) return;
    
    const std::string & upperlayername = suOptionManager::instance()->get_string_option ("upper_layer");
    
    const suLayer * layer = get_base_layer_by_name (upperlayername);
    SUASSERT (layer, "Could not get upper layer: " << upperlayername);
    SUASSERT (layer->is (sutype::lt_metal), "Upper layer is not metal");
    
    _highestDRLayer = layer;
    
  } // end of calculate_highest_routing_wire_layers_

} // end of namespace amsr

// end of suLayerManager.cpp
