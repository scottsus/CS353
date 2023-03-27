/*
 *  Server
 */

#ifndef _MY_SERVER_H_
#define _MY_SERVER_H_

#include <string>
#include <vector>
#include <memory>
#include <queue>
#include <condition_variable>

#include "my_connection.h"

extern int server_socketfd;
extern const int MAX_CONNECTIONS;

extern map<string, map<string, string>> config;
extern string rootdir;
extern ofstream logfile;

extern vector<shared_ptr<Connection>> conns;

extern mutex mut;
extern queue<shared_ptr<Connection>> reaper_q;
extern condition_variable reaper_cv;

void run_server(string config_file);
void run_server(string port, string logfile);
void handle_console(vector<shared_ptr<Connection>> *conns);
void reap_threads(vector<shared_ptr<Connection>> *conns);
void receive_request(shared_ptr<Connection> client_conn);
void send_response(shared_ptr<Connection> client_conn);
tuple<int, int, string> process_request(int client_socketfd, shared_ptr<Connection> client_conn);
void write_res_headers(int status_code, shared_ptr<Connection> client_conn, string md5_hash);
void write_res_body(shared_ptr<Connection> client_conn, string file_path);

#endif
