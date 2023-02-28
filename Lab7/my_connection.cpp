/*
 *  Custom Connection Class
 *  Author: Scott Susanto
 */

#include "my_connection.h"

Connection::Connection()
{
    this->conn_number = -1;
    this->client_socketfd = -1;
    this->conn_thread = NULL;
}

Connection::Connection(int conn_number, int client_socketfd, shared_ptr<thread> conn_thread)
{
    this->conn_number = conn_number;
    this->client_socketfd = client_socketfd;
    this->conn_thread = conn_thread;
}

int Connection::get_conn_number()
{
    return conn_number;
}

int Connection::get_client_socketfd()
{
    return client_socketfd;
}

bool Connection::is_alive()
{
    return client_socketfd != -1;
}

void Connection::set_client_socketfd(int socketfd)
{
    this->client_socketfd = socketfd;
}

shared_ptr<thread> Connection::get_conn_thread()
{
    return conn_thread;
}

void Connection::set_conn_thread(shared_ptr<thread> conn_thread)
{
    this->conn_thread = conn_thread;
}
