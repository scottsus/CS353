/*
 * Custom Timer Class
 * Author: Scott Susanto
 */

#ifndef _MY_TIMER_H_
#define _MY_TIMER_H_

#include <memory>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <sys/time.h>

#include "my_event.h"

using namespace std;

class Timer
{
private:
    int expiration_time;
    struct timeval start_time;
    struct timeval now_time;
    shared_ptr<thread> timer_thread;

public:
    Timer();
    Timer(int expiration_time);
    int get_expiration_time();
    shared_ptr<thread> get_timer_thread();

    string stop();
    shared_ptr<Event> await_timeout();
};

void countdown(int expiration_time);
void send_to_timer_q(shared_ptr<Event> event);

#endif