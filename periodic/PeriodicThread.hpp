#ifndef __PERIODIC_THREAD_HPP__
#define __PERIODIC_THREAD_HPP__

#include <thread>
#include <cstdio>
#include <functional>
#include <unistd.h>
#include <stdexcept>
#include <time.h>
#include <atomic>
#include <iostream>
#include <mutex>
#include <pthread.h>


class PeriodicThread 
{
public:
    explicit PeriodicThread(uint64_t period_usec,
            std::function<void(PeriodicThread*, void*)> f,
            void* arg): f_(f) {
        t_ = std::make_unique<std::thread>([=](){
                clock_gettime(CLOCK_MONOTONIC, &act_next_);
                timespec period = {0, ((long int) period_usec)*1000};

                while(!stop_) {

                    timespec sleep_until;
                    {
                        std::lock_guard lock(lock_);
                        act_curr_ = act_next_;
                        timespecadd(&act_next_, &period, &act_next_);
                        sleep_until = act_next_; // Used to release the lock
                    }

                    timespec now;
                    clock_gettime(CLOCK_MONOTONIC, &now);
                    if (timespeccmp(&now, &sleep_until, >))
                        deadline_miss_++;

                    f_(this, arg); // Periodic code
                    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &sleep_until, NULL);
                }
        });
    }

    ~PeriodicThread(){
        t_->join();
    } 

    inline void stop(){
        stop_ = true;
    }

    inline void join(){
        t_->join();
    }

    inline timespec getNextActivationTime() {
        timespec ret;
        std::lock_guard lock(lock_);
        ret = act_next_;
        return ret;
    }

    inline uint64_t getDeadlineMiss() {
        return deadline_miss_;
    }

    inline timespec getCurrActivationTime() {
        timespec ret;
        std::lock_guard lock(lock_);
        ret = act_curr_;
        return ret;
    }

    bool set_rt_prio(uint8_t prio) {
        int policy;
        struct sched_param param;
        if (pthread_getschedparam(t_->native_handle(), &policy, &param) != 0) {
            std::cerr << "ERROR: pthread_getschedparam()" << std::endl;
            return false;
        }
        param.sched_priority = prio;
        if (pthread_setschedparam(t_->native_handle(), SCHED_FIFO, &param) != 0) {
            std::cerr << "ERROR: pthread_setschedparam()" << std::endl;
            return false;
        }
        return true;
    }
    
private:
    std::function<void(PeriodicThread*, void* arg)> f_;
    std::unique_ptr<std::thread> t_;
    std::atomic<bool> stop_ = false;
    std::atomic<uint64_t> deadline_miss_ = 0;

    std::mutex lock_;
    timespec act_next_;
    timespec act_curr_;
};

#endif // __PERIODIC_THREAD_HPP__
