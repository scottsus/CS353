/*
 * Console
 */

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>

#include "my_console.h"
#include "my_reaper.h"
#include "my_utils.h"

void handle_console(string nodeid, vector<shared_ptr<Connection>> *conns)
{
    string cmd;
    while (true)
    {
        cout << nodeid + "> ";
        cin >> cmd;
        if (cin.fail() || cin.eof() || cmd == "quit")
        {
            if (cin.fail() || cin.eof())
                cout << endl;

            send_to_reaper(NULL);
            cout << "Console thread terminated" << endl;
            break;
        }
        else if (cmd == "status")
        {
            mut.lock();
            if (has_active_conns(*conns))
            {
                cout << "The following connections are active:" << endl;
                for (shared_ptr<Connection> conn : *conns)
                    if (conn->is_alive())
                    {
                        cout << "\t[" << conn->get_conn_number() << "]\tClient at " << conn->get_ip_port() << endl;
                        cout << "\t\tPath: " << conn->get_uri() << endl;
                        cout << "\t\tContent-Length: " << conn->get_content_len() << endl;
                        cout << "\t\tStart-Time: " << conn->get_start_time() << endl;
                        cout << "\t\tShaper-Params: " << conn->get_shaper_params() << endl;
                        cout << "\t\tSent: " << conn->get_kb_sent() << " bytes " << conn->get_kb_percent_sent() << ", ";
                        cout << "time elapsed: " << conn->get_time_elapsed() << " sec" << endl;
                    }
            }
            else
            {
                cout << "No active connections" << endl;
            }
            mut.unlock();
        }
        else if (cmd == "dial")
        {
            string line;
            getline(cin, line);

            string target_conn_number_str, percent_str;
            stringstream ss(line);
            ss >> target_conn_number_str >> percent_str;

            if (!is_digit(target_conn_number_str) || !is_digit(percent_str))
            {
                cout << "Invalid # or percent. The command syntax is 'dial # percent'. Please try again" << endl;
                continue;
            }

            int target_conn_number = stoi(target_conn_number_str);
            shared_ptr<Connection> conn = find_conn(target_conn_number, conns);

            if (!conn)
            {
                cout << "No such connection: " << target_conn_number << endl;
                continue;
            }

            int percent = stoi(percent_str);
            if (!(1 <= percent && percent <= 100))
            {
                cout << "Dial value is out of range (it must be >=1 and <=100)" << endl;
                continue;
            }

            conn->set_dial(percent);
            cout << "Dial for connection " << target_conn_number << " at " << percent << "%. ";
            cout << "Token rate at " << conn->get_speed_str() << " tokens/s." << endl;

            log("Shaper-Params[" + to_string(conn->get_conn_number()) + "]: " + conn->get_shaper_params());
        }
        else if (cmd == "neighbors")
        {
            if (conns->empty())
            {
                cout << nodeid << " has no active neighbors" << endl;
                continue;
            }

            cout << "Active neighbors of " + nodeid + ":" << endl;
            mut.lock();
            for (shared_ptr<Connection> conn : *conns)
            {
                if (!conn->is_alive())
                    continue;

                string neighbor_nodeid = conn->get_neighbor_nodeid();
                if (neighbor_nodeid == nodeid)
                    continue;

                if (neighbor_nodeid != "")
                    cout << "\t" + neighbor_nodeid << endl;
            }
            mut.unlock();
        }
        else if (cmd == "netgraph")
        {
            mut.lock();
            string neighbors = get_neighbors(conns);
            if (neighbors.empty() && graph.empty())
            {
                cout << nodeid << " has no active neighbors" << endl;
                mut.unlock();
                continue;
            }

            string neighbors_cache = graph[nodeid];
            if (!neighbors.empty() && neighbors_cache != neighbors)
                graph[nodeid] = neighbors;

            for (map<string, string>::iterator itr = graph.begin(); itr != graph.end(); itr++)
            {
                string direct_neighbors = itr->first;
                string indirect_neighbors = itr->second;
                cout << direct_neighbors << ": " << indirect_neighbors << endl;
            }
            mut.unlock();
        }
        else if (cmd == "close")
        {
            string target_conn_str;
            cin >> target_conn_str;
            int target_conn = stoi(target_conn_str);

            shared_ptr<Connection> conn = find_conn(target_conn, conns);

            if (!conn)
            {
                cout << "No such connection: " << target_conn << endl;
                continue;
            }

            shutdown(conn->get_orig_socketfd(), SHUT_RDWR);
            close(conn->get_orig_socketfd());
            conn->set_curr_socketfd(-2);
            conn->set_reason("at user's request");
            send_to_reaper(conn);
            cout << "Closing connection " << to_string(conn->get_conn_number()) << " ..." << endl;
        }
        else
        {
            cout << "Command not recognized. Valid commands are:" << endl;
            cout << "\tclose #" << endl;
            cout << "\tdial # percent" << endl;
            cout << "\tneighbors" << endl;
            cout << "\tnetgraph" << endl;
            cout << "\tstatus" << endl;
            cout << "\tquit" << endl;
        }
    }

    mut.lock();
    shutdown(server_socketfd, SHUT_RDWR);
    close(server_socketfd);
    server_socketfd = -1;
    mut.unlock();

    return;
}
// Can only be called when lock is held
bool has_active_conns(vector<shared_ptr<Connection>> conns)
{
    for (shared_ptr<Connection> conn : conns)
        if (conn->is_alive())
            return true;

    return false;
}

bool is_digit(string str)
{
    try
    {
        stoi(str);
    }
    catch (exception &e)
    {
        return false;
    }
    return true;
}
