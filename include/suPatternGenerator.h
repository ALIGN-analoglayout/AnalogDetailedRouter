// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Feb  6 12:42:46 2018

//! \file   suPatternGenerator.h
//! \brief  A header of the class suPatternGenerator.

#ifndef _suPatternGenerator_h_
#define _suPatternGenerator_h_

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
  class suGenerator;
  class suGrid;
  class suPattern;

  //! 
  class suPatternGenerator
  {
  private:

    //! static instance
    static suPatternGenerator * _instance;

  private:

    //!
    sutype::grids_t _grids;

    //! unique patterns
    sutype::patterns_t _patterns;
    
  private:

    //! default constructor
    suPatternGenerator ()
    {
      init_ ();

      create_grids_ ();
      
    } // end of suPatternGenerator

  private:

    //! copy constructor
    suPatternGenerator (const suPatternGenerator & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suPatternGenerator

    //! assignment operator
    suPatternGenerator & operator = (const suPatternGenerator & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suPatternGenerator ();

  private:

    //! init all class members
    inline void init_ ()
    {
    } // end of init_

    //! copy all class members
    inline void copy_ (const suPatternGenerator & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

  public:

    // static methods
    //

    //
    static void delete_instance ();
    
    //
    static inline suPatternGenerator * instance ()
    {
      if (suPatternGenerator::_instance == 0) {
        suPatternGenerator::_instance = new suPatternGenerator ();
      }

      return suPatternGenerator::_instance;

    } // end of instance

  public:
    
    //
    void generate_pattern_file (const std::string & filename);
    
  private:

    //
    void create_grids_ ();

    //
    bool find_the_rule_ (const sutype::tokens_t & tokens,
                         const std::string & rulename,
                         sutype::dcoord_t & rulevalue,
                         const suLayer * & rulelayer)
      const;
    //
    bool find_the_rule_ (const sutype::tokens_t & tokens,
                         const std::string & rulename,
                         sutype::dcoord_t & rulevalue,
                         const suLayer * & rulelayer1,
                         const suLayer * & rulelayer2)
      const;

    //
    void dump_patterns_forbid_via_overlap_ ();

    //
    void dump_patterns_V2_size_based_rules_ (const sutype::tokens_t & tokens);
    
    //
    void generate_pattern_file_1273d1_ (const sutype::tokens_t & tokens);

    //
    void generate_pattern_file_1273d5_ (const sutype::tokens_t & tokens);
      
    //
    void generate_pattern_file_1274dX_ (const sutype::tokens_t & tokens);

    //
    void dump_patterns_VX_spacing_ (const sutype::tokens_t & tokens,
                                    const int rulegd,
                                    const bool ruleWorksForTheSameColors,
                                    const std::string & rulename1,
                                    const std::string & rulename2);
    //
    void dump_patterns_VX_spacing_2layers_ (const sutype::tokens_t & tokens,
                                            const int rulegd,
                                            const bool ruleWorksForTheSameColors,
                                            const std::string & rulename1,
                                            const std::string & rulename2);

    //
    void dump_patterns_VX_25_ (const sutype::tokens_t & tokens,
                               const std::string & rulename,
                               const bool ruleWorksForTheSameColors);
    
    // \return true if added
    bool add_pattern_if_unique_ (suPattern * pattern);
       
    //
    bool patterns_are_identical_ (suPattern * pattern0,
                                  suPattern * pattern1,
                                  sutype::trs_t & feasibletrs)
      const;

    //
    bool funcs_are_identical_ (suLayoutFunc * func0,
                               suLayoutFunc * func1,
                               sutype::trs_t & trs)
      const;

    //
    bool leaves_are_identical_ (suLayoutLeaf * leaf0,
                                suLayoutLeaf * leaf1,
                                sutype::trs_t & trs)
      const;
    
    //
    bool nodes_are_identical_ (sutype::layoutnodes_t & nodes0,
                               sutype::layoutnodes_t & nodes1,
                               sutype::trs_t & trs)
      const;

    //
    bool find_chain_of_nodes_ (const sutype::dcoordpairs_t & pairsOfNodes,
                               const sutype::uvi_t startIndex,
                               std::vector<bool> & used0,
                               std::vector<bool> & used1,
                               sutype::uvi_t & chainLength)
      const;
    
    //
    void get_possible_transformations_ (suLayoutNode * node0,
                                        suLayoutNode * node1,
                                        sutype::trs_t & trs)
      const;
    
  }; // end of class suPatternGenerator

} // end of namespace amsr

#endif // _suPatternGenerator_h_

// end of suPatternGenerator.h

