/*
 * Custom Event Class
 * Author: Scott Susanto
 */

#ifndef _MY_EVENT_H_
#define _MY_EVENT_H_

#include <string>

using namespace std;

class Event
{
private:
    string event_type; // 0 for ttl_zero, 1 for PONG, 2 for message lifetime

public:
    Event();
    Event(string event_type);

    string get_event_type();
};

#endif