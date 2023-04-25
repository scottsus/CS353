/*
 * Custom Timer Class
 * Author: Scott Susanto
 */
#include <iostream>
#include <unistd.h>

#include "my_timer.h"
#include "my_timestamp.h"

extern mutex mut;
extern condition_variable timer_cv;
extern queue<shared_ptr<Event>> timer_q;
extern bool cancelled;

Timer::Timer()
{
    this->expiration_time = 0;
    gettimeofday(&start_time, NULL);
    this->timer_thread = NULL;
}

Timer::Timer(int expiration_time)
{
    this->expiration_time = expiration_time;
    gettimeofday(&start_time, NULL);
    this->timer_thread = make_shared<thread>(thread(countdown, expiration_time));
    cancelled = false;
}

int Timer::get_expiration_time()
{
    return expiration_time;
}

string Timer::stop()
{
    cancelled = true;
    gettimeofday(&now_time, NULL);
    return str_timestamp_diff_in_seconds(&start_time, &now_time);
}

shared_ptr<thread> Timer::get_timer_thread()
{
    return timer_thread;
}

shared_ptr<Event> Timer::await_timeout()
{
    unique_lock<mutex> lock(mut);
    while (timer_q.empty())
        timer_cv.wait(lock);

    shared_ptr<Event> event = timer_q.front();
    timer_q.pop();
    return event;
}

void countdown(int expiration_time)
{
    int ticks_left = expiration_time * 10;
    while (true)
    {
        usleep(100000);
        mut.lock();
        if (cancelled)
        {

            mut.unlock();
            return;
        }
        mut.unlock();
        ticks_left--;
        if (ticks_left <= 0)
            break;
    }
    mut.lock();
    timer_q.push(make_shared<Event>(2));
    timer_cv.notify_one();
    mut.unlock();
}

void send_to_timer_q(shared_ptr<Event> event)
{
    mut.lock();
    timer_q.push(event);
    timer_cv.notify_one();
    mut.unlock();
}