// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Mon Oct  9 15:24:28 2017

//! \file   suNet.cpp
//! \brief  A collection of methods of the class suNet.

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
#include <suGeneratorInstance.h>
#include <suLayer.h>
#include <suLayerManager.h>
#include <suRuleManager.h>
#include <suSatRouter.h>
#include <suStatic.h>
#include <suWire.h>
#include <suWireManager.h>

// module include
#include <suNet.h>

namespace amsr
{
  
  //
  sutype::id_t suNet::_uniqueId = 0;
  
  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  //
  suNet::~suNet ()
  {
    //SUINFO(1) << "Delete net " << name() << std::endl;

    for (const auto & iter : _wires) {
    
      suWireManager::instance()->release_wire (iter,
                                               false); // verbose=false
    }
    
    _wires.clear();

    for (const auto & iter : _generatorinstances)
      delete iter;
    
  } // end of suNet::~suNet

  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  //
  bool suNet::wire_is_redundant (suWire * wire)
    const
  {
    bool ret = suStatic::wire_is_redundant (wire, _wires);
    if (ret) return ret;

    for (const auto & iter : _generatorinstances) {

      suGeneratorInstance * gi = iter;
      ret = suStatic::wire_is_redundant (wire, gi->wires());
      if (ret) return ret;
    }

    return false;
    
  } // end of suNet::wire_is_redundant

  //
  bool suNet::has_equal_generator_instance (suGeneratorInstance * gi)
      const
  {
    SUASSERT (gi->net() == this, "");

    for (const auto & iter : _generatorinstances) {

      suGeneratorInstance * netgi = iter;
      SUASSERT (netgi->net() == this, "");
      
      if (netgi->generator() != gi->generator()) continue;
      if (netgi->net() != gi->net()) continue;
      if (netgi->point().x() != gi->point().x()) continue;
      if (netgi->point().y() != gi->point().y()) continue;

      return true;
    }

    return false;
    
  } // suNet::has_equal_generator_instance
  
  //
  void suNet::add_wire (suWire * wire)
  {
    SUASSERT (wire->net() == this, "");
    //SUASSERT (wire->is (sutype::wt_preroute), "");
    
    _wires.push_back (wire);

    add_wire_ (wire);
    
  } // end of suNet::add_wire

  //
  void suNet::remove_wire (suWire * wire)
  {
    SUASSERT (wire->net() == this, "");

    const auto & iter = std::find (_wires.begin(), _wires.end(), wire);
    SUASSERT (iter != _wires.end(), "");
    
    sutype::uvi_t index = iter - _wires.begin();
    SUASSERT (_wires[index] == wire, "");
    _wires[index] = _wires.back();
    _wires.pop_back ();
    
    remove_wire_ (wire);
    
  } // end of suNet::remove_wire

  //
  void suNet::add_generator_instance (suGeneratorInstance * gi)
  {
    _generatorinstances.push_back (gi);
    
  } // end of suNet::add_generator_instance

  //
  void suNet::remove_generator_instance (suGeneratorInstance * gi)
  {
    const auto & iter = std::find (_generatorinstances.begin(), _generatorinstances.end(), gi);
    SUASSERT (iter != _generatorinstances.end(), "");

    sutype::uvi_t index = iter - _generatorinstances.begin();
    SUASSERT (index < _generatorinstances.size(), "");
    SUASSERT (_generatorinstances[index] == gi, "");

    _generatorinstances[index] = _generatorinstances.back();
    _generatorinstances.pop_back();
    
  } // end of suNet::remove_generator_instance
  
  //
  bool suNet::wire_is_covered_by_a_preroute (suWire * wire1)
    const
  {
    SUASSERT (wire1, "");
    SUASSERT (!wire1->is (sutype::wt_preroute), "");
    
    const suLayer *  layer1 = wire1->layer();
    sutype::dcoord_t sidel1 = wire1->sidel();
    sutype::dcoord_t sideh1 = wire1->sideh();
    sutype::dcoord_t edgel1 = wire1->edgel();
    sutype::dcoord_t edgeh1 = wire1->edgeh();
    
    SUASSERT (layer1->pers_id() >= 0, "");
    SUASSERT (layer1->pers_id() < (sutype::id_t)_wiresPerLayers.size(), "");
    
    const auto & container0 = _wiresPerLayers [layer1->pers_id()]; // a map of wires sorted by sidel
    
    for (const auto & iter0 : container0) {
      
      sutype::dcoord_t sidel2 = iter0.first;

      // input wire is below; next sidel2 can be only large; wire2 can't overlap wire1
      if (sidel1 < sidel2) break;
      
      const auto & container1 = iter0.second; // a map of wires sorted by sideh

      // now, sidel1 >= sidel2
      // don't need to check every sideh2 if the maximal sideh2 < sideh1, i.e. none of wire2 can overlap wire1
      if (1) {
        SUASSERT (!container1.empty(), "");
        sutype::dcoord_t sideh2 = container1.rbegin()->first; // get the last value
        if (sideh2 < sideh1) continue;
      }
      
      for (const auto & iter1 : container1) {
        
        sutype::dcoord_t sideh2 = iter1.first;
        SUASSERT (sideh2 > sidel2, "");
        
        // input wire is upper; wire2 can't overlap wire1
        if (sideh1 > sideh2) continue;
        
        // input wire must be inside a preroute
        SUASSERT (sidel1 >= sidel2, "");
        SUASSERT (sideh1 <= sideh2, "");
        
        const auto & container2 = iter1.second; // a map of wires sorted by edgel
        
        // now we reached some reasonable iterations
        for (const auto & iter2 : container2) {
          
          sutype::dcoord_t edgel2 = iter2.first;
          const auto & container3 = iter2.second; // a map of wires sorted by edgeh
          
          for (const auto & iter3 : container3) {
            
            sutype::dcoord_t edgeh2 = iter3.first;
            const auto & container4 = iter3.second; // a vector of wires

            SUASSERT (edgeh2 > edgel2, "");
            
            for (const auto & iter4 : container4) {
              
              suWire * wire2 = iter4;
              
              SUASSERT (wire2->net() == this, "");
              SUASSERT (wire2->layer() == layer1, "");
              SUASSERT (wire2->sidel() == sidel2, "sidel2=" << sidel2 << "; wire2=" << wire2->to_str());
              SUASSERT (wire2->sideh() == sideh2, "sideh2=" << sideh2 << "; wire2=" << wire2->to_str());
              SUASSERT (wire2->edgel() == edgel2, "edgel2=" << edgel2 << "; wire2=" << wire2->to_str());
              SUASSERT (wire2->edgeh() == edgeh2, "edgeh2=" << edgeh2 << "; wire2=" << wire2->to_str());
              SUASSERT (wire2->is (sutype::wt_preroute), "");
              SUASSERT (wire2 != wire1, "");
              
              if (sidel1 >= sidel2 && sideh1 <= sideh2 && edgel1 >= edgel2 && edgeh1 <= edgeh2)
                return true;
            }
          }
        }
      }
    }
    
    return false;
    
  } // end of suNet::wire_is_covered_by_a_preroute

  //
  void suNet::merge_wires ()
  {
    sutype::svi_t numwires0 = _wires.size();

    suWireManager::instance()->merge_wires (_wires);

    sutype::svi_t numwires1 = _wires.size();

    if (numwires1 != numwires0) {
      SUINFO(1) << numwires0 << " wires of net " << name() << " merged into " << numwires1 << " wires." << std::endl;
    }

    std::sort (_wires.begin(), _wires.end(), suStatic::compare_wires_by_level);
    
    _wiresPerLayers.clear();

    for (const auto & iter : _wires) {

      suWire * wire = iter;
      add_wire_ (wire);
    }
    
  } // end of suNet::merge_wires

  //
  void suNet::remove_useless_wires ()
  {
    sutype::uvi_t counter = 0;

    for (sutype::uvi_t i=0; i < _wires.size(); ++i) {

      suWire * wire1 = _wires[i];

      bool wireIsUseless = false;

      for (const auto & iter1 : _generatorinstances) {

        const sutype::wires_t & giwires = iter1->wires();

        for (const auto & iter2 : giwires) {

          const suWire * giwire = iter2;
          if (giwire->layer() != wire1->layer()) continue;
          if (!giwire->rect().covers_compeletely (wire1->rect())) continue;
          
          // do not release wire with reserved gid
          if (wire1->gid() != sutype::UNDEFINED_GLOBAL_ID) {
            if (suWireManager::instance()->get_wire_by_reserved_gid (wire1->gid()) != 0) continue;
          }
          
          SUINFO(0)
            << "Wire is useless: " << wire1->to_str()
            << " because of giwire: " << giwire->to_str()
            << "; gi=" << iter1->to_str()
              << std::endl;
          
          wireIsUseless = true;
          break;
        }

        if (wireIsUseless) break;
      }

      if (!wireIsUseless) {

        _wires [counter] = wire1;
        ++counter;
        
        continue;
      }

      suWireManager::instance()->release_wire (wire1);

      remove_wire_ (wire1);
    }

    if (_wires.size() != counter)
      _wires.resize (counter);
    
  } // end of suNet::remove_useless_wires

  //
  bool suNet::wire_is_in_conflict (const suWire * wire1,
                                   bool verbose,
                                   bool avoidUselessChecks)
    const
  {
    const bool na = false;
    const bool np = false;//(wire1->id() == 7901);

    SUINFO(np)
      << "wire_is_in_conflict"
      << "; net=" << name()
      << "; verbose=" << verbose
      << "; avoidUselessChecks=" << avoidUselessChecks
      << "; wire1=" << wire1->to_str()
      << std::endl;

    const suNet * net1 = wire1->net();
    const suLayer * layer1 = wire1->layer();
    
    SUASSERT (net1, "");
    SUASSERT (layer1, "");
    
    if (na) {
      SUASSERT (layer1->has_pgd(), "");
    }

    sutype::dcoord_t sidel1 = wire1->sidel();
    sutype::dcoord_t sideh1 = wire1->sideh();
    sutype::dcoord_t edgel1 = wire1->edgel();
    sutype::dcoord_t edgeh1 = wire1->edgeh();

    // a vector of layers
    // a map of wires sorted by sidel
    // a map of wires sorted by sideh
    // a map of wires sorted by edgel
    // a set of wires sorted by edgeh

    sutype::dcoord_t minete = 0;
    if (suRuleManager::instance()->rule_is_defined (sutype::rt_minete, layer1)) {
      minete = suRuleManager::instance()->get_rule_value (sutype::rt_minete, layer1);
    }
    SUASSERT (minete >= 0, "");

    // now, I check wires only of the same base layer, because I check following conflicts:
    //  shorts between wires of the same layer
    //  minete
    //  trunks
    //  color
    
    const sutype::svi_t index = layer1->base_id();
    SUASSERT (index >= 0 && index < (sutype::svi_t)_wiresPerLayers.size(), layer1->to_str() << "; net=" << name() << "; index=" << index << "; size=" << _wiresPerLayers.size());
    
    const auto & container0 = _wiresPerLayers [index]; // a map of wires sorted by sidel

    // debug
    if (np) {
      for (const auto & iter0 : container0) {
        sutype::dcoord_t sidel2 = iter0.first;
        const auto & container1 = iter0.second; // a map of wires sorted by sideh
        for (const auto & iter1 : container1) {
          sutype::dcoord_t sideh2 = iter1.first;
          const auto & container2 = iter1.second; // a map of wires sorted by edgel
          for (const auto & iter2 : container2) {
            sutype::dcoord_t edgel2 = iter2.first;
            const auto & container3 = iter2.second; // a map of wires sorted by edgeh
            for (const auto & iter3 : container3) {
              sutype::dcoord_t edgeh2 = iter3.first;
              const auto & container4 = iter3.second; // a vector of wires
              for (const auto & iter4 : container4) {
                suWire * wire2 = iter4;
                SUASSERT (wire2->sidel() == sidel2, "Data is corrupted: " << "sidel2=" << sidel2 << "; wire2=" << wire2->to_str());
                SUASSERT (wire2->sideh() == sideh2, "Data is corrupted: " << "sideh2=" << sideh2 << "; wire2=" << wire2->to_str());
                SUASSERT (wire2->edgel() == edgel2, "Data is corrupted: " << "edgel2=" << edgel2 << "; wire2=" << wire2->to_str());
                SUASSERT (wire2->edgeh() == edgeh2, "Data is corrupted: " << "edgeh2=" << edgeh2 << "; wire2=" << wire2->to_str());
              }
            }
          }
        }
      }
    }
    //
    
    for (const auto & iter0 : container0) {

      sutype::dcoord_t sidel2 = iter0.first;

      if (avoidUselessChecks) {
        // no checks after this sidel2
        if (sideh1 < sidel2) {
          SUINFO(np)
            << "avoidUselessChecks 1.0: "
            << "sideh1=" << sideh1
            << "; sidel2=" << sidel2
            << std::endl;
          break;
        }
      }
      
      const auto & container1 = iter0.second; // a map of wires sorted by sideh
      
      // quick check; check the last element sideh2; if it smaller than sidel1 -- we don't need to check every one
      if (avoidUselessChecks) {
        if (1) {
          SUASSERT (!container1.empty(), "");
          sutype::dcoord_t sideh2 = container1.rbegin()->first; // get the last value
          if (sideh2 < sidel1) {
            SUINFO(np)
              << "avoidUselessChecks 2.0: "
              << "sideh2=" << sideh2
              << "; sidel1=" << sidel1
              << std::endl;
            continue;
          }
        }
      }
      
      for (const auto & iter1 : container1) {
          
        sutype::dcoord_t sideh2 = iter1.first;
        
        if (avoidUselessChecks) {
          // no checks before this sideh2
          if (sidel1 > sideh2) {
            SUINFO(np)
              << "avoidUselessChecks 3.0: "
              << "sidel1=" << sidel1
              << "; sideh2=" << sideh2
              << std::endl;
            continue;
          }
        }
          
        const auto & container2 = iter1.second; // a map of wires sorted by edgel

        // now we reached some reasonable iterations
        for (const auto & iter2 : container2) {

          sutype::dcoord_t edgel2 = iter2.first;

          if (avoidUselessChecks) {
            // no checks before that
            if (minete > 0 && edgeh1 + minete <= edgel2) {
              SUINFO(np)
                << "avoidUselessChecks 4.0: "
                << "edgeh1+minete=" << (edgeh1+minete)
                << "; edgel2=" << edgel2
                << std::endl;
              continue;
            }
            if (minete == 0 && edgeh1 + minete < edgel2) {
              SUINFO(np)
                << "avoidUselessChecks 4.1: "
                << "edgeh1+minete=" << (edgeh1+minete)
                << "; edgel2=" << edgel2
                << std::endl;
              continue;
            }
          }
            
          const auto & container3 = iter2.second; // a map of wires sorted by edgeh
            
          for (const auto & iter3 : container3) {

            sutype::dcoord_t edgeh2 = iter3.first;
            const auto & container4 = iter3.second; // a vector of wires

            for (const auto & iter4 : container4) {
              
              suWire * wire2 = iter4;
              if (wire1 == wire2) continue;
                
              if (avoidUselessChecks) {
                // no checks after that
                if (minete > 0 && edgel1 - minete >= edgeh2) {
                  SUINFO(np)
                    << "avoidUselessChecks 5.0: "
                    << "edgel1-minete=" << (edgel1-minete)
                    << "; edgeh2=" << edgeh2
                    << std::endl;
                  break;
                }
                if (minete == 0 && edgel1 - minete > edgeh2) {
                  SUINFO(np)
                    << "avoidUselessChecks 5.1: "
                    << "edgel1-minete=" << (edgel1-minete)
                    << "; edgeh2=" << edgeh2
                    << std::endl;
                  break;
                }
              }
                
              sutype::wire_conflict_t wireconflict = sutype::wc_undefined;
              sutype::bitid_t targetwireconflict = (sutype::bitid_t)sutype::wc_all_types;
                
              bool ret = suSatRouter::wires_are_in_conflict (wire1, wire2, wireconflict, targetwireconflict, verbose);
                
              if (ret) { 
                SUINFO(np)
                  << "Found a conflict"
                  << ": wire1=" << wire1->to_str()
                  << "; wire2=" << wire2->to_str()
                  << std::endl;
                return true;
              }
              
            }
          }
        }
      }
    }

    return false;
    
  } // end of suNet::wire_is_in_conflict

  //
  bool suNet::wire_data_model_is_legal ()
    const
  {
    const sutype::layers_tc & layers = suLayerManager::instance()->idToLayer();

    SUASSERT (_wiresPerLayers.empty() || _wiresPerLayers.size() == layers.size(), "");
    
    for (sutype::uvi_t i=0; i < _wiresPerLayers.size(); ++i) {

      const suLayer * layer1 = layers[i];
      const auto & container0 = _wiresPerLayers [i]; // a map of wires sorted by sidel
      
      for (const auto & iter0 : container0) {
      
        sutype::dcoord_t sidel2 = iter0.first;      
        const auto & container1 = iter0.second; // a map of wires sorted by sideh
        
        SUASSERT (!container1.empty(), "");
     
        for (const auto & iter1 : container1) {
        
          sutype::dcoord_t sideh2 = iter1.first;
          const auto & container2 = iter1.second; // a map of wires sorted by edgel
          
          SUASSERT (!container2.empty(), "");
          SUASSERT (sideh2 > sidel2, "");
        
          for (const auto & iter2 : container2) {
          
            sutype::dcoord_t edgel2 = iter2.first;
            const auto & container3 = iter2.second; // a map of wires sorted by edgeh
            
            SUASSERT (!container3.empty(), "");
          
            for (const auto & iter3 : container3) {
            
              sutype::dcoord_t edgeh2 = iter3.first;
              const auto & container4 = iter3.second; // a vector of wires

              SUASSERT (!container4.empty(), "");
              SUASSERT (edgeh2 > edgel2, "");
            
              for (const auto & iter4 : container4) {
              
                suWire * wire2 = iter4;
              
                SUASSERT (suWireManager::instance()->get_wire_usage (wire2) > 0, wire2->to_str());
                SUASSERT (wire2->net() == this, "net=" << name() << "; wire2=" << wire2->to_str());
                SUASSERT (wire2->layer()->base() == layer1->base(), "layer1=" << layer1->to_str() << "; wire2=" << wire2->to_str());
                SUASSERT (wire2->sidel() == sidel2, "sidel2=" << sidel2 << "; wire2=" << wire2->to_str());
                SUASSERT (wire2->sideh() == sideh2, "sideh2=" << sideh2 << "; wire2=" << wire2->to_str());
                SUASSERT (wire2->edgel() == edgel2, "edgel2=" << edgel2 << "; wire2=" << wire2->to_str());
                SUASSERT (wire2->edgeh() == edgeh2, "edgeh2=" << edgeh2 << "; wire2=" << wire2->to_str());
                SUASSERT (wire2->is (sutype::wt_preroute), "");
              }
            }
          }
        }
      }
    }

    return true;
    
  } // end of suNet::wire_data_model_is_legal
  
  //
  std::string suNet::to_str ()
    const
  {
    std::ostringstream oss;

    oss << name();
    
    oss << ";" << id();

    return oss.str();
    
  } // end of suNet::to_str
  
  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------

  //
  void suNet::register_wire_ (suWire * wire,
                              bool action) // true - add; false - remove
  {
    SUASSERT (wire->net(), "");
    SUASSERT (wire->layer(), "");
    
    if (_wiresPerLayers.empty()) {
      _wiresPerLayers.resize (suLayerManager::instance()->idToLayer().size());
    }
    
    const suLayer * layer = wire->layer();
    SUASSERT (layer->pers_id() >= 0 && layer->pers_id() < (sutype::id_t)_wiresPerLayers.size(), "");
    
    const sutype::dcoord_t sidel = layer->has_pgd() ? wire->sidel() : wire->rect().yl();
    const sutype::dcoord_t sideh = layer->has_pgd() ? wire->sideh() : wire->rect().yh();
    const sutype::dcoord_t edgel = layer->has_pgd() ? wire->edgel() : wire->rect().xl();
    const sutype::dcoord_t edgeh = layer->has_pgd() ? wire->edgeh() : wire->rect().xh();

    const sutype::dcoord_t key1 = sidel;
    const sutype::dcoord_t key2 = sideh;
    const sutype::dcoord_t key3 = edgel;
    const sutype::dcoord_t key4 = edgeh;

    const sutype::id_t key00 = layer->pers_id();
    const sutype::id_t key01 = layer->base_id();

    SUASSERT (key00 >= 0 && key00 < (sutype::id_t)_wiresPerLayers.size(), "");
    SUASSERT (key01 >= 0 && key01 < (sutype::id_t)_wiresPerLayers.size(), "");

    // add a wire
    if (action == true) {

      _wiresPerLayers [key00][key1][key2][key3][key4].push_back (wire);

      if (key01 != key00) {
        _wiresPerLayers [key01][key1][key2][key3][key4].push_back (wire);
      }
    }

    // remove a wire
    else {

      const int minmode = 0;
      const int maxmode = (key01 == key00) ? 0 : 1;
      
      for (int mode = minmode; mode <= maxmode; ++mode) {

        SUASSERT (mode == 0 || mode == 1, "");
        
        sutype::id_t key0 = (mode == 0) ? key00 : key01;
        
        sutype::wires_t & wires = _wiresPerLayers [key0][key1][key2][key3][key4];
        SUASSERT (!wires.empty(), "");
        
        const auto & iter = std::find (wires.begin(), wires.end(), wire);
        SUASSERT (iter != wires.end(), "");
        
        sutype::uvi_t index = iter - wires.begin();
        SUASSERT (index < wires.size(), "");
        SUASSERT (wires[index] == wire, "");
        
        wires[index] = wires.back();
        wires.pop_back ();
        
        if (wires.empty())                                   _wiresPerLayers[key0][key1][key2][key3].erase (key4);
        if (_wiresPerLayers[key0][key1][key2][key3].empty()) _wiresPerLayers[key0][key1][key2]      .erase (key3);
        if (_wiresPerLayers[key0][key1][key2].empty())       _wiresPerLayers[key0][key1]            .erase (key2);
        if (_wiresPerLayers[key0][key1].empty())             _wiresPerLayers[key0]                  .erase (key1);
      }
    }
    
  } // end of suNet::register_wire_
  
} // end of namespace amsr

// end of suNet.cpp
