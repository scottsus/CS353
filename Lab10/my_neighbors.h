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
extern map<string, map<string, string>> config;
extern mutex mut;

void find_neighbors(string neighbors, vector<shared_ptr<Connection>> *conns);
