/*
 *  Server
 */

#ifndef _MY_SERVER_H_
#define _MY_SERVER_H_

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <queue>
#include <condition_variable>

#include "my_connection.h"
#include "my_console.h"
#include "my_neighbors.h"
#include "my_reaper.h"

extern int server_socketfd;
extern int conn_number;
extern const int MAX_CONNECTIONS;

extern map<string, map<string, string>> config;
extern string rootdir;
extern ofstream logfile;

extern vector<shared_ptr<Connection>> conns;

extern mutex mut;
extern queue<shared_ptr<Connection>> reaper_q;
extern condition_variable reaper_cv;

void run_server(string config_file);

void await_request(shared_ptr<Connection> client_conn, vector<shared_ptr<Connection>> *conns);
void processP2PRequest(int client_socketfd, shared_ptr<Connection> client_conn, vector<shared_ptr<Connection>> *conns);
void processHTTPRequest(int client_socketfd, shared_ptr<Connection> client_conn, string method, string uri, string version);

void send_response(string sender_nodeid, shared_ptr<Connection> client_conn);
void write_res_headers(int status_code, shared_ptr<Connection> client_conn, string md5_hash);
void write_res_body(shared_ptr<Connection> client_conn, string file_path);
void write_hello(shared_ptr<Connection> neighbor_conn, string ttl, string flood, string sender_nodeid, int content_len);

#endif
