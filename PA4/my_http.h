/*
 * HTTP Functions
 * Author: Scott Susanto
 */

#ifndef _MY_HTTP_H_
#define _MY_HTTP_H_

#include "my_connection.h"

using namespace std;

extern string rootdir;

void await_http_request(shared_ptr<Connection> client_conn);
void send_http_response(shared_ptr<Connection> client_conn);
void write_res_headers(int status_code, shared_ptr<Connection> client_conn, string md5_hash);
void write_res_body(shared_ptr<Connection> client_conn, string file_path);

#endif