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
//! \date   Thu Feb  1 12:50:17 2018

//! \file   suRuleManager.cpp
//! \brief  A collection of methods of the class suRuleManager.

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
#include <suLayer.h>
#include <suLayerManager.h>
#include <suOptionManager.h>
#include <suStatic.h>

// module include
#include <suRuleManager.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  suRuleManager * suRuleManager::_instance = 0;


  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  //! default constructor
  suRuleManager::suRuleManager ()
  {
    const sutype::layers_tc & idToLayer = suLayerManager::instance()->idToLayer();
    const sutype::uvi_t numlayers = idToLayer.size();

    if (1) {
      std::vector<bool> tmp0 ((unsigned)sutype::rt_num_types, false);
      std::vector<std::vector<bool> > tmp1 (numlayers, tmp0);
      _ruleIsDefined.resize (numlayers, tmp1);
    }

    SUASSERT (_ruleIsDefined.size() == numlayers, "");
    SUASSERT (_ruleIsDefined.front().size() == numlayers, "");
    SUASSERT (_ruleIsDefined.front().front().size() == (sutype::uvi_t)sutype::rt_num_types, "");
    
    if (1) {
      sutype::dcoords_t tmp0 ((unsigned)sutype::rt_num_types, 0);
      std::vector<sutype::dcoords_t> tmp1 (numlayers, tmp0);
      _ruleValue.resize (numlayers, tmp1);
    }

    SUASSERT (_ruleValue.size() == numlayers, "");
    SUASSERT (_ruleValue.front().size() == numlayers, "");
    SUASSERT (_ruleValue.front().front().size() == (sutype::uvi_t)sutype::rt_num_types, "");
    
    sutype::strings_t layertokens;
    layertokens.push_back (std::string ("layer"));
    layertokens.push_back (std::string ("layer1"));
    layertokens.push_back (std::string ("layer2"));
    
    for (int i = 0; i < (int)sutype::rt_num_types; ++i) {
      
      sutype::rule_type_t rt = (sutype::rule_type_t)i;
      const std::string & rulename = suStatic::rule_type_2_str (rt);
      
      sutype::tokens_t tokens = suOptionManager::instance()->token()->get_tokens ("Rule", "type", rulename);
      
      for (const auto & iter1 : tokens) {
        
        suToken * token = iter1;

        SUASSERT (token->is_defined("value"), "");
        sutype::dcoord_t value = token->get_integer_value ("value");

        sutype::layers_tc layers;

        for (const auto & iter : layertokens) {

          const std::string & layertoken = iter;
          if (!token->is_defined (layertoken)) continue;

          const std::string & layername = token->get_string_value (layertoken);
          const suLayer * layer = suLayerManager::instance()->get_base_layer_by_name (layername);
          
          SUASSERT (layer, "Could not get layer " << layername);
          SUASSERT (value > 0, "");
          SUASSERT (layer->is_base(), "");

          sutype::id_t baselayerid = layer->base_id();
          SUASSERT (baselayerid >= 0, "");
          SUASSERT (baselayerid < (sutype::id_t)_ruleIsDefined.size(), "");
          SUASSERT (baselayerid < (sutype::id_t)_ruleIsDefined.front().size(), "");

          layers.push_back (layer);
        }

        SUASSERT (layers.size() >= 1 && layers.size() <= 2, "");
        
        if (layers.size() == 1) {
          layers.push_back (layers.front());
        }

        SUASSERT (layers.size() == 2, "");

        sutype::id_t baselayerid0 = layers[0]->base_id();
        sutype::id_t baselayerid1 = layers[1]->base_id();
        
        SUASSERT (!_ruleIsDefined[baselayerid0][baselayerid1][i], "Rule " << rulename << " is defined at least twice for layer1=" << layers[0]->name() << "; layer2=" << layers[1]->name());
        SUASSERT (!_ruleIsDefined[baselayerid1][baselayerid0][i], "Rule " << rulename << " is defined at least twice for layer1=" << layers[0]->name() << "; layer2=" << layers[1]->name());
        
        _ruleIsDefined [baselayerid0][baselayerid1][i] = true;
        _ruleIsDefined [baselayerid1][baselayerid0][i] = true;
        
        _ruleValue     [baselayerid0][baselayerid1][i] = value;
        _ruleValue     [baselayerid1][baselayerid0][i] = value;
      }
    }

    for (sutype::uvi_t i=0; i < _ruleIsDefined.size(); ++i) {

      for (sutype::uvi_t k=0; k < _ruleIsDefined[i].size(); ++k) {

        for (sutype::uvi_t j=0; j < _ruleIsDefined[i][k].size(); ++j) {
          
          if (!_ruleIsDefined[i][k][j]) continue;
          
          const suLayer * layer1 = idToLayer[i];
          const suLayer * layer2 = idToLayer[k];

          SUASSERT (j >= 0 && j < (sutype::uvi_t)sutype::rt_num_types, "j=" << j);
          
          sutype::rule_type_t rt = (sutype::rule_type_t)j;
          sutype::dcoord_t value = _ruleValue[i][k][j];

          if (layer1->base_id() > layer2->base_id()) continue;
          
          SUINFO(1)
            << "Rule"
            << " type=" << suStatic::rule_type_2_str (rt)
            << " value=" << value;
                    
          if (layer1 == layer2) {
            SUOUT(1)
              << " layer=" << layer1->name();
          }
          else {
            SUOUT(1)
              << " layer1=" << layer1->name()
              << " layer2=" << layer2->name();
          }

          SUOUT(1) << std::endl;
        }
      }
    }
    
  } // end of suRuleManager::suRuleManager


  // ------------------------------------------------------------
  // -
  // --- Public static methods
  // -
  // ------------------------------------------------------------

  // static
  void suRuleManager::delete_instance ()
  {
    if (suRuleManager::_instance)
      delete suRuleManager::_instance;
  
    suRuleManager::_instance = 0;
    
  } // end of suRuleManager::delete_instance


  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  //
  bool suRuleManager::rule_is_defined (sutype::rule_type_t rt,
                                       const suLayer * layer1)
    const
  {
    return rule_is_defined (rt,
                            layer1,
                            layer1);
    
  } // end of suRuleManager::rule_is_defined

  //
  bool suRuleManager::rule_is_defined (sutype::rule_type_t rt,
                                       const suLayer * layer1,
                                       const suLayer * layer2)
    const
  {
    SUASSERT (layer1, "");
    SUASSERT (layer2, "");

    sutype::id_t baselayerid1 = layer1->base_id();
    sutype::id_t baselayerid2 = layer2->base_id();

    SUASSERT (baselayerid1 >= 0 && baselayerid1 < (sutype::id_t)_ruleIsDefined.size(), "");
    SUASSERT (baselayerid2 >= 0 && baselayerid2 < (sutype::id_t)_ruleIsDefined.size(), "");

    bool value12 = _ruleIsDefined [baselayerid1][baselayerid2][(int)rt];
    bool value21 = _ruleIsDefined [baselayerid2][baselayerid1][(int)rt];
    
    SUASSERT (value12 == value21, "");
    
    return value12;
    
  } // end of suRuleManager::rule_is_defined

  //
  sutype::dcoord_t suRuleManager::get_rule_value (sutype::rule_type_t rt,
                                                  const suLayer * layer1)
    const
  {
    return suRuleManager::get_rule_value (rt,
                                          layer1,
                                          layer1);
    
  } // end of suRuleManager::get_rule_value
  
  //
  sutype::dcoord_t suRuleManager::get_rule_value (sutype::rule_type_t rt,
                                                  const suLayer * layer1,
                                                  const suLayer * layer2)
    const
  {
    SUASSERT (layer1, "");
    SUASSERT (layer2, "");

    SUASSERT (rule_is_defined (rt, layer1, layer2), "");

    sutype::id_t baselayerid1 = layer1->base_id();
    sutype::id_t baselayerid2 = layer2->base_id();

    SUASSERT (baselayerid1 >= 0 && baselayerid1 < (sutype::id_t)_ruleValue.size(), "");
    SUASSERT (baselayerid2 >= 0 && baselayerid2 < (sutype::id_t)_ruleValue.size(), "");

    sutype::dcoord_t value12 = _ruleValue [baselayerid1][baselayerid2][(int)rt];
    sutype::dcoord_t value21 = _ruleValue [baselayerid2][baselayerid1][(int)rt];
    
    SUASSERT (value12 == value21, "");
    
    return value12;
    
  } // end of suRuleManager::get_rule_value

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

// end of suRuleManager.cpp
