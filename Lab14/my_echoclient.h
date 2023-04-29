/*
 * Custom Echo Client Class
 * Author: Scott Susanto
 */

#ifndef _MY_ECHOCLIENT_H_
#define _MY_ECHOCLIENT_H_

#include <mutex>
#include <condition_variable>
#include <queue>

using namespace std;

extern mutex mut;
extern condition_variable echo_cv;
extern queue<string> echo_q;

string await_echoed_line();
void send_to_echoclient(string message);

#endif