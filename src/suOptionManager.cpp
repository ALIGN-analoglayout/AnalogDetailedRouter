// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Thu Oct  5 16:35:30 2017

//! \file   suOptionManager.cpp
//! \brief  A collection of methods of the class suOptionManager.

// std includes
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suCellManager.h>
#include <suGeneratorManager.h>
#include <suGlobalRouter.h>
#include <suLayerManager.h>
#include <suMetalTemplateManager.h>
#include <suPatternManager.h>
#include <suTokenParser.h>

// module include
#include <suOptionManager.h>

namespace amsr
{

  //
  suOptionManager * suOptionManager::_instance = 0;
  std::string suOptionManager::_emptyString = "";
  
  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  //
  suOptionManager::~suOptionManager ()
  {
    if (_tokenParser)
      delete _tokenParser;
    
  } // end of suOptionManager::~suOptionManager
  
  // ------------------------------------------------------------
  // -
  // --- Public static methods
  // -
  // ------------------------------------------------------------
  
  //
  void suOptionManager::delete_instance ()
  {
    if (suOptionManager::_instance)
      delete suOptionManager::_instance;
    
    suOptionManager::_instance = 0;
    
  } // end of suOptionManager::delete_instance

  
  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------
  
  //
  void suOptionManager::read_run_file (const std::string & filename)
  {
    if (filename.empty()) {
      SUASSERT (false, "No run file specified. Nothing to execute.");
      return;
    }
    
    _tokenParser = new suTokenParser ();
    
    bool ok = _tokenParser->read_file (filename);
    SUASSERT (ok, "");
    
    // suOptionManager inherits suTokenOwner
    _token = _tokenParser->rootToken();

    // read 0+ option_file
    if (1) {
      
      sutype::strings_t strs = get_string_options ("option_file");
      //SUASSERT (strs.size() >= 1, "At least one option file must be specified; use \"Option name=option_file value=<filename>\"");
      
      for (const auto & iter : strs) {
        ok = _tokenParser->read_file (iter);
        SUASSERT (ok, "");
      }
    }

    // read 1+ arch_file
    if (1) {
      
      sutype::strings_t strs = get_string_options ("arch_file");
      SUASSERT (strs.size() >= 1, "At least one architecture file must be specified; use \"Option name=arch_file value=<filename>\"");
       
      for (const auto & iter : strs) {
        ok = _tokenParser->read_file (iter);
        SUASSERT (ok, "");
      }
    }
    
  } // end of suOptionManager::read_run_file
  
  //
  void suOptionManager::read_external_files ()
  {
    SUASSERT (_token, "");
    
    // layer files; suLayerManager::instance() creates own token parser
    if (1) {
      
      sutype::tokens_t tokens = _token->get_tokens ("Option", "name", "layer_file");
      SUASSERT (tokens.size() > 0, "At least one layer file must be specified; use \"Option name=layer_file value=<filename>\"");
      
      for (sutype::uvi_t i=0; i < tokens.size(); ++i) {
        suLayerManager::instance()->read_layer_file (tokens[i]->get_string_value ("value"));
      }
    }
    
    // metal templates
    if (1) {

      sutype::tokens_t tokens = _token->get_tokens ("Option", "name", "metal_template_file");
      SUASSERT (tokens.size() > 0, "At least one tech file must be specified; use \"Option name=metal_template_file value=<filename>\"");

      for (sutype::uvi_t i=0; i < tokens.size(); ++i) {
        suMetalTemplateManager::instance()->read_metal_template_file (tokens[i]->get_string_value ("value"));
      }
    }

    // generators
    if (1) {

      sutype::tokens_t tokens = _token->get_tokens ("Option", "name", "generator_file");
      SUASSERT (tokens.size() > 0, "At least one tech file must be specified; use \"Option name=generator_file value=<filename>\"");

      for (sutype::uvi_t i=0; i < tokens.size(); ++i) {
        suGeneratorManager::instance()->read_generator_file (tokens[i]->get_string_value ("value"));
      }
    }

    // patterns
    if (1) {

      sutype::tokens_t tokens = _token->get_tokens ("Option", "name", "pattern_file");
      //SUASSERT (tokens.size() > 0, "At least one tech file must be specified; use \"Option name=pattern_file value=<filename>\"");

      for (sutype::uvi_t i=0; i < tokens.size(); ++i) {
        suPatternManager::instance()->read_pattern_file (tokens[i]->get_string_value ("value"));
      }
    }
    
    // netlist/layout files; temporary token parser is not stored
    if (1) {
      
      sutype::tokens_t tokens = _token->get_tokens ("Option", "name", "input_file");
      SUASSERT (tokens.size() > 0, "At least one netlist file must be specified; use \"Option name=input_file value=<filename>\"");
      
      for (sutype::uvi_t i=0; i < tokens.size(); ++i) {
        suCellManager::instance()->read_input_file (tokens[i]->get_string_value ("value"));
      }
    }
    
  } // end of suOptionManager::read_external_files

  //
  sutype::strings_t suOptionManager::get_string_options (const std::string & optioname)
    const
  {
    sutype::tokens_t tokens = get_option_tokens_ (optioname);

    sutype::strings_t strs;

    for (const auto & iter : tokens) {

      suToken * token = iter;
      strs.push_back (token->get_string_value ("value"));
    }

    return strs;
    
  } // end of suOptionManager::get_string_options

  //
  const std::string & suOptionManager::get_string_option (const std::string & optioname)
    const
  {
    return get_string_option (optioname, suOptionManager::_emptyString);
    
  } // end of suOptionManager::get_string_option
  
  //
  const std::string & suOptionManager::get_string_option (const std::string & optioname,
                                                          const std::string & defaultvalue)
    const
  {
    const suToken * token = get_option_token_ (optioname);
    
    if (token == 0) {
#ifndef _RELEASE_BUILD_
      SUISSUE("Could not get a string option") << ": " << optioname << ". Returned default value: \"" << defaultvalue << "\"" << std::endl;
#endif // _RELEASE_BUILD_
      return defaultvalue;
    }
    
    return token->get_string_value ("value");
    
  } // end of suOptionManager::get_string_option

  //
  bool suOptionManager::get_boolean_option (const std::string & optioname,
                                            const bool defaultvalue)
                                          
    const
  {
    const suToken * token = get_option_token_ (optioname);
    
    if (token == 0) {
#ifndef _RELEASE_BUILD_
      SUISSUE("Could not get a boolean option") << ": " << optioname << ". Returned default value: " << defaultvalue << std::endl;
#endif // _RELEASE_BUILD_
      return defaultvalue;
    }
    
    return token->get_boolean_value ("value");
    
  }  // end of suOptionManager::get_boolean_option

  //
  int suOptionManager::get_integer_option (const std::string & optioname,
                                           const int defaultvalue)
    const
  {
    const suToken * token = get_option_token_ (optioname);
    
    if (token == 0) {
#ifndef _RELEASE_BUILD_
      SUISSUE("Could not get an integer option") << ": " << optioname << ". Returned default value: " << defaultvalue << std::endl;
#endif // _RELEASE_BUILD_
      return defaultvalue;
    }
    
    return token->get_boolean_value ("value");
    
  }  // end of suOptionManager::get_integer_option

  //
  float suOptionManager::get_float_option (const std::string & optioname,
                                           const float defaultvalue)
    const
  {
    const suToken * token = get_option_token_ (optioname);
    
    if (token == 0) {
#ifndef _RELEASE_BUILD_
      SUISSUE("Could not get a float option") << ": " << optioname << ". Returned default value: " << defaultvalue << std::endl;
#endif // _RELEASE_BUILD_
      return defaultvalue;
    }
    
    return token->get_float_value ("value");
    
  }  // end of suOptionManager::get_float_option
  
  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------

  //
  sutype::tokens_t suOptionManager::get_option_tokens_ (const std::string & optioname)
    const
  {
    return _token->get_tokens ("Option", "name", optioname);
    
  } // end of suOptionManager::get_option_tokens_
  
  //
  const suToken * suOptionManager::get_option_token_ (const std::string & optioname)
    const
  {
    sutype::tokens_t tokens = _token->get_tokens ("Option", "name", optioname);
    
    if (tokens.empty()) {

#ifndef _RELEASE_BUILD_
      SUISSUE("Could not find an option in any file")
        << ": \"Option name=" << optioname << "\""
        << std::endl;
#endif // _RELEASE_BUILD_

      return 0;
    }

    if (tokens.size() != 1) {
      SUISSUE("Option has too many instances")
        << ": Found " << tokens.size() << " instances of \"Option name=" << optioname << "\" in all option files."
        << std::endl;
      SUASSERT (false, "");
      return 0;
    }

    return tokens.front();
    
  } // end of suOptionManager::get_option_token_


} // end of namespace amsr

// end of suOptionManager.cpp
