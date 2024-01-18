#ifndef __LOG_HPP__
#define __LOG_HPP__

// Comment to disable logging
// #define LOG_ENABLED

#ifdef LOG_ENABLED

#include <iostream>
#include <iostream>
#include <string>
#include <sstream>


#define LOG(msg) { \
    std::ostringstream ss; \
    ss << msg << std::endl; \
    std::cout << ss.str(); \
    }
#else
#define LOG(msg) 
#endif

#endif

