/*
 * P2P Functions
 * Author: Scott Susanto
 */

#ifndef _MY_P2P_H_
#define _MY_P2P_H_

#include "my_connection.h"
#include "my_message.h"
#include "my_rdtstate.h"
#include "my_utils.h"

using namespace std;

extern int server_socketfd;
extern int max_ttl;

extern mutex mut;
extern condition_variable timer_cv;
extern queue<shared_ptr<Event>> timer_q;
extern map<string, shared_ptr<Message>> message_cache;
extern string session_id;
extern map<string, map<string, shared_ptr<RDTState>>> sessions;
extern bool cancelled;

void await_p2p_request(string nodeid, int neighbor_socketfd, shared_ptr<Connection> conn, vector<shared_ptr<Connection>> *conns);
void send_p2p_response(string nodeid, shared_ptr<Connection> neighbor_conn);
void send_lsupdate_to_writer(string nodeid, vector<shared_ptr<Connection>> *conns, int reason);
void write_hello(shared_ptr<Connection> neighbor_conn, shared_ptr<HelloMessage> hello);
void write_LSUPDATE(shared_ptr<Connection> neighbor_conn, shared_ptr<LSUPDATEMessage> lsupdate);
void write_UCASTAPP(shared_ptr<Connection> conn, shared_ptr<UCASTAPPMessage> data_packet);

#endif