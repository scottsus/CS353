/*
 *  Client
 */

#ifndef _MY_CLIENT_H_
#define _MY_CLIENT_H_

#include <string>
#include <vector>
#include <tuple>

using namespace std;

void run_client(vector<string> client_args);
void write_req_headers(int client_socketfd, string host, string port, string method, string uri);
tuple<int, string> parse_res_headers_and_get_content_len(int client_socketfd, string client_ip_and_port, string uri);
int write_data_to_file(int client_socketfd, int bytes_expected, string filename);

#endif
