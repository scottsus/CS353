/*
 * Custom Echo Client Class
 * Author: Scott Susanto
 */

#include "my_echoclient.h"

string await_echoed_line()
{
    unique_lock<mutex> lock(mut);
    while (echo_q.empty())
        echo_cv.wait(lock);

    string echoed_line = echo_q.front();
    echo_q.pop();
    return echoed_line;
}

void send_to_echoclient(string message)
{
    mut.lock();
    echo_q.push(message);
    echo_cv.notify_one();
    mut.unlock();
}