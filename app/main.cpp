#define VERSION_C
#include "rt_utils.hpp"

#include "Msg.hpp"
#include "Stats.hpp"
#include "Proxy.hpp"
#include "PeriodicThread.hpp"
#include "Skeleton.hpp"
#include "timespec_support.h"
#include "log.hpp"

#include <thread>
#include <chrono>
#include <iostream>
#include <cstdio>
#include <cstdlib> // rand
#include <unistd.h>
#include <atomic>
#include <string>
#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <assert.h>


timespec interconnect_task = {0, 20000000};

static uint16_t pairs_nb = 200;
static uint64_t send_period_usec = 10000;
static uint64_t recv_period_usec = 10000;
static uint64_t max_exec_time_usec = 5000;
static uint64_t interconnect_task_usec = 20000;
static uint64_t duration_sec = 10;
static uint64_t recv_activations = 0;

static const uint16_t port = 1236;

std::atomic<bool> stop = false;

std::unique_ptr<Stats[]> pairs;

inline void processing()
{
    int r = rand()%max_exec_time_usec;
    waste_usec(r);
}

///////////////// Sender

void prepare_send_message(Stats* c)
{
    static std::atomic<int> counter = 1;
    c->send_msg.SetStatsTime(c->send_th->getCurrActivationTime());
    c->send_msg.data = counter++;
    
#if 1 // Extra checks
    timespec now, curr, next, check;
    curr = c->send_th->getCurrActivationTime();
    next = c->send_th->getNextActivationTime();

    // Check nextAct > currAct
    assert (timespeccmp(&curr, &next, <));

    // Check now > currAct
    clock_gettime(CLOCK_MONOTONIC, &now);
    assert (timespeccmp(&now, &curr, >));

    // Check nextAct = currAct + period
    timespecsub(&next, &curr, &check);
    assert(check.tv_sec == 0);
    assert(check.tv_nsec == ((long int) send_period_usec)*1000);
#endif

#ifdef SLLET
    LOG("[SEND] LET time set to " << c->send_msg.GetLetTime().tv_sec << "." << 
            c->send_msg.GetLetTime().tv_nsec);
#elif SLLET_TS
    c->send_msg.SetLetTime(c->send_th->getNextActivationTime());
    c->send_msg.AddLetTime(interconnect_task);
    LOG("[SEND] LET time set to " << c->send_msg.GetLetTime().tv_sec << "." << 
            c->send_msg.GetLetTime().tv_nsec);
    LOG("[SEND] Next activation time is " << 
            c->send_th->getNextActivationTime().tv_sec << "." << 
            c->send_th->getNextActivationTime().tv_nsec);
#endif
    LOG("[SEND] STATS time set to " << c->send_msg.GetStatsTime().tv_sec << "." << 
            c->send_msg.GetStatsTime().tv_nsec);
    LOG("[SEND] Sending message with data " << counter);
}


#ifdef SLLET
void* send_processing(void* arg)
{
    Stats *c = (Stats*) arg;
    while (!stop) {
        std::unique_lock lock(c->send_exec_lock);
        c->send_exec_cond.wait(lock);
        processing();
        prepare_send_message(c);
    }
    return nullptr;
}
#endif

bool do_send(PeriodicThread *th, void* arg)
{
    (void) th;
    if (stop)
        return false;
    Stats *c = (Stats*) arg;
#ifdef SLLET
    // Send previous message (except for first round)
    if (c->send_msg.data != 0) {
        timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        c->send_msg.SetLetTime(now);
        c->send_msg.AddLetTime(interconnect_task);
        c->skel->Send(c->send_msg);
        (c->sent_messages)++;
    }
    // Execute some stuff at lower priority
    // Equivalent to processing()
    c->send_exec_cond.notify_one();
#else // SLLET_TS and normal case are identical
    processing();
    prepare_send_message(c);

    c->skel->Send(c->send_msg);
    (c->sent_messages)++;
#endif
    return true;
}

// Sender is periodic and sends the message
inline void start_sender (Stats* c)
{
    try {
#ifdef SLLET
        pthread_create(&(c->send_exec_tid), NULL, send_processing, c);
#endif
        c->send_th = std::make_unique<PeriodicThread>(send_period_usec, do_send, c);
#ifdef SLLET
        // In case of SL-LET, the sending thread is high priority
        if (!c->send_th->set_rt_prio(90))
            exit(-1);
#endif
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

///////////////// Receiver

#ifdef SLLET
void* recv_processing(void* arg)
{
    Stats *c = (Stats*) arg;
    while (!stop) {
        std::unique_lock lock(c->recv_exec_lock);
        c->recv_exec_cond.wait(lock);
        processing();
    }
    return nullptr;
}
#endif

bool do_receive (PeriodicThread *th, void* arg)
{
    timespec now;
#if 1 // Extra checks
    timespec curr;
    curr = th->getCurrActivationTime();
    clock_gettime(CLOCK_MONOTONIC, &now);
    assert (timespeccmp(&now, &curr, >));
#endif

    if (stop)
        return false;
    Stats *c = (Stats*) arg;
    c->recv_activations++;
    LOG("Current activation time is " << 
            c->recv_th->getCurrActivationTime().tv_sec << "." << 
            c->recv_th->getCurrActivationTime().tv_nsec);
#ifdef SLLET
    c->recv_msg = c->proxy->GetNewSamples(c->recv_th->getCurrActivationTime());
    update_stats(c);
    // Execute some stuff at lower priority
    c->recv_exec_cond.notify_one();
#elif SLLET_TS
    c->recv_msg = c->proxy->GetNewSamples(c->recv_th->getCurrActivationTime());
    update_stats(c);
    processing();
#else
    c->recv_msg = c->proxy->GetNewSamples();
    update_stats(c);
    processing();
#endif
    if (c->recv_activations == recv_activations){
        stop = true;
        return false;
    }
    return true;
}

// Receiver is event-triggered and measures the delay
inline void start_receiver (Stats* c)
{
    try {
#ifdef SLLET
        pthread_create(&(c->recv_exec_tid), NULL, recv_processing, c);
#endif
        c->recv_th = std::make_unique<PeriodicThread>(recv_period_usec, do_receive, c);
#ifdef SLLET
        // In case of SL-LET, the receiving thread is high priority
        if (!c->recv_th->set_rt_prio(90))
            exit(-1);
#endif
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

int main (int argc, char* argv[])
{
    calibrate_cpu();
    try {
        if (argc != 7) { 
            std::cerr << "Wrong number of arguments" << std::endl;
            std::cerr << "Usage:" << std::endl;
            std::cerr << "\t" << argv[0] << 
                "  pairs"  <<
                "  send_period_usec" <<
                "  recv_period_usec" <<
                "  max_exec_time_usec" <<
                "  interconnect_task_usec" <<
                "  duration_sec" << 
                std::endl;
            return -1;
        }
 
        pairs_nb = std::stoi(std::string(argv[1]));
        send_period_usec = std::stoi(std::string(argv[2]));
        recv_period_usec = std::stoi(std::string(argv[3]));
        max_exec_time_usec = std::stoi(std::string(argv[4]));
        interconnect_task_usec = std::stoi(std::string(argv[5]));
        interconnect_task.tv_sec = 0;
        interconnect_task.tv_nsec = interconnect_task_usec*1000;
        duration_sec = std::stoi(std::string(argv[6]));
        recv_activations = (duration_sec*1000*1000)/recv_period_usec;
        std::cout << "Pairs = " << pairs_nb << std::endl;
        std::cout << "Sender period = " << send_period_usec << std::endl;
        std::cout << "Receiver period = " << recv_period_usec << std::endl;
        std::cout << "Interconnect task = " << interconnect_task_usec << std::endl;
        std::cout << "Max exec time = " << max_exec_time_usec << std::endl;
        std::cout << "Duration = " << duration_sec << std::endl;
        std::cout << "Receiver activations = " << recv_activations << std::endl;

        pairs = std::make_unique<Stats[]>(pairs_nb);

        struct rusage ru1, ru2;
        getrusage(RUSAGE_SELF, &ru1);
        
        
        for (int i=0; i < pairs_nb; ++i) {
            pairs[i].proxy = std::make_unique<Proxy<int>> (port+i);
        }
        for (int i=0; i < pairs_nb; ++i) {
            pairs[i].skel = std::make_unique<Skeleton<int>> ("127.0.0.1", port+i);
        }

        // Senders and receivers
        for (int i=0; i < pairs_nb; ++i) {
            start_receiver(&pairs[i]);
            start_sender(&pairs[i]);
        }
        
        while(!stop)
            sleep(3);
        getrusage(RUSAGE_SELF, &ru2);

        uint64_t worst = 0;
        uint64_t best = 1000000000;
        uint64_t sum_avg = 0;
        uint64_t received_messages = 0;
        uint64_t sent_messages = 0;
        uint64_t send_deadline_miss = 0;
        uint64_t recv_deadline_miss = 0;
        uint64_t sum_jitter = 0;
        uint64_t worst_jitter = 0;

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
            uint64_t jitter = (pairs[i].worst_case_delay - pairs[i].best_case_delay)/2;
            sum_jitter += jitter;
            if (jitter > worst_jitter)
                worst_jitter = jitter;
            received_messages += pairs[i].received_messages;
            sent_messages += pairs[i].sent_messages;
            send_deadline_miss += pairs[i].send_th->getDeadlineMiss();
            recv_deadline_miss += pairs[i].recv_th->getDeadlineMiss();
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
        std::cout << "Avg jitter = " << sum_jitter/pairs_nb << " usec" << std::endl;
        std::cout << "Worst jitter = " << worst_jitter << " usec" << std::endl;
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
        std::cout << "Sender deadline misses = " << send_deadline_miss << std::endl;
        std::cout << "Receiver deadline misses = " << recv_deadline_miss << std::endl;
        std::cout << "AAAAAAAAAAAAAAAAAAA" << std::endl;
        std::cout << std::flush;

    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

        
