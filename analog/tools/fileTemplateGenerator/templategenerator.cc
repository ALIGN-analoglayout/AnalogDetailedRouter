#include <assert.h>
#include <stdlib.h>
#include <ctime>

#include <string>
#include <iostream>

void dump_h ();
void dump_cpp ();

std::string name = "";
std::string nameSpace = "amsr";
std::string project =  "Analog/Mixed Signal Router (prototype); AMSR";
std::string version = "0.00";
std::string author = "Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com";
bool _singleton = false;

int main (int argc, char* argv[])
{
  if (argc <= 3) {
    std::cout << "Usage: " << argv[0] << " <h|header|s|src> <class name> <s|singleton|g|general>" << std::endl;
    return 0;
  }

  time_t thetime;
  time (&thetime);
  std::string date = std::string (ctime (&thetime));
  
  unsigned i = 1;

  std::string fileType  = std::string (argv[i]); ++i;  
  name                  = std::string (argv[i]); ++i;
  std::string singleton = std::string (argv[i]); ++i;

  if (singleton.compare ("s") == 0 || singleton.compare ("singleton") == 0)
    _singleton = true;
  
  std::cout << "//" << std::endl;
  std::cout << "//" << std::endl;
  std::cout << "//        INTEL CONFIDENTIAL - INTERNAL USE ONLY" << std::endl;
  std::cout << "//" << std::endl;
  std::cout << "//         Copyright by Intel Corporation, 2017" << std::endl;
  std::cout << "//                 All rights reserved." << std::endl;
  std::cout << "//         Copyright does not imply publication." << std::endl;
  std::cout << "//" << std::endl;
  std::cout << "//" << std::endl;
  std::cout << "//! \\since  " << project << " " << version << std::endl;
  std::cout << "//! \\author " << author << std::endl;
  std::cout << "//! \\date   " << date << std::endl;
  
  if (fileType.compare("h") == 0 || fileType.compare("header") == 0) {
    dump_h ();
  }
  else if (fileType.compare("s") == 0 || fileType.compare("src") == 0) {
    dump_cpp ();
  }
  else {
    assert (false);
  }

  return 1;

} // end of main

void dump_h () {

  std::cout << "//! \\file   " << name << ".h" << std::endl;
  std::cout << "//! \\brief  A header of the class " << name << "." << std::endl;
  std::cout << std::endl;
  std::cout << "#ifndef _" << name << "_h_" << std::endl;
  std::cout << "#define _" << name << "_h_" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "// system includes" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "// std includes" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "// application includes" << std::endl;
  std::cout << "#include <suAssert.h>" << std::endl;
  std::cout << "#include <suDefine.h>" << std::endl;
  std::cout << "#include <suJournal.h>" << std::endl;
  std::cout << "#include <suTypedefs.h>" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "// other application includes" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "namespace " << nameSpace << std::endl;
  std::cout << "{" << std::endl;
  std::cout << "  // classes" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "  //! " << std::endl;
  std::cout << "  class " << name << std::endl;
  std::cout << "  {" << std::endl;
  
  if (_singleton) {
    std::cout << "  private:" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "    //! static instance" << std::endl;
    std::cout << "    static " << name << " * _instance;" << std::endl;
    std::cout << "" << std::endl;
  }
  
  std::cout << "  private:" << std::endl;
  std::cout << "" << std::endl;

  if (_singleton) {
    std::cout << "  private:" << std::endl;
  }
  else {
    std::cout << "  public:" << std::endl;
  }
  std::cout << "" << std::endl;
  std::cout << "    //! default constructor" << std::endl;
  std::cout << "    " << name << " ()" << std::endl;
  std::cout << "    {" << std::endl;
  std::cout << "      init_ ();" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "    } // end of " << name << std::endl;
  std::cout << "" << std::endl;
  std::cout << "  private:" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "    //! copy constructor" << std::endl;
  std::cout << "    " << name << " (const " << name << " & rs)" << std::endl;
  std::cout << "    {" << std::endl;
  std::cout << "      SUASSERT (false, \"The method is not expected to be called.\");" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "      init_ ();" << std::endl;
  std::cout << "      copy_ (rs);" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "    } // end of " << name << std::endl;
  std::cout << "" << std::endl;
  std::cout << "    //! assignment operator" << std::endl;
  std::cout << "    " << name << " & operator = (const " << name << " & rs)" << std::endl;
  std::cout << "    {" << std::endl;
  std::cout << "      SUASSERT (false, \"The method is not expected to be called.\");" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "      copy_ (rs);" << std::endl;
  std::cout << "      " << std::endl;
  std::cout << "      return *this;" << std::endl;
  std::cout << "      " << std::endl;
  std::cout << "    } // end of assignment operator" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "  public:" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "    //! destructor" << std::endl;
  std::cout << "    virtual ~" << name << " ()" << std::endl;
  std::cout << "    {" << std::endl;
  std::cout << "    } // end of ~" << name << std::endl;
  std::cout << "" << std::endl;
  std::cout << "  private:" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "    //! init all class members" << std::endl;
  std::cout << "    inline void init_ ()" << std::endl;
  std::cout << "    {" << std::endl;
  std::cout << "    } // end of init_" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "    //! copy all class members" << std::endl;
  std::cout << "    inline void copy_ (const " << name << " & rs)" << std::endl;
  std::cout << "    {" << std::endl;
  std::cout << "      SUASSERT (false, \"The method is not expected to be called.\");" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "    } // end of copy_" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "  public:" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "    // accessors (setters)" << std::endl;
  std::cout << "    //" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "    // accessors (getters)" << std::endl;
  std::cout << "    //" << std::endl;
  std::cout << "" << std::endl;
  if (_singleton) {
    std::cout << "  public:" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "    // static methods" << std::endl;
    std::cout << "    //" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "    //" << std::endl;
    std::cout << "    static void delete_instance ();" << std::endl;
    std::cout << "    " << std::endl;
    std::cout << "    //" << std::endl;
    std::cout << "    static inline " << name << " * instance ()" << std::endl;
    std::cout << "    {" << std::endl;
    std::cout << "      if (" << name << "::_instance == 0)" << std::endl;
    std::cout << "        " << name << "::_instance = new " << name << " ();" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "      return " << name << "::_instance;" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "    } // end of instance" << std::endl;
    std::cout << "" << std::endl;
  }
  std::cout << "  public:" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "  private:" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "  }; // end of class " << name << std::endl;
  std::cout << "" << std::endl;
  std::cout << "} // end of namespace " << nameSpace << std::endl;
  std::cout << "" << std::endl;
  std::cout << "#endif // _" << name << "_h_" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "// end of " << name << ".h" << std::endl;
  std::cout << "" << std::endl;
  
} // end of dump_header

void dump_cpp () {
  
  std::cout << "//! \\file   " << name << ".cpp" << std::endl;
  std::cout << "//! \\brief  A collection of methods of the class " << name << "." << std::endl;
  std::cout << std::endl;
  std::cout << "// std includes" << std::endl;
  std::cout << "#include <algorithm>" << std::endl;
  std::cout << "#include <iostream>" << std::endl;
  std::cout << "#include <string>" << std::endl;
  std::cout << "#include <vector>" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "// application includes" << std::endl;
  std::cout << "#include <suAssert.h>" << std::endl;
  std::cout << "#include <suDefine.h>" << std::endl;
  std::cout << "#include <suJournal.h>" << std::endl;
  std::cout << "#include <suTypedefs.h>" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "// other application includes" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "// module include" << std::endl;
  //std::cout << "#include <" << project << "/" << name << ".h>" << std::endl;
  std::cout << "#include <" << name << ".h>" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "namespace " << nameSpace << std::endl;
  std::cout << "{" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "  // ------------------------------------------------------------" << std::endl;
  std::cout << "  // -" << std::endl;
  std::cout << "  // --- Static variables" << std::endl;
  std::cout << "  // -" << std::endl;
  std::cout << "  // ------------------------------------------------------------" << std::endl;
  if (_singleton) {
    std::cout << "" << std::endl;
    std::cout << "  " << name << " * " << name << "::_instance = 0;" << std::endl;
  }
  std::cout << "" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "  // ------------------------------------------------------------" << std::endl;
  std::cout << "  // -" << std::endl;
  std::cout << "  // --- Special methods" << std::endl;
  std::cout << "  // -" << std::endl;
  std::cout << "  // ------------------------------------------------------------" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "  // ------------------------------------------------------------" << std::endl;
  std::cout << "  // -" << std::endl;
  std::cout << "  // --- Public static methods" << std::endl;
  std::cout << "  // -" << std::endl;
  std::cout << "  // ------------------------------------------------------------" << std::endl;
  if (_singleton) {
    std::cout << "" << std::endl;
    std::cout << "  // static" << std::endl;
    std::cout << "  void " << name << "::delete_instance ()" << std::endl;
    std::cout << "  {" << std::endl;
    std::cout << "    if (" << name << "::_instance)" << std::endl;
    std::cout << "      delete " << name << "::_instance;" << std::endl;
    std::cout << "  " << std::endl;
    std::cout << "    " << name << "::_instance = 0;" << std::endl;
    std::cout << "    " << std::endl;
    std::cout << "  } // end of " << name << "::delete_instance" << std::endl;
  }
  std::cout << "" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "  // ------------------------------------------------------------" << std::endl;
  std::cout << "  // -" << std::endl;
  std::cout << "  // --- Public methods" << std::endl;
  std::cout << "  // -" << std::endl;
  std::cout << "  // ------------------------------------------------------------" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "  // ------------------------------------------------------------" << std::endl;
  std::cout << "  // -" << std::endl;
  std::cout << "  // --- Private static methods" << std::endl;
  std::cout << "  // -" << std::endl;
  std::cout << "  // ------------------------------------------------------------" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "  // ------------------------------------------------------------" << std::endl;
  std::cout << "  // -" << std::endl;
  std::cout << "  // --- Private methods" << std::endl;
  std::cout << "  // -" << std::endl;
  std::cout << "  // ------------------------------------------------------------" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "} // end of namespace " << nameSpace << std::endl;
  std::cout << "" << std::endl;
  std::cout << "// end of " << name << ".cpp" << std::endl;
  
} // end of dump_source

