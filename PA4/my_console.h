/*
 * Console
 */

#ifndef _MY_CONSOLE_H_
#define _MY_CONSOLE_H_

#include <string>
#include <vector>
#include <memory>
#include <map>

#include "my_connection.h"

using namespace std;

extern int server_socketfd;
extern mutex mut;
extern queue<shared_ptr<Connection>> reaper_q;
extern condition_variable reaper_cv;
extern map<string, string> graph;

void handle_p2p_console(string nodeid, vector<shared_ptr<Connection>> *conns);
void handle_http_console(vector<shared_ptr<Connection>> *conns);
bool has_active_conns(vector<shared_ptr<Connection>> conns);
bool is_digit(string str);

#endif