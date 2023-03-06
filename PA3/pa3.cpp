#include <string>
#include <vector>
#include <map>
#include <mutex>

using namespace std;

/* Custom files */
#include "my_utils.h"
#include "my_server.h"
#include "my_client.h"

int server_socketfd = -1;
const int MAX_CONNECTIONS = 999999;

map<string, map<string, string>> config;
string rootdir = "";
ofstream logfile;

mutex mut;
vector<shared_ptr<Connection>> conns{};

int main(int argc, char *argv[])
{
    tuple<int, vector<string>> mode_tuple = choose_mode(argc, argv);

    int mode = get<0>(mode_tuple);
    if (mode == 1)
    {
        vector<string> server_args = get<1>(mode_tuple);
        run_server(server_args.at(0));
    }
    else
    {
        vector<string> client_args = get<1>(mode_tuple);
        run_client(client_args);
    }

    return 0;
}
