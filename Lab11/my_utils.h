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
#include "my_readwrite.h"

extern int mode;
extern map<string, map<string, string>> config;
extern ofstream logfile;
extern mutex mut;

tuple<int, vector<string>> choose_mode(int argc, char *argv[]);
map<string, map<string, string>> get_config(string config_file);
vector<int> get_throttlers(string file_type);

shared_ptr<Connection> find_conn(int conn_number, vector<shared_ptr<Connection>> *conns);
void remove_from_vector(string target, vector<string> &vec);

bool contains_complex_chars(string uri);
int get_file_size(string file_path);
string calc_md5(string file_path);
string hexDump(unsigned char *buf, unsigned long len);

void log(string message);
void log_header(string header, int conn_number);
void log_header(string type, string nodeid, int TTL, int flood, int content_len);
void usage();

#endif
