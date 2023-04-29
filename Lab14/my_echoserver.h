/*
 * Custom EchoServer Class
 * Author: Scott Susanto
 */

#ifndef _MY_ECHOSERVER_H_
#define _MY_ECHOSERVER_H_

#include <mutex>
#include <condition_variable>
#include <queue>

using namespace std;

#include "my_echojob.h"

extern int max_ttl;
extern int msg_lifetime;

extern mutex mut;
extern condition_variable echojob_cv;
extern queue<shared_ptr<EchoJob>> echojob_q;

void run_echo_server(string nodeid, vector<shared_ptr<Connection>> *conns);
shared_ptr<EchoJob> await_echojob();
void send_to_echoserver(shared_ptr<EchoJob> echojob);
void send_echo(shared_ptr<Connection> peer_conn, string src_nodeid, string target_nodeid, int app_num, string message);

#endif
