#ifndef __PERIODIC_THREAD_HPP__
#define __PERIODIC_THREAD_HPP__

#include<thread>
#include<cstdio>
#include<functional>
#include<chrono>
#include<unistd.h>
#include<stdexcept>
#include<time.h>


class PeriodicThread 
{
public:
#ifdef VERSION_C
    explicit PeriodicThread(uint64_t period_usec, std::function<void()> f): f_(f) {
        t_ = std::make_unique<std::thread>([=](){
                timespec t_next;
                clock_gettime(CLOCK_MONOTONIC, &t_next);

                while(true) {
                    f_(); // Periodic code

                    t_next.tv_nsec += period_usec;
                    if (t_next.tv_nsec >= 1000000000) {
                        t_next.tv_nsec -= 1000000000;
                        t_next.tv_sec += 1;
                    }
                    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t_next, NULL);
                }
        });
        std::printf("Using C version\n");
    }
#else
    explicit PeriodicThread(uint64_t period_usec, std::function<void()> f): f_(f) {
        t_ = std::make_unique<std::thread>([=](){
                using clock = std::chrono::high_resolution_clock;
                auto t_next = clock::now();

                while(true) {
                    f_(); // Periodic code

                    t_next += std::chrono::nanoseconds(period_usec);
                    std::this_thread::sleep_until(t_next);
                }
        }); 
        std::printf("Using C++ version\n");
    }
#endif

    ~PeriodicThread(){
        t_->join();
    } 
    
private:
    std::function<void()> f_;
    std::unique_ptr<std::thread> t_;
};

#endif // __PERIODIC_THREAD_HPP__
