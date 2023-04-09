/*
 *  Server
 */

#include <iostream>
#include <fstream>
#include <mutex>
#include <map>
#include <algorithm>

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include "my_server.h"
#include "my_http.h"
#include "my_p2p.h"
#include "my_reaper.h"
#include "my_message.h"
#include "my_utils.h"

using namespace std;

void run_p2p_server(string config_file)
{
    config = get_config(config_file);
    map<string, string> startup_opts = config["startup"];

    rootdir = "lab10data";
    string host = startup_opts["port"];
    string port = startup_opts["port"];
    string nodeid = ":" + port;

    max_ttl = stoi(config["params"]["max_ttl"]);

    logfile.open(startup_opts["logfile"]);
    log("START: port=" + port + ", rootdir=" + rootdir);

    server_socketfd = create_listening_socket(port);
    if (server_socketfd == -1)
    {
        cerr << "Unable to create server socket" << endl;
        return;
    }
    string server_ip_and_port = get_ip_and_port_for_server(server_socketfd, 1);

    thread console_thread(handle_console, nodeid, &conns);
    thread reaper_thread(reap_threads, &conns);
    thread neighbors_thread(find_neighbors, nodeid, config["topology"][nodeid], &conns);

    while (true)
    {
        if (conn_number == MAX_CONNECTIONS)
        {
            cout << "999,999 connections served. Proceed with auto-shutdown..." << endl;
            break;
        }

        mut.lock();
        if (server_socketfd == -1)
        {
            shutdown(server_socketfd, SHUT_RDWR);
            close(server_socketfd);
        }
        mut.unlock();

        int neighbor_socketfd = my_accept(server_socketfd);
        if (neighbor_socketfd == -1)
            break;
        log("CONNECT[" + to_string(conn_number) + "]: " + get_ip_and_port_for_client(neighbor_socketfd, 1));

        mut.lock();
        shared_ptr<Connection> neighbor_conn = make_shared<Connection>(Connection(conn_number++, neighbor_socketfd));
        neighbor_conn->set_reader_thread(make_shared<thread>(thread(await_p2p_request, nodeid, neighbor_conn, &conns)));
        neighbor_conn->set_writer_thread(make_shared<thread>(thread(send_p2p_response, nodeid, neighbor_conn, &conns)));
        conns.push_back(neighbor_conn);
        mut.unlock();
    }

    console_thread.join();
    reaper_thread.join();
    neighbors_thread.join();

    log("STOP: port=" + port);
}

void run_http_server(string config_file)
{
    config = get_config(config_file);
    map<string, string> startup_opts = config["startup"];

    rootdir = "lab10data";
    string host = startup_opts["port"];
    string port = startup_opts["port"];
    string nodeid = ":" + port;

    logfile.open(startup_opts["logfile"]);
    log("START: port=" + port + ", rootdir=" + rootdir);

    server_socketfd = create_listening_socket(port);
    if (server_socketfd == -1)
    {
        cerr << "Unable to create server socket" << endl;
        return;
    }
    string server_ip_and_port = get_ip_and_port_for_server(server_socketfd, 1);

    thread console_thread(handle_console, nodeid, &conns);
    thread reaper_thread(reap_threads, &conns);

    while (true)
    {
        if (conn_number == MAX_CONNECTIONS)
        {
            cout << "999,999 connections served. Proceed with auto-shutdown..." << endl;
            break;
        }

        mut.lock();
        if (server_socketfd == -1)
        {
            shutdown(server_socketfd, SHUT_RDWR);
            close(server_socketfd);
        }
        mut.unlock();

        int client_socketfd = my_accept(server_socketfd);
        if (client_socketfd == -1)
            break;
        log("CONNECT[" + to_string(conn_number) + "]: " + get_ip_and_port_for_client(client_socketfd, 1));

        mut.lock();
        shared_ptr<Connection> client_conn = make_shared<Connection>(Connection(conn_number++, client_socketfd));
        client_conn->set_reader_thread(make_shared<thread>(thread(await_http_request, client_conn, &conns)));
        client_conn->set_writer_thread(make_shared<thread>(thread(send_http_response, client_conn, &conns)));
        conns.push_back(client_conn);
        mut.unlock();
    }

    console_thread.join();
    reaper_thread.join();

    log("STOP: port=" + port);
}
