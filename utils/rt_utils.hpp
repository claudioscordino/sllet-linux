#ifndef __RT_UTILS_HPP__
#define __RT_UTILS_HPP__

#include<cstdio>
#include<cstdint>

const uint64_t max = 10000000;
static uint64_t cycles_per_usec = 0;

static inline void waste_cycles(uint64_t cycles)
{
    for (uint64_t c = 0; c < cycles; ++c)
        __asm volatile ("mov %eax, %eax");
}

inline void waste_usec(uint64_t usec)
{
    waste_cycles(usec*cycles_per_usec);
}


#ifdef VERSION_C
#include<time.h>
void calibrate_cpu()
{
    timespec start, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);    
    waste_cycles(max);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);    
    uint64_t duration = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_nsec - start.tv_nsec)/1000;
    cycles_per_usec = max/duration;
    std::printf("usecs = %ld\n", duration);
    std::printf("cycles = %ld\n", max);
    std::printf("1 usec = %ld\n\n", cycles_per_usec);
}
#else
#include<cstdio>
#include<chrono>

void calibrate_cpu()
{
    using clock = std::chrono::high_resolution_clock;
    auto start = clock::now();
    waste_cycles(max);
    auto end = clock::now();
    uint64_t duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    cycles_per_usec = max/duration;
    std::printf("usecs = %ld\n", duration);
    std::printf("cycles = %ld\n", max);
    std::printf("1 usec = %ld\n\n", cycles_per_usec);
}
#endif

#endif //__RT_UTILS_HPP__
