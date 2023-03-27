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
}

Message::Message(int status_code, string file_path, string md5_hash)
{
    this->status_code = status_code;
    this->file_path = file_path;
    this->md5_hash = md5_hash;
}

int Message::get_status_code()
{
    return status_code;
}

string Message::get_file_path()
{
    return file_path;
}

string Message::get_md5_hash()
{
    return md5_hash;
}
