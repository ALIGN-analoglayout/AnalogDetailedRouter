// see LICENSE
//! \author Ryzhenko Nikolai, 10984646, nikolai.v.ryzhenko@intel.com
//! \date   Tue Aug 21 14:52:55 2018

//! \file   suErrorManager.cpp
//! \brief  A collection of methods of the class suErrorManager.

// std includes
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// application includes
#include <suAssert.h>
#include <suDefine.h>
#include <suJournal.h>
#include <suTypedefs.h>

// other application includes
#include <suError.h>
#include <suRectangle.h>
#include <suStatic.h>

// module include
#include <suErrorManager.h>

namespace amsr
{

  // ------------------------------------------------------------
  // -
  // --- Static variables
  // -
  // ------------------------------------------------------------

  suErrorManager * suErrorManager::_instance = 0;


  // ------------------------------------------------------------
  // -
  // --- Special methods
  // -
  // ------------------------------------------------------------

  //
  suErrorManager::~suErrorManager ()
  {
    for (const auto & iter : _errors) {
      suError * error = iter;
      delete error;
    }
    
  } // end of suErrorManager::~suErrorManager


  // ------------------------------------------------------------
  // -
  // --- Public static methods
  // -
  // ------------------------------------------------------------

  // static
  void suErrorManager::delete_instance ()
  {
    if (suErrorManager::_instance)
      delete suErrorManager::_instance;
  
    suErrorManager::_instance = 0;
    
  } // end of suErrorManager::delete_instance


  // ------------------------------------------------------------
  // -
  // --- Public methods
  // -
  // ------------------------------------------------------------

  //
  void suErrorManager::add_error (const std::string & errorname,
                                  const suRectangle & rect)
  {
    add_error (errorname,
               rect.xl(),
               rect.yl(),
               rect.xh(),
               rect.yh());
    
  } // end of suErrorManager::add_error

  //
  void suErrorManager::add_error (const std::string & errorname,
                                  sutype::dcoord_t x1,
                                  sutype::dcoord_t y1,
                                  sutype::dcoord_t x2,
                                  sutype::dcoord_t y2)
  {
    suError * error = 0;

    for (const auto & iter : _errors) {
      suError * error2 = iter;
      if (error2->name().compare (errorname) == 0) {
        error = error2;
        break;
      }
    }

    if (error == 0) {
      error = new suError (errorname);
      _errors.push_back (error);
    }

    error->add_rectangle (x1, y1, x2, y2);

    SUINFO(1)
      << "Saved an error"
      << ": " << errorname
      << "; (" << x1 << "," << y1 << ")-(" << x2 << "," << y2 << ")"
      << std::endl;
    
  } // end of suErrorManager::add_error

  //
  void suErrorManager::dump_error_file (const std::string & dirname)
    const
  {
    const std::string defaultoffset = std::string ("    ");
    std::string offset = std::string ("");
    int numoffsets = 0;

    sutype::uvi_t totalerrors = 0;

    for (const auto & iter : _errors) {
      suError * error = iter;
      totalerrors += error->rectangles().size();
    }

//     if (totalerrors == 0) {
//       SUINFO(1) << "No errors found. Do not dump error file." << std::endl;
//       return;
//     }

    std::string filename = "";

    if (!dirname.empty()) {
      filename = dirname;
      if (dirname.c_str()[dirname.length()-1] != '/')
        filename += "/";
    }
    
    filename += _cellName + "." + _fileExtension;

    suStatic::create_parent_dir (filename);

    std::ofstream out (filename);
    if (!out.is_open()) {
      SUISSUE("Could not open file for writing") << ": " << filename << std::endl;
      SUASSERT (false, "");
    }

    out
      << offset
      << "(FileExtension \"" << _fileExtension << "\")"
      << std::endl;

    out
      << offset
      << "(Version \"Grammar version: 2.0\" \"Tool: Analog Router\")"
      << std::endl;

    out
      << offset
      << "(Cell: \"" << _cellName << "\""
      << std::endl;

    ++numoffsets; offset = ""; for (int n=0; n < numoffsets; ++n) { offset += defaultoffset; }

    out
      << offset
      << "(Comment: \"This file is created from Analog Router database\")"
      << std::endl;

     out
      << offset
      << "(ConversionFactor " << _conversionFactor << ")"
      << std::endl;
    
     out
      << offset
      << "(TotalErrors " << totalerrors << ")"
      << std::endl;

     for (const auto & iter1 : _errors) {

       suError * error = iter1;
       const sutype::strings_t & errordescriptions = error->descriptions();
       const sutype::rectangles_t & rectangles = error->rectangles();
       
       out
         << offset
         << "(ErrorSet \"" << error->name() << "\" \"" << error->type() << "\""
         << std::endl;

       ++numoffsets; offset = ""; for (int n=0; n < numoffsets; ++n) { offset += defaultoffset; }

       // ErrorDescriptions
       if (1) {
       
         out
           << offset
           << "(ErrorDescriptions " << errordescriptions.size()
           << std::endl;

         ++numoffsets; offset = ""; for (int n=0; n < numoffsets; ++n) { offset += defaultoffset; }

         for (const auto & iter2 : errordescriptions) {
         
           const std::string & errordescription = iter2;
         
           out
             << offset
             << "\"" << errordescription << "\""
             << std::endl;
         }

         --numoffsets; offset = ""; for (int n=0; n < numoffsets; ++n) { offset += defaultoffset; }
       
         // end of ErrorDescriptions
         out
           << offset
           << ")"
           << std::endl;
       }

       // ErrorPolygons
       if (1) {
         
         out
           << offset
           << "(ErrorPolygons " << rectangles.size()
           << std::endl;

         ++numoffsets; offset = ""; for (int n=0; n < numoffsets; ++n) { offset += defaultoffset; }

         for (const auto & iter2 : rectangles) {

           const suRectangle * rectangle = iter2;
           
           out
             << offset
             << "(Polygon 4"
             << std::endl;

           ++numoffsets; offset = ""; for (int n=0; n < numoffsets; ++n) { offset += defaultoffset; }

           // corner points
           out << offset << "(" << rectangle->xl() << ", " << rectangle->yl() << ")" << std::endl;
           out << offset << "(" << rectangle->xl() << ", " << rectangle->yh() << ")" << std::endl;
           out << offset << "(" << rectangle->xh() << ", " << rectangle->yh() << ")" << std::endl;
           out << offset << "(" << rectangle->xh() << ", " << rectangle->yl() << ")" << std::endl;
           
           --numoffsets; offset = ""; for (int n=0; n < numoffsets; ++n) { offset += defaultoffset; }
           
           // end of Polygon
           out
             << offset
             << ")"
             << std::endl;
         }

         --numoffsets; offset = ""; for (int n=0; n < numoffsets; ++n) { offset += defaultoffset; }
       
         // end of ErrorPolygons
         out
           << offset
           << ")"
           << std::endl;
       }

       --numoffsets; offset = ""; for (int n=0; n < numoffsets; ++n) { offset += defaultoffset; }
       
       // end of ErrorSet
       out
         << offset
         << ")"
         << std::endl;
     }
    
     --numoffsets; offset = ""; for (int n=0; n < numoffsets; ++n) { offset += defaultoffset; }
     
     // end of Cell
     out
       << offset
       << ")"
       << std::endl;
     
     out.close ();
     
     if (!_errors.empty()) {
       
       SUISSUE("Written a non-empty error file")
         << ": " << filename
         << std::endl;
     }
     
     SUASSERT (numoffsets == 0, "");
     
  } // end of suErrorManager::dump_error_file
  
  
  // ------------------------------------------------------------
  // -
  // --- Private static methods
  // -
  // ------------------------------------------------------------


  // ------------------------------------------------------------
  // -
  // --- Private methods
  // -
  // ------------------------------------------------------------


} // end of namespace amsr

// end of suErrorManager.cpp
