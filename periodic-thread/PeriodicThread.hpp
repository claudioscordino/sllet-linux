#ifndef __PERIODIC_THREAD_HPP__
#define __PERIODIC_THREAD_HPP__

#include<thread>
#include<cstdio>
#include<functional>
#include<chrono>
#include<unistd.h>
#include <stdexcept>


class PeriodicThread 
{
public:
#ifdef VERSION_C
    explicit PeriodicThread(uint64_t period_usec, std::function<void()> f): 
        std::thread ([=](){
                while(true) {
                    timespec start, end;
                    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
                    f();
                    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
                    uint64_t duration = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
                    std::printf("Duration = %ld\n", duration);
                    auto sleep_duration = period_usec - duration;
                    usleep(sleep_duration);
                }
            }) {
                std::printf("Using C version\n");
        }
#else
    explicit PeriodicThread(uint64_t period_usec, std::function<void()> f): f_(f) {
        t_ = std::make_unique<std::thread>([=](){
                while(true) {
                    using clock = std::chrono::high_resolution_clock;
                    auto start = clock::now();
                    f_();
                    auto end = clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                    std::printf("Duration = %ld\n", duration);
                    auto sleep_duration = period_usec - std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                    std::this_thread::sleep_for(std::chrono::microseconds(sleep_duration));
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
