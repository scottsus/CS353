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

#include "my_connection.h"
#include "my_readwrite.h"

extern map<string, map<string, string>> config;
extern ofstream logfile;

tuple<int, vector<string>> choose_mode(int argc, char *argv[]);
map<string, map<string, string>> get_config(string config_file);
vector<int> get_throttlers(string file_type);
bool contains_complex_chars(string uri);
bool is_digit(string str);
int get_file_size(string file_path);
string calc_md5(string file_path);
string hexDump(unsigned char *buf, unsigned long len);
bool has_active_conns(vector<shared_ptr<Connection>> conns);

void log(string message);
void log_header(string header, int conn_number);
void usage();

#endif
