#ifndef __SKELETON_EVENT_HPP__
#define __SKELETON_EVENT_HPP__

#include "Msg.hpp"

#include <time.h> // clock_gettime
#include <cstdint>
#include <unistd.h>
#include <iostream>

// Skeleton: Publisher

template <class T>
class Skeleton {
public:
    explicit Skeleton (const std::string& address, const uint16_t port);
    ~Skeleton() {
        close(fd_);
    }
    bool Send(Msg<T>);

private:
    int fd_;
};

#endif
