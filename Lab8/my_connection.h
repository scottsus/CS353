/*
 *  Custom Connection Class
 *  Author: Scott Susanto
 */

#ifndef _MY_CONNECTION_H_
#define _MY_CONNECTION_H_

#include <thread>
#include <memory>

using namespace std;

class Connection
{
private:
    int conn_number;
    int curr_socketfd;
    int orig_socketfd;
    int kb_sent;
    shared_ptr<thread> conn_thread;

public:
    Connection();
    Connection(int conn_number, int client_socketfd, shared_ptr<thread> conn_thread);

    int get_conn_number();
    int get_curr_socketfd();
    int get_orig_socketfd();
    int get_kb_sent();
    bool is_alive();
    shared_ptr<thread> get_conn_thread();

    void set_curr_socketfd(int socketfd);
    void incr_kb();
    void set_conn_thread(shared_ptr<thread> conn_thread);
};

#endif