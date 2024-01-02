#ifndef __MSG_HPP__
#define __MSG_HPP__

#include <iostream> // FIXME
#include <chrono>
#include <time.h>

template <class T>
struct Msg {
public:
    T data;
    timespec GetTime() {return time_;}
    inline void SetTime() {
        clock_gettime(CLOCK_MONOTONIC, &time_);
    }

private:
    timespec time_;
};

template struct Msg<int>;

#endif




