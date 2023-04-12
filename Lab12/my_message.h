/*
 *  Custom Message Class
 *  Author: Scott Susanto
 */

#ifndef _MY_MESSAGE_H_
#define _MY_MESSAGE_H_

#include <string>
#include <memory>

using namespace std;

class Message
{
private:
    bool ok;
    bool is_http;
    bool should_terminate;
    int content_len;

    int status_code;
    string file_path;
    string md5_hash;

    bool hello;
    int ttl;
    int flood_reason;
    int next_layer;
    string message_id;
    string target_nodeid;
    string sender_nodeid;
    string origin_nodeid;
    string origin_start_time;
    string message_body;

public:
    Message();
    Message(bool should_terminate);
    Message(int status_code, string file_path, string md5_hash);
    Message(string origin_nodeid);
    Message(int ttl, int flood_reason, string sender_nodeid, string origin_nodeid, string message_body, int content_len);
    Message(int ttl, int flood_reason, string message_id, string sender_nodeid, string origin_nodeid, string origin_start_time, string message_body, int content_len);
    Message(string neighbor_nodeid, shared_ptr<Message> old_message);
    Message(int ttl, string src_nodeid, string dest_nodeid, int next_layer, int content_len, string message_body);

    bool is_ok();
    bool is_mode_http();
    bool thread_should_terminate();

    int get_status_code();
    int get_content_len();
    string get_file_path();
    string get_md5_hash();

    bool is_hello();
    int get_ttl();
    int get_flood_reason();
    int get_next_layer();
    string get_message_id();
    string get_target_nodeid();
    string get_sender_nodeid();
    string get_origin_nodeid();
    string get_origin_start_time();
    string get_message_body();

    shared_ptr<Message> decr_ttl_update_sender(string sender_nodeid);
    string to_string();
};

#endif