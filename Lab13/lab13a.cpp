#include <string>
#include <vector>
#include <map>
#include <mutex>

using namespace std;

/* Custom files */
#include "my_utils.h"
#include "my_server.h"
#include "my_client.h"

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

map<string, string> graph;
map<string, shared_ptr<Message>> message_cache;

int max_ttl = 0;
int msg_lifetime = 0;
int neighbor_retry_interval = 0;

bool cancelled = false;
string session_id = "";

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
