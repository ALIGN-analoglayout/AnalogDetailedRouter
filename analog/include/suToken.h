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
//! \date   Thu Oct  5 16:48:43 2017

//! \file   suToken.h
//! \brief  A header of the class suToken.

#ifndef _suToken_h_
#define _suToken_h_

// system includes
#include <assert.h>

// std includes
#include <string>
#include <vector>
#include <iostream>

// application includes

// custom assert
#define TOKENCOUT if (1) \
    std::cout << "-I- (" << __FILE__ << ":" << __LINE__ << "): "

#define CustomTokenAssert(value,message) if (!(value)) {                     \
    TOKENCOUT << "Program asserted." << std::endl;                      \
    TOKENCOUT << "Assert message: " << message << std::endl;            \
    assert (false);                                                     \
  }

// these defines are used just to save file size and to keep the planet green
#define _CSR_ const std::string &
#define _GSV_ const std::string & get_string_value
#define _GBV_ bool get_boolean_value
#define _GIV_ int get_integer_value
#define _GFV_ float get_float_value
#define _RSS_ return suToken::string_2_string
#define _RSB_ return suToken::string_2_bool
#define _RSI_ return suToken::string_2_int
#define _RSF_ return suToken::string_2_float

namespace amsr
{
  // classes
  class suTokenParser;
  
  //! 
  class suToken
  {
    
  public:

    // used to return something if parameter is missed
    static std::string _dummyValueString;
    static bool        _dummyValueBool;
    static int         _dummyValueInt;
    static float       _dummyValueFloat;

  private:

    //
    static char _defaultListDelimeter;
    static char _defaultListOpenBracket;
    static char _defaultListCloseBracket;
    
    //
    static int _uniqueId;
    
    //! the name of the token
    std::string _name;

    //! a list of parameters; coherent with _values
    std::vector<std::string> _parameters;
    
    //! a list of values; coherent with _parameters
    std::vector<std::string> _values;

    //! parent token
    suToken* _parentToken;

    //! a list of leaf tokens
    std::vector<suToken*> _leafTokens;

    //!
    const suTokenParser * _tokenParser;
    
    //
    unsigned _lineIndex; // a line where this token was created
    
    //
    int _id;
    
  public:

    //! custom constructor
    suToken (const suTokenParser * tokenparser,
             unsigned lineindex)
    {
      init_ ();
      
      _tokenParser = tokenparser;
      _lineIndex = lineindex;
      
    } // end of suToken

  private:
    
    //! default constructor
    suToken ()
    {
      CustomTokenAssert (false, "The method is not expected to be called.");
      
      init_ ();
            
    } // end of suToken
    
    //! copy constructor
    suToken (const suToken& rs)
    {
      CustomTokenAssert (false, "The method is not expected to be called.");
      
      init_ ();
      copy_ (rs);

    } // end of suToken

    //! assignment operator
    suToken& operator = (const suToken& rs)
    {
      CustomTokenAssert (false, "The method is not expected to be called.");

      copy_ (rs);
      
      return *this;
      
    } // end of assignment operator

  public:

    //! destructor
    virtual ~suToken ();

  private:

    //! init all class members
    inline void init_ ()
    {
      _id = suToken::_uniqueId;
      ++suToken::_uniqueId;

      _parentToken = 0;

    } // end of init_

    //! copy all class members
    inline void copy_ (const suToken& rs)
    {
      CustomTokenAssert (false, "The method is not expected to be called.");
      
    } // end of copy_

  public:

    // accessors (setters)
    //

    //! set a name of the token
    inline  void name (const std::string & v) { _name = v; }
    
    //! set parent token
    inline  void parentToken (suToken* v) { _parentToken = v; }
    
    // accessors (getters)
    //

    //! get name of the token
    inline  const std::string & name () const { return _name; }
    
  public:

    //! add a leaf token
    void add_leaf_token (suToken* token);

    //! add a new parameter and its value
    void add_parameter (const std::string & parameter,
                        const std::string & value);

    //!
    bool is_defined (const std::string & parametername)
      const;

  public:

    //
    static inline const std::string & string_2_string (const std::string & str)
    {
      return str;
      
    } // end of string_2_string
    
    //
    static inline bool string_2_bool (const std::string & str)
    {
      assert (!str.empty());

      bool result = suToken::_dummyValueBool;
      
      if        (str.compare ("true")  == 0) { result = true;
      } else if (str.compare ("false") == 0) { result = false;
      } else {
        result = (bool) std::stoi (str);
      }

      return result;
      
    } // end of string_2_bool

    //
    static inline int string_2_int (const std::string & str)
    {
      assert (!str.empty());
      
      return std::stoi (str);
      
    } // end of string_2_int

    //
    static inline float string_2_float (const std::string & str)
    {
      return std::stof (str);
      
    } // end of string_2_int
    
  public:
    
    // get values of parameters
    //

    // t --> tokenname
    // p --> parametername
    // v --> value

    // return value of parameter of index i
    _GSV_ (unsigned p1) const { _RSS_ (get_value_ (p1)); }
    _GBV_ (unsigned p1) const { _RSB_ (get_value_ (p1)); }
    _GIV_ (unsigned p1) const { _RSI_ (get_value_ (p1)); }
    _GFV_ (unsigned p1) const { _RSF_ (get_value_ (p1)); }
        
    // return value of parameter p1
    _GSV_ (_CSR_ p1) const { _RSS_ (get_value_ (p1)); }
    _GBV_ (_CSR_ p1) const { _RSB_ (get_value_ (p1)); }
    _GIV_ (_CSR_ p1) const { _RSI_ (get_value_ (p1)); }
    _GFV_ (_CSR_ p1) const { _RSF_ (get_value_ (p1)); }
    
    // find tokens t1 and return value of parameter p1
    _GSV_ (_CSR_ t1, _CSR_ p1) const { _RSS_ (get_value_ (t1, p1)); }
    _GBV_ (_CSR_ t1, _CSR_ p1) const { _RSB_ (get_value_ (t1, p1)); }
    _GIV_ (_CSR_ t1, _CSR_ p1) const { _RSI_ (get_value_ (t1, p1)); }
    _GFV_ (_CSR_ t1, _CSR_ p1) const { _RSF_ (get_value_ (t1, p1)); }

    // find tokens t1 with p1=v1 and return value of parameter p2
    _GSV_ (_CSR_ t1, _CSR_ p1, _CSR_ v1, _CSR_ p2) const { _RSS_ (get_value_ (t1, p1, v1, p2)); }
    _GBV_ (_CSR_ t1, _CSR_ p1, _CSR_ v1, _CSR_ p2) const { _RSB_ (get_value_ (t1, p1, v1, p2)); }
    _GIV_ (_CSR_ t1, _CSR_ p1, _CSR_ v1, _CSR_ p2) const { _RSI_ (get_value_ (t1, p1, v1, p2)); }
    _GFV_ (_CSR_ t1, _CSR_ p1, _CSR_ v1, _CSR_ p2) const { _RSF_ (get_value_ (t1, p1, v1, p2)); }
    
    // parse lists
    //
    
    //
    std::vector<std::string> get_list_of_strings (const std::string & parametername,
                                                  const char delimeter = suToken::_defaultListDelimeter,
                                                  const char openbracket = suToken::_defaultListOpenBracket,
                                                  const char closebracket = suToken::_defaultListCloseBracket)
      const;
    
    //
    std::vector<int> get_list_of_integers (const std::string & parametername,
                                           const char delimeter = suToken::_defaultListDelimeter,
                                           const char openbracket = suToken::_defaultListOpenBracket,
                                           const char closebracket = suToken::_defaultListCloseBracket)
      const;

    //
    std::vector<float> get_list_of_floats (const std::string & parametername,
                                           const char delimeter = suToken::_defaultListDelimeter,
                                           const char openbracket = suToken::_defaultListOpenBracket,
                                           const char closebracket = suToken::_defaultListCloseBracket)
      const;
    
    // other procedures

    //
    std::string hint ()
      const;
    
    //!
    std::string to_str ()
      const;
    
    //!
    std::string to_str (const std::string & prefix)
      const;
    
    //! get tokens by name
    std::vector<suToken *> get_tokens (const std::string & tokenname)
      const;
    
    //!
    std::vector<suToken *> get_tokens (const std::string & tokenname,
                                       const std::string & parametername,
                                       const std::string & parametervalue)
      const;
    
    //
    static std::vector<std::string> parse_as_list_of_strings (const std::string & str,
                                                               const char delimeter = suToken::_defaultListDelimeter,
                                                               const char openbracket = suToken::_defaultListOpenBracket,
                                                               const char closebracket = suToken::_defaultListCloseBracket,
                                                               const suToken * token = 0); // used just to reporn an error line
    
  private:

    //
    const std::string & get_value_ (unsigned index)
      const;
    
    //!
    const std::string & get_value_ (const std::string & parametername1)
      const;

    //!
    const std::string & get_value_ (const std::string & tokenname,
                                    const std::string & parametername1)
      const;

    //!
    const std::string & get_value_ (const std::string & tokenname,
                                    const std::string & parametername1,
                                    const std::string & parametervalue1,
                                    const std::string & parametername2)
      
      const;
    
  }; // end of class suToken

} // end of namespace amsr

#endif // _suToken_h_

// end of suToken.h

