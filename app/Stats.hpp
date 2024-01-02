#ifndef __STATS_HPP__
#define __STATS_HPP__

#include<iostream>
#include<chrono>
#include<time.h>

#include"Msg.hpp"
#include"Skeleton.hpp"
#include"Proxy.hpp"

struct Stats {
    Skeleton<int>* skel;
    Proxy<int>* proxy;
    Msg<int> send_msg;
    uint64_t worst_case_delay = 0;
    uint64_t best_case_delay = 1000000000;
    uint64_t sum_delay = 0;
    uint64_t received_messages = 0;
    uint64_t recv_activations = 0;
    uint64_t sent_messages = 0;
    Msg<int> msg;
};

#endif

