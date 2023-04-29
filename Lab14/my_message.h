/*
 *  Custom Message Class
 *  Author: Scott Susanto
 */

#ifndef _MY_MESSAGE_H_
#define _MY_MESSAGE_H_

#include <string>
#include <memory>

using namespace std;

class Message;
class RDTMessage;

/* Base Message Class */

class Message
{
protected:
    string message_type;
    int content_len;

public:
    Message();
    Message(string message_type, int content_len);

    string get_message_type();
    int get_content_len();
    bool is_ok();
    string to_string();

    void set_content_len(int content_len);
};

/* HTTP Message */

class HTTPMessage : public Message
{
private:
    int status_code;
    string file_path;
    string md5_hash;

public:
    HTTPMessage();
    HTTPMessage(int status_code, string file_path, int content_len, string md5_hash);

    int get_status_code();
    string get_file_path();
    string get_md5_hash();

    string to_string();
};

/* P2P Message */

class P2PMessage : public Message
{
protected:
    int ttl;
    int flood_reason;
    string messageid;
    string origin_nodeid;
    string message_body;

    string sender_nodeid;

public:
    P2PMessage();
    P2PMessage(string message_type, int ttl, int flood_reason, string origin_nodeid, string message_body);

    int get_ttl();
    int get_flood_reason();
    string get_messageid();
    string get_origin_nodeid();
    string get_message_body();

    string get_sender_nodeid();
    string to_string();

    void set_message_body(string message_body);
    void decr_ttl_update_sender(string sender_nodeid);
};

/* Hello Message */

class HelloMessage : public P2PMessage
{
public:
    HelloMessage();
    HelloMessage(string origin_nodeid);
};

/* LSUPDATE Message */

class LSUPDATEMessage : public P2PMessage
{
protected:
    string origin_starttime;

public:
    LSUPDATEMessage();
    LSUPDATEMessage(int ttl, int flood_reason, string message_id, string origin_nodeid, string origin_starttime, string message_body);

    string get_origin_starttime();
};

/* UCASTAPP Message */

class UCASTAPPMessage : public P2PMessage
{
protected:
    string target_nodeid;
    int next_layer;
    shared_ptr<RDTMessage> rdt_message;

public:
    UCASTAPPMessage();
    UCASTAPPMessage(int ttl, string messageid, string src_nodeid, string dest_nodeid, int next_layer);
    UCASTAPPMessage(int ttl, string messageid, string src_nodeid, string dest_nodeid, int next_layer, string message_body);
    UCASTAPPMessage(int ttl, string messageid, string src_nodeid, string dest_nodeid, int next_layer, int seq_num, int app_num, string rdt_message_body);
    UCASTAPPMessage(shared_ptr<RDTMessage> rdt);

    string get_target_nodeid();
    int get_next_layer();

    shared_ptr<RDTMessage> get_rdt_message();
};

/*
 * RDT Message
 * Can be type RDTDATA or RDTACK
 */

class RDTMessage
{
private:
    string type;

    int seq_num;
    int app_num;
    int rdt_content_len;
    string rdt_message_body;

public:
    RDTMessage(string ucastapp_headers);
    RDTMessage(int seq_num, int app_num, string rdt_message_body);

    string get_type();
    int get_seq_num();
    int get_app_num();
    int get_rdt_content_len();
    string get_rdt_message_body();
    string to_string();
};

string generate_messageid(string origin_nodeid);

#endif