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
//! \date   Thu Oct  5 16:52:26 2017

//! \file   suTokenParser.cpp
//! \brief  A collection of methods of the class suTokenParser.

// system includes
#include <assert.h>

// std includes
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

// application includes
#include <suJournal.h>

// other application includes
#include <suToken.h>

// module include
#include <suTokenParser.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  suTokenParser::suTokenParser ()
  {
    init_ ();
    
    _rootToken = new suToken (this, 0);
    
  } // end of suTokenParser::suTokenParser

  //
  suTokenParser::~suTokenParser ()
  {
    if (_rootToken)
      delete _rootToken;
    
  } // end of suTokenParser::~suTokenParser

  // just experimental code
  // static
  std::string suTokenParser::substr (const std::string & str,
                                     int pos,
                                     int count)
  {
    assert (pos >= 0 && pos < (int)str.length());
    assert (count >= 1);
    assert (pos + count <= (int)str.length());
    
    std::string newstr (count, ' ');

    for (int c=0; c < count; ++c) {
      int i = pos + c;
      newstr[c] = str[i];
    }

    return newstr;
    
  } // end of suTokenParser::substr
  
  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  //! 
  bool suTokenParser::read_file (const std::string & filename)
  {
    SUINFO(1) << "Read file: " << filename << std::endl;

    _realpath = filename;
    _numLines = 0;
    
    std::ifstream ifs (filename);
    
    if (!ifs.is_open()) {
      CustomTokenAssert (false, "Can't open file for reading: " << filename);
      return false;
    }
    
    parse_input_stream (ifs, _rootToken);
    
    ifs.close();

    return true;

  } // end of suTokenParser::read_file

  //! 
  bool suTokenParser::read_stream (std::istream& ifs)
  {    
    _realpath = "<stream>";
    
    if (!ifs) {
      std::cout << "Invalid input stream." << std::endl;
      return false;
    }

    parse_input_stream (ifs, _rootToken);
    
    return true;

  } // end of suTokenParser::read_stream

  //! 
  void suTokenParser::parse_input_stream (std::istream & inputstream,
                                          suToken* parenttoken)
  {
    std::string tmpstr ("0123456789012345678901234567890123456789");
    std::vector<std::string> bricks (1024, tmpstr); // not to create this vector many times
    bricks.clear ();
    
    while (true) {
      
      bool finished = false;
      read_a_line_and_parse_it_ (inputstream, parenttoken, finished, bricks);
      if (finished) break;
    }
    
  } // end of suTokenParser::parse_input_stream

  // name=value
  void suTokenParser::parse_parameter_name_and_values (const std::string & str,
                                                       const std::string & delimeter,
                                                       std::vector<std::string> & strs)
  {
    std::string::size_type index = str.find (delimeter);
    
    if (index == std::string::npos) {
      strs.push_back (str);
      return;
    }
    
    strs.push_back (str.substr (0, index));
    strs.push_back (str.substr (index+1));
    
  } // end of suTokenParser::parse_parameter_name_and_values
  
  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------
    
  //! 
  suToken* suTokenParser::read_a_line_and_parse_it_ (std::istream & inputstream,
                                                     suToken* parenttoken,
                                                     bool & finished,
                                                     std::vector<std::string> & bricks)
  {
    bricks.clear ();

    std::string str;
    bool ok = std::getline (inputstream, str);
    ++_numLines;
    if (!ok) {
      finished = true;
      return 0;
    }
    
    int length = str.length();
    
    bool isbrick = false;
    bool isquote = false;
    bool isspace = false;
    bool iscomment = false;
    
    bool storenewbrick = false;
    bool storeequal = false;
    
    int brickstart = 0;
    int brickend = 0;

    for (int i=0; i <= length; ++i) {

      char c = '\0';
      
      if (i < length) {
        c = str[i];
      }
      
      if (c == '\"') {
        if (isquote) {
          storenewbrick = true;
          brickend = i-1;
          isquote = false;
        } else {
          isquote = true;
          brickstart = i+1;
        }
      }

      if (isquote) continue;

      if (std::isspace(c)) {
        if (isbrick) {
          storenewbrick = true;
          isbrick = false;
        }
        isspace = true;
      } else {
        isspace = false;
      }

//       if (c == '=') {
//         if (isbrick) {
//           storenewbrick = true;
//           isbrick = false;
//         }
//         storeequal = true;
//       }

      if (c == '#') {
        if (isbrick) {
          storenewbrick = true;
          isbrick = false;
        }
        iscomment = true;
     }

      if (c == '\0') {
        if (isbrick) {
          storenewbrick = true;
          isbrick = false;
        }
      }

      if (storenewbrick) {
        if (brickend >= brickstart) {
          bricks.push_back (str.substr (brickstart, brickend-brickstart+1));
          //bricks.push_back (suTokenParser::substr (str, brickstart, brickend-brickstart+1));
        }
        storenewbrick = false;
        isbrick = false;
      }
      else {
        if (isbrick) {
          brickend = i;
        }
        else {
          if (!isspace && !storeequal) {
            isbrick = true;
            brickstart = i;
            brickend = i;
          }
        }
      }

      if (storeequal) {
        bricks.push_back (std::string ("="));
        storeequal = false;
      }

      if (iscomment) {
        break;
      }
    }

    if (isquote) {
      CustomTokenAssert (false, "Parse error: unmatched \": " << str);
    }

    unsigned numbricks = bricks.size();
    if (numbricks == 0) return 0;
    
    const std::string & firstbrick = bricks.front();
    if (firstbrick.compare ("}") == 0) {
      if (numbricks > 1) {
        CustomTokenAssert (false, "Parse error: nothing is expected after closing bracked: " << str);
      }
      return parenttoken;
    }

    const std::string & lastbrick = bricks.back();
    if (lastbrick.compare ("{") == 0) {
      --numbricks;
    }

    suToken* token = new suToken (this, _numLines);
    
    token->name (firstbrick);
    if (parenttoken != 0) {
      token->parentToken (parenttoken);
      parenttoken->add_leaf_token (token);
    }

    for (unsigned i=1; i < numbricks; ++i) {

      const std::string & str = bricks[i];
      
      if (str.compare("\"") == 0 ||
          str.compare("=") == 0 ||
          str.compare("{") == 0 ||
          str.compare("}") == 0 ||
          str.compare("#") == 0) {

        CustomTokenAssert (false, "Parse error: " << _realpath << ": service character " << str << " has been recognized as a parameter name: " << str);
      }

      std::vector<std::string> parameterNameAndValues;
      suTokenParser::parse_parameter_name_and_values (str, "=", parameterNameAndValues);

      if (parameterNameAndValues.empty()) {
        CustomTokenAssert (false, "Unexpected: " << str);
      }
      else if (parameterNameAndValues.size() == 1) {
        const std::string & parametername = parameterNameAndValues.front();
        token->add_parameter (parametername, "");
      }
      else if (parameterNameAndValues.size() == 2) {
        const std::string & parametername  = parameterNameAndValues[0];
        const std::string & parametervalue = parameterNameAndValues[1];
        token->add_parameter (parametername, parametervalue);
      }
      else {
        CustomTokenAssert (false, "Unexpected: " << str);
      }
      
//       bool foundValue = false;
      
//       if (i+1 < numbricks) {
//         const std::string & brick = bricks[i+1];
//         if (brick.compare("=") == 0) {
//           ++i;
//           if (i+1 < numbricks) {
//             const std::string & parametervalue = bricks[i+1];
//             token->add_parameter (parametername, parametervalue);
//             foundValue = true;
//             ++i;
//           }
//         }
//       }
      
//       if (!foundValue)
//         token->add_parameter (parametername, "");
    }

    if (lastbrick.compare ("{") == 0) {
      while (true) {
        suToken* leaftoken = read_a_line_and_parse_it_ (inputstream, token, finished, bricks);
        if (finished) break;
        if (leaftoken == token) break;
      }
    }
    
    return token;
    
  } // end of suTokenParser::read_a_line_and_parse_it_                                         

} // end of namespace amsr

// end of suTokenParser.cpp
