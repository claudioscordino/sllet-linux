#ifndef __PROXY_EVENT_HPP__
#define __PROXY_EVENT_HPP__

#include "Msg.hpp"
#include "log.hpp"

#include <time.h> // clock_gettime
#include <string>
#include <cstdint>
#include <unistd.h>
#include <functional>
#include <memory>

// Proxy:: Subscriber

#if defined(SLLET) || defined (SLLET_TS)
#include "timespec_support.h"

#include <sdll.hpp>

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
    }

#if defined(SLLET) || defined (SLLET_TS)
    // Returns the latest message whose Let time is < currAct
    inline Msg<T> GetNewSamples(timespec currAct) {
        LOG("[RCV] Accepting messages with LET time < " << currAct.tv_sec << "." << 
                currAct.tv_nsec << " nsec");
        Msg<T> tmp;
        while (read(fd_, &tmp, sizeof(tmp)) == sizeof(tmp)) {
                LOG("[RCV] Read message with LET time:  " << tmp.GetLetTime().tv_sec 
                    << "." << tmp.GetLetTime().tv_nsec << " nsec");
                LOG("[RCV] Read message with STATS time:" << tmp.GetStatsTime().tv_sec 
                    << "." << tmp.GetStatsTime().tv_nsec << " nsec");
                LOG("[RCV] Enqueuing message...");
    
                // Enqueue all messages
                std::unique_ptr<Msg<T>> msg = std::make_unique<Msg<T>>();
                *msg = tmp;
                queue_.Insert(tmp.GetLetTime(), std::move(msg));
        }

        // Dequeue
        timespec let_time;
        LOG("[RCV] Dequeuing messages...");
        while (true) {
            bool ret = queue_.CheckFirst(&let_time);
            if (!ret) {
                LOG("[RCV] Queue is empty. Returning...");
                break;
            } 
            LOG("[RCV] Read message with LET time:" << let_time.tv_sec 
                << "." << let_time.tv_nsec << " nsec");
            if (timespeccmp(&let_time, &currAct, >)) {
                LOG("[RCV] Discarding message");
                break;
            } else {
                LOG("[RCV] Accepting message");
            }
            std::unique_ptr<Msg<T>> m = queue_.Extract();
            last_ = *m;
        }

        return last_;
    }
#else
    // Returns the latest "valid" message
    inline Msg<T> GetNewSamples() {
        Msg<T> tmp;
        while (read(fd_, &tmp, sizeof(tmp)) == sizeof(tmp)) {
                LOG("[RCV] Message with data " << tmp.data);
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

