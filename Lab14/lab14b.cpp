#include <string>
#include <vector>
#include <map>
#include <mutex>

using namespace std;

/* Custom files */
#include "my_client.h"
#include "my_server.h"
#include "my_echojob.h"
#include "my_rdtstate.h"
#include "my_utils.h"

int mode;
int server_socketfd = -1;
int conn_number = 1;
const int MAX_CONNECTIONS = 999999;

map<string, map<string, string>> config;
string rootdir = "";
string host;

ofstream logfile;
int SAYHELLO = 0;
int LSUPDATE = 0;
int UCASTAPP = 0;

vector<shared_ptr<Connection>> conns{};

mutex mut;

condition_variable timer_cv;
queue<shared_ptr<Event>> timer_q;

condition_variable reaper_cv;
queue<shared_ptr<Connection>> reaper_q;

condition_variable echojob_cv;
queue<shared_ptr<EchoJob>> echojob_q;

condition_variable echo_cv;
queue<string> echo_q;

map<string, string> graph;
map<string, shared_ptr<Message>> message_cache;

int max_ttl = 0;
int msg_lifetime = 0;
int neighbor_retry_interval = 0;

bool cancelled = false;
string session_id = "";
map<string, map<string, shared_ptr<RDTState>>> sessions{};

int main(int argc, char *argv[])
{
    tuple<int, vector<string>> mode_tuple = choose_mode(argc, argv);
    mode = get<0>(mode_tuple);

    if (mode == 0) // P2P Server
    {
        vector<string> server_args = get<1>(mode_tuple);
        run_p2p_server(server_args.at(0));
    }
    else if (mode == 1) // HTTP Server
    {
        // Use Config file
        vector<string> server_args = get<1>(mode_tuple);
        run_http_server(server_args.at(0));
    }
    else // HTTP Client
    {
        vector<string> client_args = get<1>(mode_tuple);
        run_client(client_args);
    }

    return 0;
}
