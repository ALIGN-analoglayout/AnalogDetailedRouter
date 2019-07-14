// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Nov 14 12:09:10 2017

//! \file   suPattern.cpp
//! \brief  A collection of methods of the class suPattern.

// std includes
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suClauseBank.h>
#include <suLayer.h>
#include <suLayoutFunc.h>
#include <suLayoutLeaf.h>
#include <suLayoutNode.h>
#include <suPatternInstance.h>
#include <suPatternManager.h>
#include <suRectangle.h>
#include <suStatic.h>
#include <suWire.h>

// module include
#include <suPattern.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  //
  sutype::id_t suPattern::_uniqueId = 0;
  

  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------
  
  //
  suPattern::~suPattern ()
  {
    if (_layoutFunc) {
      _layoutFunc->delete_nodes ();
      suLayoutNode::delete_func (_layoutFunc);
    }
    
  } // end of suPattern::~suPattern
  
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
  void suPattern::create_pattern_instances (const suWire * wire,
                                            sutype::patterninstances_tc & pis)
    const
  {
    //SUINFO(1) << "Create pattern instances: " << wire->to_str() << std::endl;

    sutype::clause_t & depth = suClauseBank::loan_clause ();
    
    create_pattern_instances_ (wire, pis, _layoutFunc, depth);
    
    suClauseBank::return_clause (depth);
    
  } // end of suPattern::create_pattern_instances

  //
  void suPattern::shift_to_0x0 ()
  {
    sutype::wires_t wires;
    collect_wires_ (_layoutFunc, wires);

    SUASSERT (!wires.empty(), "");

    suRectangle bbox;

    if (!wires.empty()) {
      bbox.copy (wires.front()->rect());
    }
    for (sutype::uvi_t i=1; i < wires.size(); ++i) {
      bbox.expand (wires[i]->rect());
    }

    sutype::dcoord_t dx = -bbox.xl();
    sutype::dcoord_t dy = -bbox.yl();
    sutype::ref_t ref = sutype::ref_0;

    apply_transfomation (sutype::tr_t (dx, dy, ref));
    
  } // end of suPattern::shift_to_0x0
  
  //
  void suPattern::apply_transfomation (const sutype::tr_t & tr)
  {
    apply_transfomation_ (_layoutFunc, tr);
    
  } // end of suPattern::apply_transfomation

  //
  void suPattern::dump_as_flat_lgf_file (const std::string & directory)
    const
  {
    const std::string & cellname = name();

    std::string filename = directory + "/" + cellname + ".lgf";

    suStatic::create_parent_dir (filename);

    sutype::wires_t wires;
    collect_wires_ (_layoutFunc, wires);

    std::ofstream out (filename);
    if (!out.is_open()) {
      SUISSUE("Could not open file for writing") << ": " << filename << std::endl;
      SUASSERT (false, "");
    }

    suRectangle bbox;

    if (!wires.empty()) {
      bbox.copy (wires.front()->rect());
    }
    for (sutype::uvi_t i=1; i < wires.size(); ++i) {
      bbox.expand (wires[i]->rect());
    }

    out
      << "Cell"
      << " " << cellname
      << " bbox=" << bbox.to_str(":")
      << std::endl;

    for (const auto & iter : wires) {

      const suWire * wire = iter;
      
      out
        << "Wire"
        << " net=NONET"
        << " layer=" << wire->layer()->name()
        << " rect=" << wire->rect().to_str(":")
        << std::endl;
    }

    out.close ();

    SUINFO(1) << "Written " << filename << std::endl;
    
  } // end of suPattern::dump_as_flat_lgf_file

  //
  void suPattern::collect_unique_layers (std::map<const suLayer *, sutype::patterns_tc> & patternsPerLayer)
    const
  {
    sutype::wires_t wires;
    collect_wires_ (_layoutFunc, wires);

    std::set<const suLayer *> layers;

    for (const auto & iter : wires) {
      suWire * wire = iter;
      layers.insert (wire->layer());
    }

    for (const auto & iter : layers) {
      const suLayer * layer = iter;
      patternsPerLayer[layer].push_back (this);
    }
    
  } // end of suPattern::collect_unique_layers

  //
  void suPattern::dump (std::ostream & oss)
    const
  {
    SUASSERT (!name().empty(), "");
    SUASSERT (!shortname().empty(), "");

    oss
      << "Pattern"
      << " name=" << name()
      << " shortname=" << shortname()
      << " comment=\"" << comment() << "\""
      << " {"
      << std::endl;

    if (_layoutFunc) {
      oss << std::endl;
      dump_pattern_layout_node_ (oss, _layoutFunc, "  ");
    }
    
    oss
      << "}"
      << std::endl;
    
  } // end of suPattern::dump
  

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
  void suPattern::dump_pattern_layout_node_ (std::ostream & oss,
                                             suLayoutNode * node,
                                             const std::string & offset)
    const
  {
    oss << offset;
    
    if (node->is_func()) {
      
      sutype::logic_func_t func = node->to_func()->func();
      sutype::unary_sign_t sign = node->to_func()->sign();

      oss
        << "Node"
        << " type=";
      
      if        (func == sutype::logic_func_and && sign == sutype::unary_sign_just) { oss << "AND";
      } else if (func == sutype::logic_func_and && sign == sutype::unary_sign_not)  { oss << "NAND";
      } else if (func == sutype::logic_func_or  && sign == sutype::unary_sign_just) { oss << "OR";
      } else if (func == sutype::logic_func_or  && sign == sutype::unary_sign_not)  { oss << "NOR";
      } else {
        SUASSERT (false, "Unsupported function and sign for a while");
      }

      oss
        << " {"
        << std::endl;

      const sutype::layoutnodes_t & nodes = node->to_func()->nodes();

      for (const auto & iter : nodes) {
        dump_pattern_layout_node_ (oss, iter, offset + "  ");
      }

      oss
        << offset
        << "}"
        << std::endl;
    }

    else if (node->is_leaf()) {

      sutype::satindex_t satindex = node->to_leaf()->satindex();
      SUASSERT (satindex > 0, "");
      sutype::satindex_t wireindex = satindex;

      const suWire * wire = suPatternManager::instance()->get_wire (wireindex);
      SUASSERT (wire, "");

      oss
        << "Node"
        << " type=leaf"
        << " pr=mbp"
        << " layer=" << wire->layer()->name()
        << " rect=" << wire->rect().xl() << ":" << wire->rect().yl() << ":" << wire->rect().xh() << ":" << wire->rect().yh()
        << " w=" << wire->rect().w()   // this parameter is not used; just to read it in files
        << " h=" << wire->rect().h()   // this parameter is not used; just to read it in files
        << " xc=" << wire->rect().xc() // this parameter is not used; just to read it in files
        << " yc=" << wire->rect().yc() // this parameter is not used; just to read it in files
        << std::endl;
    }

    else {
      SUASSERT (false, "");
    }
    
  } // end of suPattern::dump_pattern_layout_node_

  //
  void suPattern::collect_wires_ (const suLayoutFunc * layoutfunc,
                                  sutype::wires_t & wires)
    const
  {
    SUASSERT (layoutfunc, "");

    const sutype::layoutnodes_t & nodes = layoutfunc->nodes ();

    for (const auto & iter : nodes) {

      const suLayoutNode * layoutnode = iter;
      
      if (layoutnode->is_func()) {
        collect_wires_ (layoutnode->to_func(), wires);
        continue;
      }

      const suLayoutLeaf * layoutleaf = layoutnode->to_leaf();
      SUASSERT (layoutleaf, "");

      sutype::satindex_t satindex = layoutleaf->satindex();
      sutype::satindex_t wireindex = abs (satindex);
      suWire * wire = suPatternManager::instance()->get_wire (wireindex);
      wires.push_back (wire);
    }
    
  } // end of suPattern::collect_wires_

  //
  void suPattern::apply_transfomation_ (suLayoutFunc * layoutfunc,
                                        const sutype::tr_t & tr)
  {
    SUASSERT (layoutfunc, "");

    const sutype::layoutnodes_t & nodes = layoutfunc->nodes ();

    for (sutype::uvi_t i=0; i < nodes.size(); ++i) {

      suLayoutNode * layoutnode = nodes[i];
      
      if (layoutnode->is_func()) {
        apply_transfomation_ (layoutnode->to_func(), tr);
        continue;
      }

      const suLayoutLeaf * layoutleaf = layoutnode->to_leaf();
      SUASSERT (layoutleaf, "");
      sutype::satindex_t satindex0 = layoutleaf->satindex();
      sutype::satindex_t wireindex0 = abs (satindex0);
      suWire * wire0 = suPatternManager::instance()->get_wire (wireindex0);
      sutype::satindex_t wireindex1 = suPatternManager::instance()->create_wire_index_for_transformation (wire0, tr);
      if (satindex0 < 0) wireindex1 = -wireindex1;
      
      layoutfunc->update_node (i, suLayoutNode::create_leaf(wireindex1)->to_node());
    }
    
  } // end of suPattern::apply_transfomation_

  //
  void suPattern::create_pattern_instances_ (const suWire * wire,
                                             sutype::patterninstances_tc & pis,
                                             const suLayoutFunc * layoutfunc,
                                             sutype::clause_t & depth)
    const
  {
    const sutype::layoutnodes_t & nodes = layoutfunc->nodes();
    
    for (sutype::uvi_t i=0; i < nodes.size(); ++i) {
      
      const suLayoutNode * node = nodes[i];

      depth.push_back (i);
      
      if (node->is_func()) create_pattern_instances_ (wire, pis, node->to_func(), depth);
      if (node->is_leaf()) create_pattern_instances_ (wire, pis, node->to_leaf(), depth);

      depth.pop_back ();
    }
    
  } // end of suPattern::create_pattern_instances_

  // depth is used to code position of the leaf inside the master layout function
  void suPattern::create_pattern_instances_ (const suWire * wire,
                                             sutype::patterninstances_tc & pis,
                                             const suLayoutLeaf * layoutleaf,
                                             sutype::clause_t & depth)
    const
  {
    const bool np = false;//(wire->satindex() == 7973 || wire->satindex() == 29951);
    
    SUINFO(np)
      << "Create pattern instances for wire=" << wire->to_str()
      << " and pattern " << name()
      << "; #pis=" << pis.size()
      << std::endl;

    // this wire's index belongs to a reference wire stored in suPatternManager
    sutype::satindex_t wireindex = abs (layoutleaf->satindex());
    
    const sutype::wires_t & refwires = suPatternManager::instance()->wires();
    SUASSERT (wireindex > 0 && wireindex < (sutype::satindex_t)refwires.size(), "");

    const suWire * refwire = refwires [wireindex];
    SUASSERT (refwire, "");
    
    SUINFO(np)
      << "Found refwire=" << refwire->to_str()
      << std::endl;

    if (refwire->layer() != wire->layer()) return;
    if (refwire->rect().w() != wire->rect().w()) return;
    if (refwire->rect().h() != wire->rect().h()) return;
    
    // I support following transformations:
    
    //  ref_0  ref_y
    //  +---   ---+
    //  |         |
    //  +--     --+
    //  |         |
    //  |         |
    //
    //  ref_x  ref_xy
    //  |         |
    //  |         |
    //  +--     --+
    //  |         |
    //  +---   ---+

    for (int i = 0; i < (int)sutype::ref_num_types; ++i) {

      const sutype::ref_t ref = (sutype::ref_t)i;
      
      // calculate dx & dy for given ref
      sutype::dcoord_t dx  = 0;
      sutype::dcoord_t dy  = 0;
      refwire->rect().calculate_transformation (wire->rect(), dx, dy, ref);

      bool found = false;

      const sutype::tr_t tr (dx, dy, ref);
      
      for (const auto & iter : pis) {

        const suPatternInstance * pi = iter;
        
        if (pi->matches (tr)) {
          
          found = true;
          pi->layoutFunc()->add_leaf (wire->satindex(), depth);
          
          if (np) {
            SUINFO(1) << "Found a pattern instance id=" << pi->id() << std::endl;
            pi->print_pattern_instance (std::cout);
          }
          
          break;
        }
      }

      if (found) continue;

      suPatternInstance * pi = new suPatternInstance (this, tr);
      pis.push_back (pi);
      pi->layoutFunc()->add_leaf (wire->satindex(), depth);
      
      if (np) {
        SUINFO(1) << "Created a pattern instance id=" << pi->id() << std::endl;
        pi->print_pattern_instance (std::cout);
      }
    }
    
  } // end of suPattern::create_pattern_instances_


} // end of namespace amsr

// end of suPattern.cpp
