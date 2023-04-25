/*
 * Neighbors
 */

#include <string>
#include <vector>
#include <memory>

#include "my_connection.h"
#include "my_utils.h"

using namespace std;

extern int server_socketfd;
extern int conn_number;
extern int neighbor_retry_interval;
extern mutex mut;

void find_neighbors(string nodeid, string neighbors, vector<shared_ptr<Connection>> *conns);
