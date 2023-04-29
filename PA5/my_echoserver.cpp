/*
 * Custom EchoServer Class
 * Author: Scott Susanto
 */

#include <iostream>
#include <unistd.h>

#include "my_echoserver.h"
#include "my_console.h"
#include "my_message.h"
#include "my_timer.h"
#include "my_utils.h"

void run_echo_server(string nodeid, vector<shared_ptr<Connection>> *conns)
{
    queue<shared_ptr<thread>> echo_workers{};
    while (true)
    {
        shared_ptr<EchoJob> echojob = await_echojob();
        if (echojob == NULL)
            break;

        mut.lock();
        echo_workers.push(
            make_shared<thread>(
                rdt_send, nodeid, echojob->get_target_nodeid(), echojob->get_message(), echojob->get_app_num(), echojob->get_next_hop_conn(), conns));
        mut.unlock();
    }

    while (true)
    {
        mut.lock();
        if (echo_workers.empty())
        {
            mut.unlock();
            break;
        }

        shared_ptr<thread> echo_worker = echo_workers.front();
        echo_workers.pop();
        echo_worker->join();
        mut.unlock();
    }
}

shared_ptr<EchoJob> await_echojob()
{
    unique_lock<mutex> lock(mut);
    while (echojob_q.empty())
        echojob_cv.wait(lock);

    shared_ptr<EchoJob> echojob = echojob_q.front();
    echojob_q.pop();
    return echojob;
}

void send_to_echoserver(shared_ptr<EchoJob> echojob)
{
    mut.lock();
    echojob_q.push(echojob);
    echojob_cv.notify_one();
    mut.unlock();
}
