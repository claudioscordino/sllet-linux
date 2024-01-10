#define VERSION_C
#include"rt_utils.hpp"

#include"Msg.hpp"
#include"Stats.hpp"
#include"Proxy.hpp"
#include"PeriodicThread.hpp"
#include"Skeleton.hpp"
#include"timespec_support.h"

#include<thread>
#include<chrono>
#include<iostream>
#include<cstdio>
#include<unistd.h>
#include<atomic>
#include<string>
#include<sched.h>
#include<sys/time.h>
#include<sys/resource.h>

const timespec interconnect_task = {0, 2000000}; // 2ms

static uint16_t pairs_nb = 200;
static uint64_t period_usec = 1000;
static uint64_t duration_sec = 10;
static uint64_t activations = 0;

static const uint16_t port = 1236;

std::atomic<bool> stop = false;

Stats *pairs = nullptr;

inline void processing()
{
    waste_usec(1000);
}

///////////////// Sender

#ifdef SLLET
void* snd_processing(void* arg)
{
    Stats *c = (Stats*) arg;
    while (!stop) {
        std::unique_lock lock(c->snd_exec_lock);
        c->snd_exec_cond.wait(lock);
        processing();
    }
    return nullptr;
}
#endif

void do_send(PeriodicThread *th, void* arg)
{
    (void) th;
    static std::atomic<int> counter = 1;
    Stats *c = (Stats*) arg;
    Skeleton<int> *skel = c->skel;
    Msg<int> msg;
    msg.data = counter++;
#ifdef SLLET
    // Send previous message (except for first round)
    if (counter > 2) {
        msg.SetTime();
        msg.AddTime(interconnect_task);
        skel->Send(msg);
        (c->sent_messages)++;
    }
    // Execute some stuff at lower priority
    c->snd_exec_cond.notify_one();
#elif SLLET_TS
    processing();
    msg.SetTime(th->getNextActivationTime());
    msg.AddTime(interconnect_task);
    skel->Send(msg);
    (c->sent_messages)++;
#else
    processing();
    msg.SetTime();
    skel->Send(msg);
    (c->sent_messages)++;
#endif
}

// Sender is periodic and sends the message
void* sender (void* arg)
{
    try {
#ifdef SLLET
        Stats *c = (Stats*) arg;
        pthread_create(&(c->snd_exec_tid), NULL, snd_processing, arg);
#endif
        [[maybe_unused]] auto pt = new PeriodicThread(period_usec, do_send, arg);
#ifdef SLLET
        // In case of SL-LET, the sending thread is high priority
        if (!pt->set_rt_prio(90))
            exit(-1);
#endif
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return nullptr;
}

///////////////// Receiver

#ifdef SLLET
void* rcv_processing(void* arg)
{
    Stats *c = (Stats*) arg;
    while (!stop) {
        std::unique_lock lock(c->rcv_exec_lock);
        c->rcv_exec_cond.wait(lock);
        processing();
    }
    return nullptr;
}
#endif


void do_receive (PeriodicThread *th, void* arg)
{
    Stats *c = (Stats*) arg;
    Proxy<int> * proxy = c->proxy;
    c->recv_activations++;
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    printf("Time is: %ld.%ld nsec\n", now.tv_sec, now.tv_nsec);
#ifdef SLLET
    Msg<int> msg = proxy->GetNewSamples(now);
#elif SLLET_TS
    Msg<int> msg = proxy->GetNewSamples(th->getCurrActivationTime());
#else
    Msg<int> msg = proxy->GetNewSamples();
#endif
    printf("Received %d\n", msg.data);
    if (msg.data == 0)
        return; // No messages yet available
    timespec orig, elapsed;
    orig = msg.GetTime();
#if defined(SLLET) || defined(SLLET_TS)
    printf("Removing delay of interconnect task... \n");
    timespecsub(&orig, &interconnect_task, &orig); 
#endif

    printf("Time was: %ld.%ld nsec\n", orig.tv_sec, orig.tv_nsec);
    if (timespeccmp(&orig, &now, <)) {
        timespecsub(&now, &orig, &elapsed);
        printf("Diff is: %ld.%ld nsec\n", elapsed.tv_sec, elapsed.tv_nsec);
        uint64_t delay = (elapsed.tv_sec*1000000) + (elapsed.tv_nsec/1000);
        printf("Delay: %lu usec\n", delay);
        if (c->worst_case_delay < delay) 
            c->worst_case_delay = delay;
        if (c->best_case_delay > delay) 
            c->best_case_delay = delay;
        c->sum_delay += delay;
    }
    c->received_messages++;
    if (c->recv_activations == activations) {
        stop = true;
        th->stop();
    }
#ifndef SLLET
    processing();
#else
    // Execute some stuff at lower priority
    c->rcv_exec_cond.notify_one();
#endif
}

// Receiver is event-triggered and measures the delay
void * receiver (void* arg)
{
    try {
#ifdef SLLET
        Stats *c = (Stats*) arg;
        pthread_create(&(c->rcv_exec_tid), NULL, rcv_processing, arg);
#endif
        [[maybe_unused]] auto pt = new PeriodicThread(period_usec, do_receive, arg);
#ifdef SLLET
        // In case of SL-LET, the receiving thread is high priority
        if (!pt->set_rt_prio(90))
            exit(-1);
#endif
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return nullptr;
}

int main (int argc, char* argv[])
{
    calibrate_cpu();
    try {
        if (argc != 4) { 
            std::cerr << "Wrong number of arguments" << std::endl;
            std::cerr << "Usage:" << std::endl;
            std::cerr << "\t" << argv[0] << "  pairs  period_usec  duration_sec" << std::endl;
            return -1;
        }
 
        pairs_nb = std::stoi(std::string(argv[1]));
        period_usec = std::stoi(std::string(argv[2]));
        duration_sec = std::stoi(std::string(argv[3]));
        activations = (duration_sec*1000*1000)/period_usec;
        std::cout << "Pairs = " << pairs_nb << std::endl;
        std::cout << "Period = " << period_usec << std::endl;
        std::cout << "Duration = " << duration_sec << std::endl;
        std::cout << "Activations = " << activations << std::endl;

        pairs = new Stats[pairs_nb];

        struct rusage ru1, ru2;
        getrusage(RUSAGE_SELF, &ru1);
        
        
        for (int i=0; i < pairs_nb; ++i) {
            pairs[i].proxy = new Proxy<int> (port+i);
        }
        for (int i=0; i < pairs_nb; ++i) {
            pairs[i].skel = new Skeleton<int> ("127.0.0.1", port+i);
        }

        // Receivers
        for (int i=0; i < pairs_nb; ++i) {
            receiver(&pairs[i]);
        }
        
        // Senders
        for (int i=0; i < pairs_nb; ++i) {
            sender(&pairs[i]);
        }
        sleep (duration_sec);
        getrusage(RUSAGE_SELF, &ru2);


        for (int i=0; i < pairs_nb; ++i) {
            delete pairs[i].skel;
            delete pairs[i].proxy;
        }

        uint64_t worst = 0;
        uint64_t best = 1000000000;
        uint64_t sum_avg = 0;
        uint64_t received_messages = 0;
        uint64_t sent_messages = 0;

        for (int i=0; i < pairs_nb; ++i) {
            if (pairs[i].worst_case_delay > worst)
                worst = pairs[i].worst_case_delay;
            if (pairs[i].best_case_delay < best)
                best = pairs[i].best_case_delay;
            if (pairs[i].received_messages != 0) {
                uint64_t avg = (pairs[i].sum_delay)/(pairs[i].received_messages);
                sum_avg += avg;
            } else {
                std::cerr << "ERROR: received messages = 0" << std::endl;    
            }
            received_messages += pairs[i].received_messages;
            sent_messages += pairs[i].sent_messages;
        }
        std::cout << std::endl << std::endl;
        std::cout << "VVVVVVVVVVVVVVVVVVV" << std::endl;
#ifdef SLLET
        std::cout << "SL-LET used: standard" << std::endl;
#elif SLLET_TS
        std::cout << "SL-LET used: timestamp" << std::endl;
#else
        std::cout << "SL-LET used: none" << std::endl;
#endif
        
        std::cout << "Received messages = " << received_messages << std::endl;
        std::cout << "Sent messages = " << sent_messages << std::endl;
        std::cout << "Avg delay = " << sum_avg/pairs_nb << " usec" << std::endl;
        std::cout << "Worst delay = " << worst << " usec" << std::endl;
        std::cout << "Best delay = " << best << " usec" << std::endl;
        std::cout << "Max jitter = " << (worst-best)/2 << " usec" << std::endl;
        std::cout << "User usage = " << 
            (ru2.ru_utime.tv_sec - ru1.ru_utime.tv_sec) << "." <<
            (ru2.ru_utime.tv_usec - ru1.ru_utime.tv_usec) << " usec" << std::endl;
        std::cout << "System usage = " << 
            (ru2.ru_stime.tv_sec - ru1.ru_stime.tv_sec) << "." <<
            (ru2.ru_stime.tv_usec - ru1.ru_stime.tv_usec) << " usec" << std::endl;
        std::cout << "Total usage = " << 
            (ru2.ru_utime.tv_sec - ru1.ru_utime.tv_sec) +
            (ru2.ru_stime.tv_sec - ru1.ru_stime.tv_sec) << "." <<
            (ru2.ru_utime.tv_usec - ru1.ru_utime.tv_usec) +
            (ru2.ru_stime.tv_usec - ru1.ru_stime.tv_usec) << " usec" << std::endl;
        std::cout << "AAAAAAAAAAAAAAAAAAA" << std::endl;
        std::cout << std::flush;

        delete pairs;
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

        
