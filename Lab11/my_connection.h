/*
 *  Custom Connection Class
 *  Author: Scott Susanto
 */

#ifndef _MY_CONNECTION_H_
#define _MY_CONNECTION_H_

#include <thread>
#include <memory>
#include <vector>
#include <queue>
#include <iomanip>
#include <sstream>
#include <condition_variable>

#include <sys/time.h>

using namespace std;

#include "my_socket.h"
#include "my_timestamp.h"
#include "my_message.h"

class Connection
{
private:
    int conn_number;
    int curr_socketfd;
    int orig_socketfd;

    string neighbor_nodeid;

    shared_ptr<thread> reader_thread;
    shared_ptr<thread> writer_thread;

    shared_ptr<mutex> mut;
    shared_ptr<condition_variable> cv;
    queue<shared_ptr<Message>> q{};

    int content_len;
    string uri;
    string file_type;
    timeval start_time;
    vector<int> throttlers;

    int kb_sent;
    string reason;

public:
    Connection();
    Connection(int conn_number, int client_socketfd);
    Connection(int conn_number, int client_socketfd, string nodeid);

    int get_conn_number();
    int get_curr_socketfd();
    int get_orig_socketfd();
    bool is_alive();

    string get_neighbor_nodeid();

    shared_ptr<thread> get_reader_thread();
    shared_ptr<thread> get_writer_thread();
    shared_ptr<Message> await_message_from_queue();

    int get_content_len();
    string get_ip_port();
    string get_uri();
    string get_file_type();
    string get_start_time();
    string get_time_elapsed();

    int get_P();
    int get_MAXR();
    int get_DIAL();
    double get_speed();
    string get_speed_str();
    string get_shaper_params();

    int get_kb_sent();
    string get_kb_percent_sent();
    string get_reason();

    void set_curr_socketfd(int socketfd);
    void set_neighbor_nodeid(string nodeid);

    void set_reader_thread(shared_ptr<thread> reader_thread);
    void set_writer_thread(shared_ptr<thread> writer_thread);
    void add_message_to_queue(shared_ptr<Message> message);

    void lock();
    void unlock();

    void set_content_len(int content_len);
    void set_uri(string uri);
    void set_file_type(string file_type);
    void set_start_time();

    void set_throttlers(vector<int> throttlers);
    void set_dial(int percent);

    void incr_kb();
    void reset_kb();
    void set_reason(string reason);
};

#endif