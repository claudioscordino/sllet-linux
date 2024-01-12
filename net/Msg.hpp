#ifndef __MSG_HPP__
#define __MSG_HPP__

#include <iostream> // FIXME
#include <chrono>
#include <time.h>
#include <timespec_support.h>

template <class T>
struct Msg {
public:
    T data;
#if defined(SLLET) || defined(SLLET_TS)
    inline timespec GetLetTime() {
        return let_time_;
    }
    inline void SetLetTime() {
        clock_gettime(CLOCK_MONOTONIC, &let_time_);
    }
    inline void SetLetTime(timespec time) {
        let_time_ = time;
    }
    inline void AddLetTime(timespec delta) {
        timespecadd(&let_time_, &delta, &let_time_);
    }
#endif

    inline timespec GetStatsTime() {
        return stats_time_;
    }
    inline void SetStatsTime() {
        clock_gettime(CLOCK_MONOTONIC, &stats_time_);
    }
    inline void SetStatsTime(timespec time) {
        stats_time_ = time;
    }

private:
#if defined(SLLET) || defined(SLLET_TS)
    timespec let_time_;
#endif
    timespec stats_time_;
};

template struct Msg<int>;

#endif




