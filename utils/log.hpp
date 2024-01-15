#ifndef __LOG_HPP__
#define __LOG_HPP__

#include <iostream>

// #define LOG_ENABLED

#ifdef LOG_ENABLED
#define LOG(msg) std::cout << msg << std::endl;
#else
#define LOG(msg) 
#endif

#endif

