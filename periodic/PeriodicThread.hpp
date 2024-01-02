#ifndef __PERIODIC_THREAD_HPP__
#define __PERIODIC_THREAD_HPP__

#include<thread>
#include<cstdio>
#include<functional>
#include<unistd.h>
#include<stdexcept>
#include<time.h>
#include<atomic>
#include<iostream>
#include<pthread.h>


class PeriodicThread 
{
public:
    explicit PeriodicThread(uint64_t period_usec,
            std::function<void(PeriodicThread*, void*)> f,
            void* arg): f_(f) {
        t_ = std::make_unique<std::thread>([=](){
                clock_gettime(CLOCK_MONOTONIC, &act_next_);

                while(!stop_) {
                    act_next_.tv_nsec += (period_usec*1000);
                    if (act_next_.tv_nsec >= 1000000000) {
                        act_next_.tv_nsec -= 1000000000;
                        act_next_.tv_sec += 1;
                    }

                    f_(this, arg); // Periodic code
                    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &act_next_, NULL);
                }
        });
    }

    ~PeriodicThread(){
        t_->join();
    } 

    inline void stop(){
        join();
    }

    inline void join(){
        t_->join();
    }

    inline timespec getNextActivation() {
        return act_next_;
    }

    void set_rt_prio() {
        int policy;
        struct sched_param param;
        if (pthread_getschedparam(t_->native_handle(), &policy, &param) != 0) {
            std::cerr << "ERROR: pthread_getschedparam()" << std::endl;
            return;
        }
        param.sched_priority = 90;
        if (pthread_setschedparam(t_->native_handle(), SCHED_FIFO, &param) != 0) {
            std::cerr << "ERROR: pthread_setschedparam()" << std::endl;
            return;
        }
    }
    
private:
    std::function<void(PeriodicThread*, void* arg)> f_;
    std::unique_ptr<std::thread> t_;
    std::atomic<bool> stop_ = false;
    timespec act_next_;
};

#endif // __PERIODIC_THREAD_HPP__
