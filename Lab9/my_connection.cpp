/*
 *  Custom Connection Class
 *  Author: Scott Susanto
 */

#include "my_connection.h"

Connection::Connection()
{
    this->conn_number = -1;
    this->curr_socketfd = -1;
    this->orig_socketfd = -1;
    this->kb_sent = 0;
    this->conn_thread = NULL;
}

Connection::Connection(int conn_number, int client_socketfd, shared_ptr<thread> conn_thread)
{
    this->conn_number = conn_number;
    this->curr_socketfd = client_socketfd;
    this->orig_socketfd = client_socketfd;
    this->kb_sent = 0;
    this->conn_thread = conn_thread;
}

int Connection::get_conn_number()
{
    return conn_number;
}

int Connection::get_curr_socketfd()
{
    return curr_socketfd;
}

int Connection::get_orig_socketfd()
{
    return orig_socketfd;
}

int Connection::get_kb_sent()
{
    return kb_sent;
}

bool Connection::is_alive()
{
    return curr_socketfd > -1;
}

void Connection::set_curr_socketfd(int socketfd)
{
    this->curr_socketfd = socketfd;
}

void Connection::incr_kb()
{
    this->kb_sent = this->kb_sent + 1;
}

shared_ptr<thread> Connection::get_conn_thread()
{
    return conn_thread;
}

void Connection::set_conn_thread(shared_ptr<thread> conn_thread)
{
    this->conn_thread = conn_thread;
}
