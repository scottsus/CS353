/*
 * Custom Event CLass
 * Author: Scott Susanto
 */

#include "my_event.h"

Event::Event()
{
    this->event_type = -1;
}

Event::Event(string event_type)
{
    this->event_type = event_type;
}

Event::Event(string event_type, shared_ptr<RDTMessage> rdt_message)
{
    this->event_type = event_type;
    this->rdt_message = rdt_message;
}

string Event::get_event_type()
{
    return event_type;
}

shared_ptr<RDTMessage> Event::get_rdt_message()
{
    return rdt_message;
}