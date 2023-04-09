/*
 *  Custom Connection Class
 *  Author: Scott Susanto
 */
#include <iostream>
#include <mutex>

#include "my_connection.h"

Connection::Connection()
{
    this->conn_number = -1;
    this->curr_socketfd = -1;
    this->orig_socketfd = -1;
}

Connection::Connection(int conn_number, int client_socketfd)
{
    this->conn_number = conn_number;
    this->curr_socketfd = client_socketfd;
    this->orig_socketfd = client_socketfd;
    this->neighbor_nodeid = "";

    this->content_len = 0;
    this->kb_sent = 0;
    this->reason = "";

    this->mut = make_shared<mutex>();
    this->cv = make_shared<condition_variable>();
    this->q = queue<shared_ptr<Message>>();
}

Connection::Connection(int conn_number, int client_socketfd, string nodeid)
{
    this->conn_number = conn_number;
    this->curr_socketfd = client_socketfd;
    this->orig_socketfd = client_socketfd;
    this->neighbor_nodeid = nodeid;

    this->content_len = 0;
    this->kb_sent = 0;
    this->reason = "";

    this->mut = make_shared<mutex>();
    this->cv = make_shared<condition_variable>();
    this->q = queue<shared_ptr<Message>>();
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

string Connection::get_neighbor_nodeid()
{
    return neighbor_nodeid;
}

bool Connection::is_alive()
{
    return curr_socketfd > -1;
}

shared_ptr<thread> Connection::get_reader_thread()
{
    return reader_thread;
}

shared_ptr<thread> Connection::get_writer_thread()
{
    return writer_thread;
}

shared_ptr<Message> Connection::await_msg_from_queue()
{
    unique_lock<mutex> lock(*mut);
    while (q.empty())
        cv->wait(lock);

    shared_ptr<Message> msg = q.front();
    q.pop();
    return msg;
}

int Connection::get_content_len()
{
    return content_len;
}

string Connection::get_ip_port()
{
    return get_ip_and_port_for_client(orig_socketfd, 1);
}

string Connection::get_uri()
{
    return uri;
}

string Connection::get_file_type()
{
    return file_type;
}

string Connection::get_start_time()
{
    return format_timestamp(&start_time);
}

string Connection::get_time_elapsed()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    double diff = timestamp_diff_in_seconds(&start_time, &now);

    ostringstream oss;
    oss << fixed << setprecision(3) << diff;
    return oss.str();
}

int Connection::get_P()
{
    return throttlers.at(0);
}

int Connection::get_MAXR()
{
    return throttlers.at(1);
}

int Connection::get_DIAL()
{
    return throttlers.at(2);
}

double Connection::get_speed()
{
    return (get_MAXR() * 1.0 / get_P()) * (get_DIAL() * 1.0 / 100);
}

string Connection::get_speed_str()
{
    ostringstream oss;
    oss << fixed << setprecision(3) << get_speed();
    return oss.str();
}

string Connection::get_shaper_params()
{
    string shaper_params = "";
    shaper_params += "P=" + to_string(get_P());
    shaper_params += ", MAXR=" + to_string(get_MAXR());
    shaper_params += " tokens/s, DIAL=" + to_string(get_DIAL());
    shaper_params += ", rate=" + get_speed_str() + " KB/s";
    return shaper_params;
}

int Connection::get_kb_sent()
{
    return kb_sent;
}

string Connection::get_kb_percent_sent()
{
    double percent_sent = (kb_sent * 1000.0 / content_len) * 100;
    if (percent_sent >= 100)
        return "(100%)";

    ostringstream oss;
    oss << fixed << setprecision(1) << percent_sent;
    return "(" + oss.str() + "%)";
}

string Connection::get_reason()
{
    return reason;
}

void Connection::set_curr_socketfd(int socketfd)
{
    this->curr_socketfd = socketfd;
}

void Connection::set_neighbor_nodeid(string nodeid)
{
    this->neighbor_nodeid = nodeid;
}

void Connection::set_reader_thread(shared_ptr<thread> reader_thread)
{
    this->reader_thread = reader_thread;
}

void Connection::set_writer_thread(shared_ptr<thread> writer_thread)
{
    this->writer_thread = writer_thread;
}

void Connection::add_msg_to_queue(shared_ptr<Message> msg)
{
    mut->lock();
    q.push(msg);
    cv->notify_one();
    mut->unlock();
}

void Connection::lock()
{
    mut->lock();
}

void Connection::unlock()
{
    mut->unlock();
}

void Connection::set_content_len(int content_len)
{
    this->content_len = content_len;
}

void Connection::set_uri(string uri)
{
    this->uri = uri;
}

void Connection::set_file_type(string file_type)
{
    this->file_type = file_type;
}

void Connection::set_start_time()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    this->start_time = now;
}

void Connection::set_throttlers(vector<int> throttlers)
{
    this->throttlers = throttlers;
}

void Connection::set_dial(int percent)
{
    this->throttlers[2] = percent;
}

void Connection::incr_kb()
{
    this->kb_sent = this->kb_sent + 1;
}

void Connection::reset_kb()
{
    this->kb_sent = 0;
}

void Connection::set_reason(string reason)
{
    this->reason = reason;
}