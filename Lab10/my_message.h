/*
 *  Custom Message Class
 *  Author: Scott Susanto
 */

#ifndef _MY_MESSAGE_H_
#define _MY_MESSAGE_H_

#include <string>

using namespace std;

class Message
{
private:
    int status_code;
    int content_len;
    string file_path;
    string md5_hash;

    bool is_http;
    string sender_nodeid;

public:
    Message();
    Message(int status_code, string file_path, string md5_hash);
    Message(string sender_nodeid, int content_len);

    int get_status_code();
    int get_content_len();
    string get_file_path();
    string get_md5_hash();

    bool is_mode_http();
    string get_nodeid();
};

#endif