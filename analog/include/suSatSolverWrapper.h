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
//! \date   Wed Sep 27 14:30:55 2017

//! \file   suSatSolverWrapper.h
//! \brief  A header of the class suSatSolverWrapper.

#ifndef _suSatSolverWrapper_h_
#define _suSatSolverWrapper_h_

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
  class suClauseManager;
  class suSatSolver;
  
  //! 
  class suSatSolverWrapper
  {
  private:

    static suSatSolverWrapper * _instance;
    
  private:

    // store and check all clauses here before moving them to solvers
    suClauseManager * _clauseManager;
    bool _useClauseManager;

    std::vector<suSatSolver*> _solvers;

    sutype::clause_t _availableSatIndices;
    
    sutype::satindex_t _nextSatIndex;
    
    unsigned _nClauses;
    
    sutype::satindex_t _constant [2];

    sutype::bools_t  _dummyModel; // to avoid compilation warning in suSatSolverWrapper::get_model

    // I put satindices constants here
    // 4 states:
    // sutype::UNDEFINED_SAT_INDEX -- nothing emitted yet for this satindex
    // get_constant(0)
    // get_constant(1)
    sutype::clause_t _emittedConstants;
    
    // a few clauses to use on most critical paths
    sutype::clause_t _hashedClauseSize1;
    sutype::clause_t _hashedClauseSize2;

    //
    std::map<unsigned,unsigned> _statisticOfEmittedClauses;

    //
    bool _unsatisfiable;
    
  private:
    
    //! default constructor
    suSatSolverWrapper ()
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      
    } // end of suSatSolverWrapper

    //! custom constructor
    suSatSolverWrapper (bool emitConstants);

    //! copy constructor
    suSatSolverWrapper (const suSatSolverWrapper & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suSatSolverWrapper

    //! assignment operator
    suSatSolverWrapper & operator = (const suSatSolverWrapper & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suSatSolverWrapper ();
    
  private:
    
    //! init all class members
    inline void init_ ()
    {
      _clauseManager = 0;
      _useClauseManager = false; // true is used for debug mostly
      _unsatisfiable = false;
      _nextSatIndex = 0;
      _nClauses = 0;
      _constant [0] = sutype::UNDEFINED_SAT_INDEX;
      _constant [1] = sutype::UNDEFINED_SAT_INDEX;

      _hashedClauseSize1.resize (1);
      _hashedClauseSize2.resize (2);
      
    } // end of init_
    
    //! copy all class members
    inline void copy_ (const suSatSolverWrapper & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

    // static methods
    //

  public:
    
    //
    static void create_instance (bool emitConstants = true);

    //
    static void delete_instance ();
    
    //
    static inline suSatSolverWrapper * instance ()
    {
      return suSatSolverWrapper::_instance;
      
    } // end of instance
    
  public:

    //
    inline bool use_clause_manager () const { return _useClauseManager; }
    
    //
    sutype::satindex_t get_next_sat_index ();
    
    // \return true if released
    bool return_sat_index (sutype::satindex_t satindex);
    
    //! index must be 0/1
    inline int get_constant (int index)
      const
    {
      //SUASSERT (index == 0 || index == 1, "");
      
      return _constant [index];
      
    } // end of get_constant

    //
    inline bool is_constant (sutype::satindex_t satindex)
      const
    {
      return (is_constant (0, satindex) || is_constant (1, satindex));
      
    } // end of is_constant
    
    // index must be 0/1
    inline bool is_constant (int index,
                             sutype::satindex_t satindex,
                             bool checkEmittedConstants = true)
      const
    {
      //SUASSERT (index == 0 || index == 1, "");
      
      return ((satindex == get_constant (index)) || 
              (satindex == -get_constant (1 - index)) || 
              (checkEmittedConstants ? is_registered_constant (index, satindex) : false));
      
    } // end of is_constant

    //
    bool is_registered_satindex (sutype::satindex_t satindex)
      const;
    
    //
    bool is_registered_constant (int index,
                                 sutype::satindex_t satindex)
      const;

    //!
    sutype::satindex_t emit_constraint (sutype::logic_func_t func,
                                        sutype::bool_t value,
                                        sutype::satindex_t satindex);
    
    //!
    sutype::satindex_t emit_constraint (sutype::logic_func_t func,
                                        sutype::bool_t value,
                                        sutype::satindex_t satindex1,
                                        sutype::satindex_t satindex2);
    
    //!
    sutype::satindex_t emit_constraint (sutype::logic_func_t func,
                                        sutype::bool_t value,
                                        const sutype::clause_t & satindices);
    
    //!
    void emit_ALWAYS_ONE (sutype::satindex_t satindex);

    //
    void emit_ALWAYS_ONE (const sutype::clause_t & clause);

    //!
    void emit_ALWAYS_ZERO (sutype::satindex_t satindex);

    //
    void emit_ALWAYS_ZERO (const sutype::clause_t & clause);
    
    //!
    void emit_OR_ALWAYS_ZERO (const sutype::clause_t & satindices);
    
    //!
    void emit_OR_ALWAYS_ONE (const sutype::clause_t & satindices);

    //!
    void emit_AND_ALWAYS_ZERO (const sutype::clause_t & satindices);

    //
    void emit_AND_ALWAYS_ZERO (sutype::satindex_t satindex1,
                               sutype::satindex_t satindex2);

    //!
    void emit_AND_ALWAYS_ONE (const sutype::clause_t & satindices);

    //!
    sutype::satindex_t emit_NOT_or_return_constant (sutype::satindex_t satindex);
    
    //!
    sutype::satindex_t emit_AND_or_return_constant (const sutype::clause_t & satindices,
                                                    sutype::satindex_t dontCareValue = 0);

    //!
    sutype::satindex_t emit_OR_or_return_constant (const sutype::clause_t & satindices,
                                                   sutype::satindex_t dontCareValue = 0);

    //!
    sutype::satindex_t emit_EQUAL_TO (const sutype::clause_t & clause,
                                      int number);

    //!
    sutype::satindex_t emit_GREATER_or_EQUAL_THEN (const sutype::clause_t & clause,
                                                   int number);

    //!
    sutype::satindex_t emit_LESS_or_EQUAL_THEN (const sutype::clause_t & clause,
                                                int number);
    
    //! don't forget to apply suClauseManager::return_clause()
    sutype::clause_t & build_assumption_max_number_of_outs_used_from_the_clause (const sutype::clause_t & clause,
                                                                                 int minnumber,
                                                                                 int maxnumber);
    
    //! don't forget to apply suClauseManager::return_clause()
    sutype::clause_t & build_assumption_min_number_of_outs_used_from_the_clause (const sutype::clause_t & clause,
                                                                                 int minnumber,
                                                                                 int maxnumber);

    //!
    void optimize_satindices (const sutype::clause_t & clause,
                              const sutype::opt_mode_t optmode,
                              int minbound = -1);
    
    //
    void read_and_emit_dimacs_file (const std::string & filname);

    //
    void fix_sat_indices (const sutype::clause_t & clause);
    
  private:

    // in some conditions, a new emit doesn't change the model at all
    bool can_keep_model_unchanged_ (sutype::satindex_t satindex,
                                    sutype::bool_t targetvalulue)
      const;
    
    // in some conditions, a new emit doesn't change the model at all
    bool can_keep_model_unchanged_ (const sutype::clause_t & clause,
                                    sutype::bool_t targetvalulue)
      const;
    
    // originaloptmode is used to log only
    void minimize_satindices_ (const sutype::clause_t & clause,
                               const sutype::opt_mode_t originaloptmodeToReport,
                               int minbound = -1);
    
    //!
    void emit_AND_ (const sutype::clause_t & satindices,
                    sutype::satindex_t out,
                    sutype::satindex_t doNotCareValue = 0);

    //!
    void emit_OR_ (const sutype::clause_t & satindices,
                   sutype::satindex_t out,
                   sutype::satindex_t doNotCareValue = 0);
    
  private:

    // private wrappers
    //
        
    //
    void create_solvers_ ();
    
    //
    void set_default_solver_params_ ();

    //
    void emit_stored_clauses_to_solvers_ ();
    
    //
    void emit_clause_ (const sutype::clause_t & clause);

    //
    void register_emitted_satindex_ (sutype::satindex_t satindex);

    //
    void register_a_constant_one_ (sutype::satindex_t satindex);
    
  public:
    
    // public wrappers
    //

    //! \return false if failed
    bool simplify ();

    //! \return false if failed
    bool solve_the_problem ();
    
    //! \return false if failed
    //! satindices are to be evaluated as constant-1
    bool solve_the_problem (const sutype::clause_t & assumptions);
    
    //
    bool satindex_is_used_in_clauses (sutype::satindex_t satindex)
      const;

    //
    bool model_is_valid ()
      const;

    //
    void keep_model_unchanged (bool v);
    
    //
    sutype::bool_t get_modeled_value (sutype::satindex_t satindex,
                                      sutype::bool_t desiredvalue = sutype::bool_undefined)
      const;
    
    //
    inline const sutype::bools_t & get_model ()
      const
    {
      return get_model_ ();
      
    } // end of get_model

    //
    void print_statistic_of_emitted_clauses ()
      const;
    
  private:
    
    // private methods
    //

    //
    const sutype::bools_t & get_model_ ()
      const;
    
    //
    void emit_constant_zero_ (sutype::satindex_t satindex);
    
    //
    void emit_constant_one_ (sutype::satindex_t satindex);
        
    //
    void init_constants_ ();

    // don't forget to apply suClauseManager::return_clause()
    sutype::clause_t & build_adder_ (const sutype::clause_t & clause,
                                     const int minCheckSum,
                                     const int maxCheckSum);

    //
    int estimate_the_number_of_mandatory_sat_indices_ (const sutype::clause_t & clause,
                                                       const sutype::opt_mode_t optmode,
                                                       bool fastmode);
    
    // this prcoedure is optmode agnostic
    void detect_optional_satindices_ (sutype::clause_t & assumptions,
                                      sutype::clause_t & undesiredsatindices,
                                      sutype::clause_t & mandatorysatindices);
    
  }; // end of class suSatSolverWrapper

} // end of namespace amsr

#endif // _suSatSolverWrapper_h_

// end of suSatSolverWrapper.h

