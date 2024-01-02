#ifndef __PROXY_EVENT_HPP__
#define __PROXY_EVENT_HPP__

#include "Msg.hpp"
#include <time.h> // clock_gettime
#include <string>
#include <cstdint>
#include <unistd.h>
#include <functional>

// Proxy:: Subscriber

template <class T>
class Proxy {
public:
    
    explicit Proxy (const uint16_t port) {
        Connect(port);
    }
    

    ~Proxy(){
        close(fd_);
    }

    inline Msg<T> GetNewSamples() {
        Msg<T> tmp;
        while (true) {
            if (read(fd_, &tmp, sizeof(tmp)) == sizeof(tmp))
                last_ = tmp;
            else
                break;
        }

        return last_;
    }

private:
    void Connect (const uint16_t port);
    int fd_;
    Msg<T> last_;
};

#endif

