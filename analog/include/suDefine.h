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
//! \date   Wed Sep 27 14:05:22 2017

//! \file   suDefine.h
//! \brief  A collection of hard-coded defines

#ifndef _suDefine_h_
#define _suDefine_h_

#include <string>

#ifdef _DEBUG_LAYOUT_FUNCTIONS_

//#define SUFILELINE_DLF std::string (__FILE__ + __LINE__)
#define SUFILELINE_DLF std::string (__FILE__), __LINE__

#else // _DEBUG_LAYOUT_FUNCTIONS_

#define SUFILELINE_DLF "", 0

#endif // _DEBUG_LAYOUT_FUNCTIONS_

#endif // _suDefine_h_

// end of suDefine.h
