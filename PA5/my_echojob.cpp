/*
 * Custom EchoJob Class
 * Author: Scott Susanto
 */

#include "my_echojob.h"

EchoJob::EchoJob(shared_ptr<Connection> next_hop_conn, string target_nodeid, int app_num, string message)
{
    this->next_hop_conn = next_hop_conn;
    this->target_nodeid = target_nodeid;
    this->app_num = app_num;
    this->message = message;
    this->echo_thread = NULL;
}

shared_ptr<Connection> EchoJob::get_next_hop_conn()
{
    return next_hop_conn;
}

string EchoJob::get_target_nodeid()
{
    return target_nodeid;
}

int EchoJob::get_app_num()
{
    return app_num;
}

string EchoJob::get_message()
{
    return message;
}

shared_ptr<thread> EchoJob::get_echo_thread()
{
    return echo_thread;
}

void EchoJob::set_echo_thread(shared_ptr<thread> echo_thread)
{
    this->echo_thread = echo_thread;
}
