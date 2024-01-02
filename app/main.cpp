#include<thread>
#include<chrono>
#include<iostream>
#include<cstdio>
#include<unistd.h>
#include<atomic>
#include<string>
#include<sched.h>
#include<sys/time.h>

#include"Msg.hpp"
#include"Stats.hpp"
#include"Proxy.hpp"
#include"PeriodicThread.hpp"
#include"Skeleton.hpp"

#define timespecsub(tsp, usp, vsp)                          \
    do {                                                    \
        (vsp)->tv_sec = (tsp)->tv_sec - (usp)->tv_sec;      \
        (vsp)->tv_nsec = (tsp)->tv_nsec - (usp)->tv_nsec;   \
        if ((vsp)->tv_nsec < 0) {                           \
            (vsp)->tv_sec--;                                \
            (vsp)->tv_nsec += 1000000000L;                  \
        }                                                   \
    } while (0)

static uint16_t pairs_nb = 200;
static uint64_t period_usec = 1000;
static uint64_t duration_sec = 10;
static uint64_t to_be_sent = 0;

static const uint16_t port = 1236;

Stats *pairs = nullptr;

void do_send(PeriodicThread *th, void* arg)
{
    static std::atomic<int> counter = 0;
    Stats *c = (Stats*) arg;
    Skeleton<int> *skel = c->skel;
    Msg<int> msg;
    msg.data = counter++;
    skel->Send(msg);
    (c->sent_messages)++;
}

// Sender is periodic and sends the message
void* sender (void* arg)
{
    try {
        auto pt = new PeriodicThread(period_usec, do_send, arg);
        if (!pt->set_rt_prio())
            exit(-1);
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return nullptr;
}

void do_receive (PeriodicThread *th, void* arg)
{
    Stats *c = (Stats*) arg;
    Proxy<int> * proxy = c->proxy;
    Msg<int> message = proxy->GetNewSamples();
    Msg<int> *msg = &message;
    printf("Received %d\n", msg->data);
    timespec now, orig, elapsed;
    orig = msg->GetTime();
    printf("Time was: %ld\t%ld\n", orig.tv_sec, orig.tv_nsec);
    clock_gettime(CLOCK_MONOTONIC, &now);
    printf("Time is: %ld\t%ld\n", now.tv_sec, now.tv_nsec);
    timespecsub(&now, &orig, &elapsed);
    printf("Diff is: %ld\t%ld\n", elapsed.tv_sec, elapsed.tv_nsec);
    uint64_t delay = elapsed.tv_nsec;
    printf("Delay: %lu\n", delay);
    if (c->worst_case_delay < delay) 
        c->worst_case_delay = delay;
    if (c->best_case_delay > delay) 
        c->best_case_delay = delay;
    c->sum_delay += delay;
    c->received_messages++;
    if (c->received_messages == to_be_sent)
        th->stop();
}

// Receiver is event-triggered and measures the delay
void * receiver (void* arg)
{
    try {
        auto pt = new PeriodicThread(period_usec, do_receive, arg);
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return nullptr;
}

int main (int argc, char* argv[])
{
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
        to_be_sent = (duration_sec*1000*1000)/period_usec;
        std::cout << "Pairs = " << pairs_nb << std::endl;
        std::cout << "Period = " << period_usec << std::endl;
        std::cout << "Duration = " << duration_sec << std::endl;
        std::cout << "To be sent = " << to_be_sent << std::endl;

        pairs = new Stats[pairs_nb];
        
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
        std::cout << "Received messages = " << received_messages << std::endl;
        std::cout << "Sent messages = " << sent_messages << std::endl;
        std::cout << "Avg delay = " << sum_avg/pairs_nb << " usec" << std::endl;
        std::cout << "Worst delay = " << worst << " usec" << std::endl;
        std::cout << "Best delay = " << best << " usec" << std::endl;
        std::cout << "Max jitter = " << (worst-best)/2 << " usec" << std::endl;
        std::cout << "AAAAAAAAAAAAAAAAAAA" << std::endl;

        delete pairs;
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

        
