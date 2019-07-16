#ifndef CustomAssert_h
#define CustomAssert_h

// Nikolai; Sep 5, 2017; added

#include <assert.h>

#include <iostream>

#define MYCOUT if (1) \
    std::cout << "-I- (" << __FILE__ << ":" << __LINE__ << "): "

#define CustomAssert(value,message) if (!(value)) { \
                                      MYCOUT << "Program asserted." << std::endl;                          \
                                      MYCOUT << "Assert message: " << message << std::endl; \
                                      throw OutOfMemoryException(); \
                                    }

#endif // CustomAssert_h
