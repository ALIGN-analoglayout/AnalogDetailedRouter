// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Mon Oct  9 12:11:53 2017

//! \file   suCellMaster.h
//! \brief  A header of the class suCellMaster.

#ifndef _suCellMaster_h_
#define _suCellMaster_h_

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
  class suCellManager;
  class suNet;
  
  //! 
  class suCellMaster
  {
    friend class suCellManager;

  private:
    
    //
    std::string _name;

    //
    sutype::nets_t _nets;

    //
    suRectangle _bbox;
    
  public:

    //! default constructor
    suCellMaster (const std::string & name)
    {
      init_ ();

      _name = name;
      
    } // end of suCellMaster
    
  private:

    //! default constructor
    suCellMaster ()
    {
      init_ ();

    } // end of suCellMaster

    //! copy constructor
    suCellMaster (const suCellMaster & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suCellMaster

    //! assignment operator
    suCellMaster & operator = (const suCellMaster & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:
    
    //! destructor
    virtual ~suCellMaster ();

  private:

    //! init all class members
    inline void init_ ()
    {
    } // end of init_

    //! copy all class members
    inline void copy_ (const suCellMaster & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

    //
    inline const suRectangle & bbox () const { return _bbox; }
    
    //
    inline const std::string & name () const { return _name; }
    
    //
    inline const sutype::nets_t & nets () const { return _nets; }
    
  public:
    
    //
    suNet * get_net_by_name (const std::string & netname) const;
    
    //
    suNet * create_net (const std::string & netname);

    //
    suNet * create_net_if_needed (const std::string & netname);
    
  }; // end of class suCellMaster

} // end of namespace amsr

#endif // _suCellMaster_h_

// end of suCellMaster.h

