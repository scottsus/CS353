/*
 * Custom Event Class
 * Author: Scott Susanto
 */

#ifndef _MY_EVENT_H_
#define _MY_EVENT_H_

class Event
{
private:
    int event_type; // 0 for ttl_zero, 1 for PONG, 2 for message lifetime

public:
    Event();
    Event(int event_type);

    int get_event_type();
};

#endif