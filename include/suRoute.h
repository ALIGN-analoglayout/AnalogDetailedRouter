// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Oct 17 12:17:02 2017

//! \file   suRoute.h
//! \brief  A header of the class suRoute.

#ifndef _suRoute_h_
#define _suRoute_h_

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
  class suLayoutFunc;
  class suWire;
  
  //! 
  class suRoute
  {
  protected:
    
    //
    sutype::wires_t _wires;
    
    //
    suLayoutFunc * _layoutFunc;

  public:

    //! default constructor
    suRoute ();

  private:

    //! copy constructor
    suRoute (const suRoute & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suRoute

    //! assignment operator
    suRoute & operator = (const suRoute & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    ~suRoute ();
    
  private:

    //! init all class members
    inline void init_ ()
    {
      _layoutFunc = 0;
      
    } // end of init_

    //! copy all class members
    inline void copy_ (const suRoute & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

  public:

    //
    inline suLayoutFunc * layoutFunc() const { return _layoutFunc; }
    
    //
    inline const sutype::wires_t & wires () const { return _wires; }

  public:
    
    // \return true if added
    bool add_wire (suWire * wire);
    
    //
    inline void clear_wires () { _wires.clear (); }

    //
    inline bool has_wire (suWire * wire) 
      const
    {
      return (std::find (_wires.begin(), _wires.end(), wire) != _wires.end());
      
    } // end of has_wire

    //
    bool release_useless_wires ();

    // \return true if _layoutFunc was modified
    bool remove_wire_and_update_layout_function (suWire * wire,
                                                 sutype::svi_t index = -1);
    
  private:

  }; // end of class suRoute

} // end of namespace amsr

#endif // _suRoute_h_

// end of suRoute.h

