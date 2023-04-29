/*
 * Custom Reaper Class
 * Author: Scott Susanto
 */

#ifndef _MY_REAPER_H_
#define _MY_REAPER_H_

#include <queue>
#include <mutex>
#include <condition_variable>

using namespace std;

#include "my_connection.h"

extern mutex mut;
extern condition_variable reaper_cv;
extern queue<shared_ptr<Connection>> reaper_q;
extern int server_socketfd;

void reap_threads(vector<shared_ptr<Connection>> *conns);
shared_ptr<Connection> await_to_reap();
void send_to_reaper(shared_ptr<Connection> conn);

#endif