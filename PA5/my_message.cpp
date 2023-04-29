/*
 *  Custom Message Class
 *  Author: Scott Susanto
 */

#include <iostream>

#include "my_message.h"
#include "my_utils.h"

/* General Message */

Message::Message()
{
    this->message_type = "";
    this->content_len = 0;
}

Message::Message(string message_type, int content_len)
{
    this->message_type = message_type;
    this->content_len = content_len;
}

string Message::get_message_type()
{
    return message_type;
}

int Message::get_content_len()
{
    return content_len;
}

bool Message::is_ok()
{
    return message_type != "";
}

string Message::to_string()
{
    string result = "";
    result += "Message Type: " + message_type + "\r\n";
    result += "Content Length: " + std::to_string(content_len) + "\r\n";
    return result;
}

void Message::set_content_len(int content_len)
{
    this->content_len = content_len;
}

/* HTTP Message */

HTTPMessage::HTTPMessage()
    : Message("", 0)
{
    this->status_code = 0;
    this->file_path = "";
    this->md5_hash = "";
}

HTTPMessage::HTTPMessage(int status_code, string file_path, int content_len, string md5_hash)
    : Message("HTTP", content_len)
{
    this->status_code = status_code;
    this->file_path = file_path;
    this->md5_hash = md5_hash;
}

int HTTPMessage::get_status_code()
{
    return status_code;
}

string HTTPMessage::get_file_path()
{
    return file_path;
}

string HTTPMessage::get_md5_hash()
{
    return md5_hash;
}

string HTTPMessage::to_string()
{
    string result = Message::to_string();
    result += "HTTP/1.1 " + std::to_string(status_code) + " OK\r";
    result += "URI: " + file_path + "\r";
    result += "Content Length: " + std::to_string(content_len) + "\r";
    result += "MD5: " + md5_hash + "\r";
    return result;
}

/* General P2P Message */

P2PMessage::P2PMessage()
    : Message("", 0)
{
    this->ttl = 0;
    this->flood_reason = 0;
    this->messageid = "";
    this->origin_nodeid = "";
    this->content_len = 0;
    this->message_body = "";
    this->sender_nodeid = "";
}

P2PMessage::P2PMessage(string message_type, int ttl, int flood_reason, string origin_nodeid, string message_body)
    : Message(message_type, message_body.length())
{
    this->ttl = ttl;
    this->flood_reason = flood_reason;
    this->messageid = generate_messageid(origin_nodeid);
    this->origin_nodeid = origin_nodeid;
    this->content_len = message_body.length();
    this->message_body = message_body;
    this->sender_nodeid = origin_nodeid;
}

int P2PMessage::get_ttl()
{
    return ttl;
}

int P2PMessage::get_flood_reason()
{
    return flood_reason;
}

string P2PMessage::get_messageid()
{
    return messageid;
}

string P2PMessage::get_origin_nodeid()
{
    return origin_nodeid;
}

string P2PMessage::get_message_body()
{
    return message_body;
}

string P2PMessage::get_sender_nodeid()
{
    return sender_nodeid;
}

string P2PMessage::to_string()
{
    string result = Message::to_string();
    result += "TTL: " + std::to_string(ttl) + "\r";
    result += "Flood: " + std::to_string(flood_reason) + "\r";
    result += "Message ID: " + messageid + "\r";
    result += "Origin Node ID: " + origin_nodeid + "\r";
    result += "Content Length: " + std::to_string(content_len) + "\r";
    result += "Message Body: " + message_body + "\r";
    return result;
}

void P2PMessage::set_message_body(string message_body)
{
    this->message_body = message_body;
}

void P2PMessage::decr_ttl_update_sender(string sender_nodeid)
{
    this->ttl = ttl - 1;
    this->sender_nodeid = sender_nodeid;
}

/* P2P HelloMessage */

HelloMessage::HelloMessage()
    : P2PMessage()
{
    this->origin_nodeid = "";
}

HelloMessage::HelloMessage(string origin_nodeid)
    : P2PMessage("HELLO", 0, 0, origin_nodeid, "")
{
}

/* LSUPDATEMessage */

LSUPDATEMessage::LSUPDATEMessage()
    : P2PMessage()
{
    this->origin_starttime = "";
}

LSUPDATEMessage::LSUPDATEMessage(int ttl, int flood_reason, string messageid, string origin_nodeid, string origin_starttime, string message_body)
    : P2PMessage("LSUPDATE", ttl, flood_reason, origin_nodeid, message_body)
{
    this->origin_starttime = origin_starttime;
    if (messageid == "")
        this->messageid = generate_messageid(origin_nodeid);
    else
        this->messageid = messageid;
}

string LSUPDATEMessage::get_origin_starttime()
{
    return origin_starttime;
}

/* UCASTAPP Message */

UCASTAPPMessage::UCASTAPPMessage()
    : P2PMessage()
{
    this->target_nodeid = "";
    this->next_layer = 0;
}

UCASTAPPMessage::UCASTAPPMessage(int ttl, string messageid, string src_nodeid, string dest_nodeid, int next_layer)
    : P2PMessage("UCASTAPP", ttl, 0, src_nodeid, "")
{
    this->target_nodeid = dest_nodeid;
    this->next_layer = next_layer;

    if (messageid == "")
        this->messageid = generate_messageid(src_nodeid);
    else
        this->messageid = messageid;
}

UCASTAPPMessage::UCASTAPPMessage(int ttl, string messageid, string src_nodeid, string dest_nodeid, int next_layer, string message_body)
    : UCASTAPPMessage(ttl, messageid, src_nodeid, dest_nodeid, next_layer)
{
    this->rdt_message = make_shared<RDTMessage>(RDTMessage(message_body));
    this->content_len = message_body.length();
    this->message_body = message_body;
}

UCASTAPPMessage::UCASTAPPMessage(int ttl, string messageid, string src_nodeid, string dest_nodeid, int next_layer, int seq_num, int app_num, string rdt_message_body)
    : UCASTAPPMessage(ttl, messageid, src_nodeid, dest_nodeid, next_layer)
{
    this->rdt_message = make_shared<RDTMessage>(RDTMessage(seq_num, app_num, rdt_message_body));
    this->content_len = rdt_message->to_string().length();
    this->message_body = rdt_message->to_string();
}

UCASTAPPMessage::UCASTAPPMessage(shared_ptr<RDTMessage> rdt)
{
    this->rdt_message = rdt;
}

string UCASTAPPMessage::get_target_nodeid()
{
    return target_nodeid;
}

int UCASTAPPMessage::get_next_layer()
{
    return next_layer;
}

shared_ptr<RDTMessage> UCASTAPPMessage::get_rdt_message()
{
    return rdt_message;
}

RDTMessage::RDTMessage(string ucastapp_headers)
{
    this->type = "";
    this->seq_num = 0;
    this->app_num = 0;
    this->rdt_content_len = 0;
    this->rdt_message_body = "";

    stringstream ss(ucastapp_headers);
    string protocol_line;
    getline(ss, protocol_line, '\n');
    if (protocol_line == "353UDT/1.0 RDTDATA\r")
    {
        this->type = "RDTDATA";

        string seq_num_line;
        getline(ss, seq_num_line, '\n');
        int seq_num = stoi(extract_header_value(seq_num_line));

        string app_num_line;
        getline(ss, app_num_line, '\n');
        int app_num = stoi(extract_header_value(app_num_line));

        string rdt_content_len_line;
        getline(ss, rdt_content_len_line, '\n');
        int rdt_content_len = stoi(extract_header_value(rdt_content_len_line));

        string empty_line;
        getline(ss, empty_line, '\n');

        string rdt_message_body;
        getline(ss, rdt_message_body, '\n');
        if (rdt_message_body == "")
            rdt_message_body = "\\n";

        this->seq_num = seq_num;
        this->app_num = app_num;
        this->rdt_content_len = rdt_content_len;
        this->rdt_message_body = rdt_message_body;
    }
    else if (protocol_line.substr(0, 17) == "353UDT/1.0 RDTACK")
    {
        this->type = "RDTACK";

        stringstream ss(protocol_line);
        string version, message_type, seq_num_str, app_num_str;
        ss >> version >> message_type >> seq_num_str >> app_num_str;

        int seq_num = stoi(seq_num_str);
        int app_num = stoi(app_num_str);

        this->seq_num = seq_num;
        this->app_num = app_num;
        this->rdt_content_len = 0;
        this->rdt_message_body = "";
    }
}

RDTMessage::RDTMessage(int seq_num, int app_num, string rdt_message_body)
{
    this->type = "RDTDATA";
    if (rdt_message_body == "")
        this->type = "RDTACK";

    this->seq_num = seq_num;
    this->app_num = app_num;
    this->rdt_content_len = rdt_message_body.length();
    this->rdt_message_body = rdt_message_body;
}

string RDTMessage::get_type()
{
    return type;
}

int RDTMessage::get_seq_num()
{
    return seq_num;
}

int RDTMessage::get_app_num()
{
    return app_num;
}

int RDTMessage::get_rdt_content_len()
{
    return rdt_content_len;
}

string RDTMessage::get_rdt_message_body()
{
    return rdt_message_body;
}

string RDTMessage::to_string()
{
    string result = "";

    if (type == "RDTDATA")
    {
        result += "353UDT/1.0 RDTDATA\r\n";
        result += "Seq-Number: " + std::to_string(seq_num) + "\r\n";
        result += "RDT-App-Number: " + std::to_string(app_num) + "\r\n";
        result += "RDT-Content-Length: " + std::to_string(rdt_content_len) + "\r\n";
        result += "\r\n";
        result += rdt_message_body;
    }
    else if (type == "RDTACK")
    {
        result += "353UDT/1.0 RDTACK " + std::to_string(seq_num) + " " + std::to_string(app_num) + "\r\n";
    }

    return result;
}

string generate_messageid(string origin_nodeid)
{
    string messageid, origin_starttime;
    get_message_id_and_start_time(origin_nodeid, "msg", messageid, origin_starttime);

    return messageid;
}

// Message::Message(int ttl, int flood_reason, string sender_nodeid, string origin_nodeid, string message_body, int content_len)
// {
//     this->ok = true;
//     this->is_http = false;
//     this->should_terminate = false;
//     this->content_len = content_len;

//     this->status_code = 0;
//     this->file_path = "";
//     this->md5_hash = "";

//     this->hello = false;
//     this->ttl = ttl;
//     this->flood_reason = flood_reason;
//     this->target_nodeid = "";
//     this->sender_nodeid = sender_nodeid;
//     this->origin_nodeid = origin_nodeid;
//     this->message_body = message_body;

//     string message_id, origin_start_time;
//     get_message_id_and_start_time(origin_nodeid, "msg", message_id, origin_start_time);
//     this->message_id = message_id;
//     this->origin_start_time = origin_start_time;
// }

// Message::Message(string neighbor_nodeid, shared_ptr<Message> old_message)
// {
//     this->ok = true;
//     this->is_http = false;
//     this->should_terminate = false;
//     this->content_len = 0;

//     this->status_code = 0;
//     this->file_path = "";
//     this->md5_hash = "";
//     this->target_nodeid = "";

//     this->hello = false;
//     this->ttl = old_message->get_ttl();
//     this->flood_reason = old_message->get_flood_reason();
//     this->message_id = old_message->get_message_id();
//     this->origin_nodeid = neighbor_nodeid;
//     this->origin_start_time = old_message->get_origin_start_time();
//     this->message_body = old_message->get_message_body();
// }
