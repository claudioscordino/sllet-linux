#ifndef __STATS_HPP__
#define __STATS_HPP__

#include<iostream>
#include<chrono>
#include<time.h>
#include<condition_variable>
#include<mutex>

#include"Msg.hpp"
#include"Skeleton.hpp"
#include"Proxy.hpp"
#include"PeriodicThread.hpp"

struct Stats {
    int last_processed_msg = -1;
    Skeleton<int>* skel;
    Proxy<int>* proxy;
    uint64_t worst_case_delay = 0;
    uint64_t best_case_delay = 1000000000;
    uint64_t sum_delay = 0;
    uint64_t received_messages = 0;
    uint64_t recv_activations = 0;
    uint64_t sent_messages = 0;

    PeriodicThread* send_th;
    PeriodicThread* recv_th;

#ifdef SLLET
    std::condition_variable recv_exec_cond;
    std::mutex recv_exec_lock;
    pthread_t recv_exec_tid;

    std::condition_variable send_exec_cond;
    std::mutex send_exec_lock;
    pthread_t send_exec_tid;
#endif
    Msg<int> send_msg;
    Msg<int> recv_msg;
};

// Once received a message (available in c->recv_msg), this
// function updates the statistics.
// Delay is defined as difference between sender's and
// receiver's activation times.
void update_stats(Stats *c)
{
    Msg<int>* msg = &(c->recv_msg);
    LOG("[RECV] data = " << msg->data);
    if (msg->data == 0)
        return; // No messages yet available
    if (c->last_processed_msg == msg->data)
        return; // Discard messages already processed

    timespec now = c->recv_th->getCurrActivationTime();
    timespec orig, elapsed;
    orig = msg->GetStatsTime();

    LOG("[RECV] Stats time was: " << msg->GetStatsTime().tv_sec << "." << 
            msg->GetStatsTime().tv_nsec);

    // Ensure no adjustments have been performed (e.g. by NTP or PTP)
    if (timespeccmp(&orig, &now, <)) {
        timespecsub(&now, &orig, &elapsed);
        LOG("[RECV] Diff is: " << elapsed.tv_sec << "." << elapsed.tv_nsec);
        uint64_t delay = (elapsed.tv_sec*1000000) + (elapsed.tv_nsec/1000);
        LOG("[RECV] Delay: " << delay << " nsec");
        if (c->worst_case_delay < delay) 
            c->worst_case_delay = delay;
        if (c->best_case_delay > delay) 
            c->best_case_delay = delay;
        c->sum_delay += delay;
    }
    c->received_messages++;
    c->last_processed_msg = msg->data;
}

#endif

