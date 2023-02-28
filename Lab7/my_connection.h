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
    int client_socketfd;
    shared_ptr<thread> conn_thread;

public:
    Connection();
    Connection(int conn_number, int client_socketfd, shared_ptr<thread> conn_thread);
    int get_conn_number();
    int get_client_socketfd();
    bool is_alive();
    void set_client_socketfd(int socketfd);
    shared_ptr<thread> get_conn_thread();
    void set_conn_thread(shared_ptr<thread> conn_thread);
};

#endif