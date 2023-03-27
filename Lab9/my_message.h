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
    string file_path;
    string md5_hash;

public:
    Message();
    Message(int status_code, string file_path, string md5_hash);

    int get_status_code();
    string get_file_path();
    string get_md5_hash();
};

#endif