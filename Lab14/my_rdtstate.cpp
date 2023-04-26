/*
 * Custom RDT3.0 State Class
 * Author: Scott Susanto
 */

#include "my_rdtstate.h"

RDTState::RDTState()
{
    this->peer_nodeid = "";
    this->seq_num = 0;
    this->app_num = 0;
    this->sndpkt = "";
    this->message_received = "";
}

RDTState::RDTState(string peer_nodeid, int seq_num, int app_num, string sndpkt, string message_received)
{
    this->peer_nodeid = peer_nodeid;
    this->seq_num = seq_num;
    this->app_num = app_num;
    this->sndpkt = sndpkt;
    this->message_received = message_received;
}

string RDTState::get_peer_nodeid()
{
    return peer_nodeid;
}

int RDTState::get_seq_num()
{
    return seq_num;
}

int RDTState::get_app_num()
{
    return app_num;
}

string RDTState::get_sndpkt()
{
    return sndpkt;
}

string RDTState::get_message_received()
{
    return message_received;
}
