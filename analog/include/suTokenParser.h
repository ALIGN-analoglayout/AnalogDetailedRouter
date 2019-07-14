// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Thu Oct  5 16:50:54 2017

//! \file   suTokenParser.h
//! \brief  A header of the class suTokenParser.

#ifndef _suTokenParser_h_
#define _suTokenParser_h_

// system includes
#include <assert.h>

// std includes
#include <string>
#include <vector>
#include <istream>
#include <ostream>

// application includes
#include <suToken.h>

namespace amsr
{
  // classes
  class suToken;

  //! 
  class suTokenParser
  {
  private:

    //! file real path
    std::string _realpath;

    //!
    suToken* _rootToken;
    
    //!
    unsigned _numLines;
    
  public:

    //! default constructor
    suTokenParser ();    
    
    //! copy constructor
    suTokenParser (const suTokenParser& rs)
    {
      CustomTokenAssert (false, "The method is not expected to be called.");

      init_ ();
      copy_ (rs);

    } // end of suTokenParser

    //! assignment operator
    suTokenParser& operator = (const suTokenParser& rs)
    {
      CustomTokenAssert (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

    //! destructor
    virtual ~suTokenParser ();

  private:

    //! init all class members
    inline void init_ ()
    {
      _rootToken = 0;
      _numLines = 0;
      
    } // end of init_
    
    //! copy all class members
    inline void copy_ (const suTokenParser& rs)
    {
      CustomTokenAssert (false, "The method is not expected to be called.");

    } // end of copy_

  public:

    // accessors (setters)
    //


    // accessors (getters)
    //

    //
    inline  const suToken * rootToken () const { return _rootToken; }

    //
    inline  const std::string & realpath () const { return _realpath; }
    
  public:

    // static
    std::string substr (const std::string & str,
                        int index1,
                        int index2);
    
    //! open a file and parse it
    bool read_file (const std::string & filename);

    //! parse input stream 
    bool read_stream (std::istream & inputstream);

    //! parse an input stream
    void parse_input_stream (std::istream & inputstream,
                             suToken* parenttoken);

    //
    static void parse_parameter_name_and_values (const std::string & str,
                                                 const std::string & delimeter,
                                                 std::vector<std::string> & strs);
    
  private:
    
    //! The procedure reads a line from an input stream and treats its as a separate token.
    //! Top-level tokens are stored in _params. Next-level tokens are stored hierarchicaly.
    //! \param inputstream an input stream
    //! \param parenttoken a parent token
    //! \param finished the boolean variable is set to true if end of the stream is reached
    //! \param bricks just a vector of strings; created externally to avoid mulptiple allocations/deallocations
    suToken* read_a_line_and_parse_it_ (std::istream& inputstream,
                                        suToken* parenttoken,
                                        bool& finished,
                                        std::vector<std::string> & bricks);
    
  }; // end of class suTokenParser

} // end of namespace amsr

#endif // _suTokenParser_h_

// end of suTokenParser.h

