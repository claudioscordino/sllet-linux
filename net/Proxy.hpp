#ifndef __PROXY_EVENT_HPP__
#define __PROXY_EVENT_HPP__

#include "Msg.hpp"
#include <time.h> // clock_gettime
#include <string>
#include <cstdint>
#include <unistd.h>
#include <functional>

// Proxy:: Subscriber

#ifdef SLLET
#include <sdll.hpp>
#include"timespec_support.h"

template <class D>
class Sdll_timespec: public Sdll<timespec, D> {
public:
    virtual bool Compare (const timespec* t1, const timespec* t2) override {
        return timespeccmp(t1, t2, <);
    }
};

#endif

template <class T>
class Proxy {
public:
    
    explicit Proxy (const uint16_t port) {
        Connect(port);
    }
    

    ~Proxy(){
        close(fd_);
        // No need to delete elements in queue_ because
        // they are deleted by Sdl's dtor.
    }

#ifdef SLLET
    // Returns the latest "valid" message
    inline Msg<T> GetNewSamples() {
        Msg<T> tmp;
        while (read(fd_, &tmp, sizeof(tmp)) == sizeof(tmp)) {
                // Enqueue all messages
                Msg<T>* msg = new Msg<T>;
                *msg = tmp;
                queue_.Insert(msg->GetTime(), msg);
        }

        // Dequeue
        timespec now, first;
        clock_gettime(CLOCK_MONOTONIC, &now);
        while (true) {
            bool ret = queue_.CheckFirst(&first);
            if ((!ret) || (timespeccmp(&now, &first, <)))
                break;
            Msg<T>* m = queue_.Extract();
            last_ = *m;
            delete m;
        }

        return last_;
    }
#else
    // Returns the latest "valid" message
    inline Msg<T> GetNewSamples() {
        Msg<T> tmp;
        while (read(fd_, &tmp, sizeof(tmp)) == sizeof(tmp)) {
                last_ = tmp;
        }

        return last_;
    }
#endif

private:
    void Connect (const uint16_t port);
    int fd_;
    Msg<T> last_;
#ifdef SLLET
    Sdll_timespec<Msg<T>> queue_;
#endif
};

#endif

