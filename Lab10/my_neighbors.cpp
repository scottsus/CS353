/*
 * Neighbors
 */

#include <thread>
#include <unistd.h>
#include <iostream>

#include "my_neighbors.h"
#include "my_server.h"

void find_neighbors(string neighbors_str, vector<shared_ptr<Connection>> *conns)
{
    vector<string> neighbors{};
    stringstream ss(neighbors_str);
    string neighbor_nodeid;
    while (getline(ss, neighbor_nodeid, ','))
        neighbors.push_back(neighbor_nodeid);

    while (true)
    {
        vector<string> inactive_neighbors(neighbors.begin(), neighbors.end());

        mut.lock();
        if (server_socketfd == -1)
        {
            mut.unlock();
            break;
        }

        for (shared_ptr<Connection> neighbor : *conns)
        {
            string neighbor_nodeid = neighbor->get_neighbor_nodeid();
            if (neighbor_nodeid != "")
                remove_from_vector(neighbor_nodeid, inactive_neighbors);
        }
        mut.unlock();

        for (string neighbor_nodeid : inactive_neighbors)
        {
            int neighbor_socketfd = create_client_socket_and_connect("LOCALHOST", neighbor_nodeid.substr(1));
            if (neighbor_socketfd == -1)
                continue;

            mut.lock();
            shared_ptr<Connection> neighbor_conn = make_shared<Connection>(Connection(conn_number++, neighbor_socketfd, neighbor_nodeid));
            neighbor_conn->set_reader_thread(make_shared<thread>(thread(await_request, neighbor_conn, conns)));
            neighbor_conn->set_writer_thread(make_shared<thread>(thread(send_response, neighbor_nodeid, neighbor_conn)));
            neighbor_conn->set_neighbor_nodeid(neighbor_nodeid);
            conns->push_back(neighbor_conn);

            Message hello(neighbor_nodeid, 0);
            neighbor_conn->add_msg_to_queue(make_shared<Message>(hello));
            mut.unlock();
        }

        sleep(stoi(config["params"]["neighbor_retry_interval"]));
    }
}