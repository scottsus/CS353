/*
 * Custom EchoJob Class
 * Author: Scott Susanto
 */

#ifndef _MY_ECHOJOB_H_
#define _MY_ECHOJOB_H_

#include <string>
#include <memory>
#include <thread>

#include "my_connection.h"

using namespace std;

class EchoJob
{
private:
    shared_ptr<Connection> next_hop_conn;
    string target_nodeid;
    int app_num;
    string message;
    shared_ptr<thread> echo_thread;

public:
    EchoJob(shared_ptr<Connection> next_hop_conn, string target_nodeid, int app_num, string message);

    shared_ptr<Connection> get_next_hop_conn();
    string get_target_nodeid();
    int get_app_num();
    string get_message();
    shared_ptr<thread> get_echo_thread();

    void set_echo_thread(shared_ptr<thread> echo_thread);
};

#endif