/*
 *  Custom Message Class
 *  Author: Scott Susanto
 */

#include "my_message.h"

Message::Message()
{
    this->status_code = 501;
    this->file_path = "";
    this->md5_hash = "";

    this->sender_nodeid = "";
    this->content_len = 0;
}

Message::Message(int status_code, string file_path, string md5_hash)
{
    this->status_code = status_code;
    this->file_path = file_path;
    this->md5_hash = md5_hash;

    this->is_http = true;
    this->sender_nodeid = "";
}

Message::Message(string sender_nodeid, int content_len)
{
    this->status_code = 0;
    this->file_path = "";
    this->md5_hash = "";

    this->is_http = false;
    this->sender_nodeid = sender_nodeid;
    this->content_len = content_len;
}

int Message::get_status_code()
{
    return status_code;
}

int Message::get_content_len()
{
    return content_len;
}

string Message::get_file_path()
{
    return file_path;
}

string Message::get_md5_hash()
{
    return md5_hash;
}

bool Message::is_mode_http()
{
    return is_http;
}

string Message::get_nodeid()
{
    return sender_nodeid;
}
