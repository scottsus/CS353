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
extern queue<shared_ptr<Connection>> reaper_q;
extern condition_variable reaper_cv;
extern int server_socketfd;

void reap_threads(vector<shared_ptr<Connection>> *conns);
void reap_thread(shared_ptr<Connection> conn);
shared_ptr<Connection> wait_to_reap();

#endif