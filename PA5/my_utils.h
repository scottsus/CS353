/*
 *  Utility functions
 */

#ifndef _MY_UTILS_H
#define _MY_UTILS_H

#include <string>
#include <tuple>
#include <vector>
#include <map>
#include <fstream>
#include <mutex>

#include "my_connection.h"
#include "my_event.h"
#include "my_node.h"
#include "my_readwrite.h"

extern int mode;
extern map<string, map<string, string>> config;

extern ofstream logfile;
extern int SAYHELLO;
extern int LSUPDATE;
extern int UCASTAPP;

extern mutex mut;
extern map<string, string> graph;

tuple<int, vector<string>> choose_mode(int argc, char *argv[]);
map<string, map<string, string>> get_config(string config_file);
vector<int> get_throttlers(string file_type);

void BFS(shared_ptr<Node> start_node, map<string, map<string, shared_ptr<Node>>> adj_list, map<string, shared_ptr<Node>> *nodes);
map<string, shared_ptr<Node>> get_forwarding_table(string nodeid, vector<shared_ptr<Connection>> *conns);

shared_ptr<Connection> find_conn(int conn_number, vector<shared_ptr<Connection>> *conns);
shared_ptr<Connection> find_conn(string target_nodeid, vector<shared_ptr<Connection>> *conns);

bool neighbor_is_up(vector<shared_ptr<Connection>> *conns, string nodeid);
void remove_from_vector(string target, vector<string> &vec);
void update_graph(string removed_nodeid, vector<shared_ptr<Connection>> *conns);

void get_message_id_and_start_time(string nodeid, const char *obj_category, string &hexstring_of_unique_obj_id, string &origin_start_time);
shared_ptr<P2PMessage> await_message(int peer_socketfd, shared_ptr<Connection> conn);

bool is_neighbor(string neighbor_nodeid, vector<shared_ptr<Connection>> *conns);
string get_neighbors(vector<shared_ptr<Connection>> *conns);

bool contains_complex_chars(string uri);
int get_file_size(string file_path);
pair<string, string> str_split(string str, char delimeter);
string extract_header_value(string header);
string calc_md5(string file_path);
string hexDump(unsigned char *buf, unsigned long len);

void log(string message);
void log_header(string header, int conn_number);
void log_hello(string type, shared_ptr<HelloMessage> hello);
void log_LSUPDATE(string type, shared_ptr<LSUPDATEMessage> lsupdate);
void log_UCASTAPP(string type, shared_ptr<UCASTAPPMessage> ucastapp);
void usage();

#endif
