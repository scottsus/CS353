/*
 * Custom RDT3.0 State Class
 * Author: Scott Susanto
 */

#ifndef _MY_RDTSTATE_H_
#define _MY_RDTSTATE_H_

#include <string>

using namespace std;

class RDTState
{
public:
    RDTState();
    RDTState(string peer_nodeid, int seq_num, int app_num, string sndpkt, string message_received);

    string get_peer_nodeid();
    int get_seq_num();
    int get_app_num();
    string get_sndpkt();
    string get_message_received();

private:
    string peer_nodeid;
    int seq_num;
    int app_num;
    string sndpkt;
    string message_received;
};

#endif