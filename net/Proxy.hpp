#ifndef __PROXY_EVENT_HPP__
#define __PROXY_EVENT_HPP__

#include "Msg.hpp"
#include <time.h> // clock_gettime
#include <string>
#include <cstdint>
#include <unistd.h>
#include <functional>

// Proxy:: Subscriber

#if defined(SLLET) || defined (SLLET_TS)
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

#if defined(SLLET) || defined (SLLET_TS)
    // Returns the latest "valid" message whose time is >= now
    inline Msg<T> GetNewSamples(timespec now) {
        std::cout << "[RCV] Accepting messages with LET time:  " << now.tv_sec 
            << "." << now.tv_nsec << " nsec" << std::endl;
        Msg<T> tmp;
        while (read(fd_, &tmp, sizeof(tmp)) == sizeof(tmp)) {
                std::cout << "[RCV] Read message with LET time:  " << tmp.GetLetTime().tv_sec 
                    << "." << tmp.GetLetTime().tv_nsec << " nsec" << std::endl;
                std::cout << "[RCV] Read message with STATS time:" << tmp.GetStatsTime().tv_sec 
                    << "." << tmp.GetStatsTime().tv_nsec << " nsec" << std::endl;
                std::cout << "[RCV] Enqueuing message..." << std::endl;
    
                // Enqueue all messages
                Msg<T>* msg = new Msg<T>;
                *msg = tmp;
                queue_.Insert(msg->GetLetTime(), msg);
        }

        // Dequeue
        timespec rcv_time;
        std::cout << "[RCV] Dequeuing messages..." << std::endl;
        while (true) {
            bool ret = queue_.CheckFirst(&rcv_time);
            if (!ret) {
                std::cout << "[RCV] Queue is empty. Returning..." << std::endl;
                break;
            } 
            std::cout << "[RCV] Read message with LET time:" << rcv_time.tv_sec 
                << "." << rcv_time.tv_nsec << " nsec" << std::endl;
            if (timespeccmp(&now, &rcv_time, <)) {
                std::cout << "[RCV] Discarding message" << std::endl;
                break;
            } else {
                std::cout << "[RCV] Accepting message" << std::endl;
            }
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
#if defined(SLLET) || defined (SLLET_TS)
    Sdll_timespec<Msg<T>> queue_;
#endif
};

#endif

