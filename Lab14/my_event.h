/*
 * Custom Event Class
 * Author: Scott Susanto
 */

#ifndef _MY_EVENT_H_
#define _MY_EVENT_H_

#include <string>
#include <memory>

using namespace std;

#include "my_message.h"

class Event
{
private:
    string event_type;
    shared_ptr<RDTMessage> rdt_message;

public:
    Event();
    Event(string event_type);
    Event(string event_type, shared_ptr<RDTMessage> incoming_rdt); // for RDTACK events

    string get_event_type();
    shared_ptr<RDTMessage> get_rdt_message();
};

#endif