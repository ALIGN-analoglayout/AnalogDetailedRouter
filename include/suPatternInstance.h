// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Nov 14 16:28:21 2017

//! \file   suPatternInstance.h
//! \brief  A header of the class suPatternInstance.

#ifndef _suPatternInstance_h_
#define _suPatternInstance_h_

// system includes

// std includes
#include <ostream>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suPoint.h>

namespace amsr
{
  // classes
  class suLayoutFunc;
  class suPattern;

  //! 
  class suPatternInstance
  {
  private:

    //
    static sutype::id_t _uniqueId;

    //
    sutype::tr_t _tr;
    
    //
    sutype::id_t _id;
    
    //
    const suPattern * _pattern;

    //
    suLayoutFunc * _layoutFunc;

  public:

    //! custom constructor
    suPatternInstance (const suPattern * pattern,
                       const sutype::tr_t & tr);

  private:

    //! default constructor
    suPatternInstance ()
    {
      init_ ();

    } // end of suPatternInstance

    //! copy constructor
    suPatternInstance (const suPatternInstance & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suPatternInstance

    //! assignment operator
    suPatternInstance & operator = (const suPatternInstance & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suPatternInstance ();

  private:

    //! init all class members
    inline void init_ ()
    {
      _id = suPatternInstance::_uniqueId;
      ++suPatternInstance::_uniqueId;

      _tr = sutype::tr_t (0, 0, sutype::ref_0);
      _pattern = 0;
      _layoutFunc = 0;
      
    } // end of init_
    
    //! copy all class members
    inline void copy_ (const suPatternInstance & rs)
    {
      SUASSERT (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

    //
    inline sutype::id_t id () const { return _id; }

    //
    inline suLayoutFunc * layoutFunc() const { return _layoutFunc; }

    //
    inline const suPattern * pattern () const { return _pattern; }
    
  public:

    //
    std::string to_str ()
      const;

    // potentially different combinations of (x,y,ref,{rot},{scalex},{scaley}) may lead to the same transformation
    // now, I support only (dy,dy,ref)
    inline bool matches (const sutype::tr_t & tr)
      const
    {
      return (tr == _tr);
      
    } // end of matches
    
    //
    void print_pattern_instance (std::ostream & oss)
      const;

    //
    suPatternInstance * create_clone ()
      const;
    
  private:
    
  }; // end of class suPatternInstance

} // end of namespace amsr

#endif // _suPatternInstance_h_

// end of suPatternInstance.h

