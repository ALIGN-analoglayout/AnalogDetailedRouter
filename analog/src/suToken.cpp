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
//! \date   Thu Oct  5 16:51:32 2017

//! \file   suToken.cpp
//! \brief  A collection of methods of the class suToken.

#include <stdlib.h>

// system includes
#include <assert.h>

// std includes
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

// application includes

// other application includes
#include <suTokenParser.h>

// module include
#include <suToken.h>

namespace amsr
{

  //
  std::string suToken::_dummyValueString ("");
  bool        suToken::_dummyValueBool   (false);
  int         suToken::_dummyValueInt    (0);
  float       suToken::_dummyValueFloat  (0.0);
  
  char suToken::_defaultListDelimeter    = ',';
  char suToken::_defaultListOpenBracket  = '{';
  char suToken::_defaultListCloseBracket = '}';
  
  int suToken::_uniqueId = 0;
 
  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  //!
  suToken::~suToken ()
  {
    for (std::vector<suToken*>::iterator iter=_leafTokens.begin(); iter != _leafTokens.end(); ++iter) {
      delete *iter;
    }
    
  } // end of suToken::~suToken

  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  //! add a leaf token
  void suToken::add_leaf_token (suToken* token)
  {
    _leafTokens.push_back (token);

  } // end of suToken::add_leaf_token

  //! add a new parameter and its value
  void suToken::add_parameter (const std::string& parameter,
                               const std::string& value)
  {
    _parameters.push_back (parameter);
    _values.push_back (value);

  } // end of suToken::add_parameter

  //
  bool suToken::is_defined (const std::string & parametername)
    const
  {
    for (unsigned i = 0; i < _parameters.size(); ++i) {
      if (_parameters[i].compare (parametername) == 0) return true;
    }
    
    return false;
    
  } // end of suToken::is_defined
   
  //
  std::vector<std::string> suToken::get_list_of_strings (const std::string & parametername,
                                                         const char delimeter,
                                                         const char openbracket,
                                                         const char closebracket)
    const
  {
    const std::string & str = get_value_ (parametername);
    
    return suToken::parse_as_list_of_strings (str, delimeter, openbracket, closebracket, this);
    
  } // end of suToken::get_list_of_strings
  
  //
  std::vector<int> suToken::get_list_of_integers (const std::string & parametername,
                                                  const char delimeter,
                                                  const char openbracket,
                                                  const char closebracket)
    const
  {
    std::vector<std::string> strs = get_list_of_strings (parametername,
                                                         delimeter,
                                                         openbracket,
                                                         closebracket);
    std::vector<int> values;

    for (unsigned i=0; i < strs.size(); ++i) {
      values.push_back (suToken::string_2_int (strs[i]));
    }
    
    return values;
    
  } // end of suToken::get_list_of_integers

   //
  std::vector<float> suToken::get_list_of_floats (const std::string & parametername,
                                                  const char delimeter,
                                                  const char openbracket,
                                                  const char closebracket)
    const
  {
    std::vector<std::string> strs = get_list_of_strings (parametername,
                                                         delimeter,
                                                         openbracket,
                                                         closebracket);
    std::vector<float> values;

    for (unsigned i=0; i < strs.size(); ++i) {
      values.push_back (suToken::string_2_float (strs[i]));
    }
    
    return values;
    
  } // end of suToken::get_list_of_floats

  //!
  std::string suToken::hint ()
    const
  {
    std::ostringstream oss;

    oss
      << _tokenParser->realpath()
      << ":"
      << _lineIndex
      << ":"
      << _name;

    return oss.str();
    
  } // end of suToken::hint

  //!
  std::string suToken::to_str ()
    const
  {
    return to_str ("");
    
  } // end of suToken::to_str
  
  //! recursive procedure; resurcion is applied to leaf tokens
  //! \param prefix string prefix; incremented by several spaces duting recursion
  //! \return string representation of the token
  std::string suToken::to_str (const std::string & prefix)
    const
  {
    std::ostringstream oss;
    
    oss << prefix;

    if (!_name.empty()) {
      oss << _name;
    }
    else {
      oss << "__NO_TOKEN_NAME__";
    }

    oss << " id=" << _id;
    
    for (unsigned i=0; i < _parameters.size(); ++i) {
      
      const std::string & parameter = _parameters[i];
      const std::string & value     = _values[i];

      oss << " ";
      oss << parameter;
      
      if (!value.empty()) {
        oss << "=";
        bool needkavichki = false;
        unsigned length = value.length();
        for (unsigned k=0; k<length; ++k) {
          char c = value[k];
          if (std::isspace (c) || c == '#' || c == '=' || c == '{' || c == '}') {
            needkavichki = true;
            break;
          }
        }
        if (needkavichki)
          oss << "\"";
        
        oss << value;
        
        if (needkavichki)
          oss << "\"";
      }
    }

    if (!_leafTokens.empty()) {
      oss << " {";
    }

    oss << std::endl;

    std::string newprefix = prefix + "  ";;

    for (std::vector<suToken*>::const_iterator iter=_leafTokens.begin(); iter != _leafTokens.end(); ++iter) {
      suToken* token = *iter;
      oss << token->to_str (newprefix);
    }

    if (!_leafTokens.empty()) {
      oss << prefix << "}" << std::endl;
    }
    
    return oss.str();
    
  } // end of suToken::to_str

  //! get tokens by name
  std::vector<suToken*> suToken::get_tokens (const std::string & tokenname)
    const
  {
    std::vector<suToken*> tokens;
    
    for (std::vector<suToken*>::const_iterator iter=_leafTokens.begin(); iter != _leafTokens.end(); ++iter) {
      
      suToken* token = *iter;
      
      if (token->name().compare (tokenname) != 0) continue;
      
      tokens.push_back (token);
    }

    return tokens;
    
  } // end of suToken::get_tokens

  //! get tokens by name, and parameter name
  std::vector<suToken*> suToken::get_tokens (const std::string & tokenname,
                                             const std::string & parametername,
                                             const std::string & parametervalue)
    const
  {
    std::vector<suToken*> tokens;

    for (std::vector<suToken*>::const_iterator iter=_leafTokens.begin(); iter != _leafTokens.end(); ++iter) {

      suToken* token = *iter;
      
      if (token->name().compare (tokenname) != 0) continue;
      if (!token->is_defined (parametername)) continue;
      if (token->get_string_value(parametername).compare (parametervalue) != 0) continue;
      
      tokens.push_back (token);
    }

    return tokens;
    
  } // end of suToken::get_tokens
  
  // static
  std::vector<std::string> suToken::parse_as_list_of_strings (const std::string & str,
                                                              const char delimeter,
                                                              const char openbracket,
                                                              const char closebracket,
                                                              const suToken * token)
  {
    std::vector<std::string> tokens;

    const int length = str.length();
    
    bool startednewtoken = false;
    int newtokenstartindex = -1;

    bool foundOpenBracket = false;
    bool foundCloseBracket = false;

    for (int i=0; i <= length; ++i) {

      char c = (i < length) ? str[i] : ' ';
      
      if (i == 0 && c == openbracket) {
        c = ' ';
        foundOpenBracket = true;
      }
      else if (i+1 == length && c == closebracket) {
        c = ' ';
        foundCloseBracket = true;
      }
      
      if (std::isspace(c) || c == delimeter) {
        
        if (startednewtoken) {
          std::string token = str.substr (newtokenstartindex, i-newtokenstartindex);
          tokens.push_back (token);
          startednewtoken = false;
        }
      }
      else {
        if (!startednewtoken) {
          startednewtoken = true;
          newtokenstartindex = i;
        }
      }
    }
    
    CustomTokenAssert (foundOpenBracket == foundCloseBracket, (token ? (token->hint() + " ") : "") << "Bad list: " << str);
    
    //std::cout << str << std::endl;
    //for (unsigned i=0; i < tokens.size(); ++i) {
    //  std::cout << "  " << tokens[i] << std::endl;
    //}
    //assert (false);
    
    return tokens;
    
  } // end of suToken::parse_as_list_of_strings

  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------

  // return parameter name at index
  const std::string & suToken::get_value_ (unsigned index)
    const
  {
    CustomTokenAssert (index >= 0,                 hint() << " token " << name() << "; bad index=" << index);
    CustomTokenAssert (index < _parameters.size(), hint() << " token " << name() << "; bad index=" << index);
    CustomTokenAssert (index < _values.size(),     hint() << " token " << name() << "; bad index=" << index);

    if (index >= 0 && index < _parameters.size()) return _parameters[index];
    
    return suToken::_dummyValueString;
    
  } // end of suToken::get_value_

  const std::string & suToken::get_value_ (const std::string & parametername)
    const
  {
    int index = -1;

    for (unsigned i=0; i < _parameters.size(); ++i) {

      if (_parameters[i].compare (parametername) != 0) continue;
      
      if (index < 0) {
        index = i;
        continue;
      }
      
      CustomTokenAssert (_values[i].compare (_values[index]) == 0, hint() << " token " << name() << " has at least two parameters " << parametername << " with different values: "<< _values[i] << ", " << _values[index]);
    }
    
    if (index >= 0) return _values[index];
    
    CustomTokenAssert (false, hint() << " token " << name() << " has no parameter " << parametername);
    
    return suToken::_dummyValueString;
    
  } // end of suToken::get_value_

  //!
  const std::string & suToken::get_value_ (const std::string & tokenname,
                                           const std::string & parametername)
    const
  {
    int index = -1;

    std::vector<suToken*> tokens = get_tokens (tokenname);
    
    for (unsigned i=0; i < tokens.size(); ++i) {
      
      const suToken * token = tokens[i];
      if (!token->is_defined (parametername)) continue;

      if (index < 0) {
        index = i;
        continue;
      }

      CustomTokenAssert (false, hint() << " token " << name() << " has at least two sub-tokens " << tokenname << " with parameter " << parametername);
    }
    
    if (index >= 0) {
      return tokens[index]->get_value_ (parametername);
    }
    
    CustomTokenAssert (false, hint() << " token " << name() << " has no sub-token " << tokenname << " with parameter " << parametername);
    
    return suToken::_dummyValueString;
    
  } // end of suToken::get_value_ 

  //!
  const std::string & suToken::get_value_ (const std::string & tokenname,
                                           const std::string & parametername1,
                                           const std::string & parametervalue1,
                                           const std::string & parametername2)
    const
  {
    int index = -1;

    std::vector<suToken*> tokens = get_tokens (tokenname);

    for (unsigned i=0; i < tokens.size(); ++i) {

      const suToken * token = tokens[i];
      if (!token->is_defined (parametername1)) continue;
      if (!token->is_defined (parametername2)) continue;
      if (token->get_value_ (parametername1).compare (parametervalue1) != 0) continue;
      
      if (index < 0) {
        index = i;
        continue;
      }
      
      CustomTokenAssert (false, hint() << " token " << name() << " has at least two sub-tokens " << tokenname << " with " << parametername1 << "=" << parametervalue1 << " and parameter " << parametername2);
    }
    
    if (index >= 0) {
      return tokens[index]->get_value_ (parametername2);
    }
    
    CustomTokenAssert (false, hint() << " token " << name() << " has no sub-token " << tokenname << " with " << parametername1 << "=" << parametervalue1 << " and parameter " << parametername2);
    
    return suToken::_dummyValueString;
    
  } // end of suToken::get_string_value  
  
} // end of namespace amsr

// end of suToken.cpp
