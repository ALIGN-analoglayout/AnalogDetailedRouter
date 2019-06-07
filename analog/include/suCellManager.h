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
//! \date   Mon Oct  9 12:11:42 2017

//! \file   suCellManager.h
//! \brief  A header of the class suCellManager.

#ifndef _suCellManager_h_
#define _suCellManager_h_

// system includes

// std includes
#include <algorithm>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes

namespace amsr
{
  // classes
  class sucellMaster;

  //! 
  class suCellManager
  {
  private:

    //
    static suCellManager * _instance;

    //
    suCellMaster * _topCellMaster;

    //
    sutype::cellmasters_t _cellMasters;

    //
    sutype::nets_tc _idToNet;

    //
    sutype::nets_tc _netsToRoute;
    
  public:

    //! default constructor
    suCellManager ()
    {
      init_ ();

    } // end of suCellManager

    //! copy constructor
    suCellManager (const suCellManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suCellManager

    //! assignment operator
    suCellManager & operator = (const suCellManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suCellManager ();

  private:

    //! init all class members
    inline void init_ ()
    {
      _topCellMaster = 0;

    } // end of init_

    //! copy all class members
    inline void copy_ (const suCellManager & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

    //
    inline const suCellMaster * topCellMaster () const { return _topCellMaster; }

    //
    inline suCellMaster * topCellMaster () { return _topCellMaster; }

    //
    inline const sutype::nets_tc & netsToRoute () const { return _netsToRoute; }

    //
    inline const suNet * get_net_by_id (sutype::id_t id)
      const
    {
      SUASSERT (id >= 0 && id < (sutype::id_t)_idToNet.size(), "");

      return _idToNet [id];
      
    } // end of get_net_by_id
    
  public:

    // static methods
    //
    
    //
    static void delete_instance ();
    
    //
    static inline suCellManager * instance ()
    {
      if (suCellManager::_instance == 0)
        suCellManager::_instance = new suCellManager ();
      
      return suCellManager::_instance;
      
    } // end of instance

  public:

    // quite simplified procedure just to create a dummy cell to test routing algorithms
    void read_input_file (const std::string & filename);

    //
    void dump_out_file (const std::string & filename,
                        bool lgfstyle = false)
      const;
    
    //
    void register_a_net (const suNet * net);

    //
    void detect_nets_to_route ();

    //
    void fix_conflicts ();

    //
    inline bool route_this_net (const suNet * net)
      const
    {
      return (std::find (_netsToRoute.begin(), _netsToRoute.end(), net) != _netsToRoute.end());
      
    } // end of route_this_net
    
  private:

    //
    void read_wires_ (suCellMaster * cellmaster,
                      const suToken * token)
      const;
    
  }; // end of class suCellManager

} // end of namespace amsr

#endif // _suCellManager_h_

// end of suCellManager.h

