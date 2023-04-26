/*
 * Console
 */

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>

#include "my_console.h"
#include "my_event.h"
#include "my_reaper.h"
#include "my_timer.h"
#include "my_utils.h"

int read_rdt_ack_and_get_seq_num(int peer_socketfd)
{
    string protocol;
    int bytes_received = read_a_line(peer_socketfd, protocol);
    if (bytes_received <= 2)
        return 0;
    stringstream ss(protocol);
    string version, type;
    ss >> version >> type;

    if (type != "ACK")
        return 0;

    string seq_num_line;
    bytes_received = read_a_line(peer_socketfd, seq_num_line);
    if (bytes_received <= 2)
        return 0;
    int seq_num = stoi(extract_header_value(seq_num_line));

    string app_num_line;
    bytes_received = read_a_line(peer_socketfd, app_num_line);
    if (bytes_received <= 2)
        return 0;

    string content_len_line;
    bytes_received = read_a_line(peer_socketfd, content_len_line);
    if (bytes_received <= 2)
        return 0;

    string empty_line;
    bytes_received = read_a_line(peer_socketfd, empty_line);
    if (bytes_received != 2)
        return 0;

    return seq_num;
}

void handle_p2p_console(string nodeid, vector<shared_ptr<Connection>> *conns)
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
        else if (cmd == "neighbors")
        {
            if (conns->empty())
            {
                cout << nodeid << " has no active neighbors" << endl;
                continue;
            }

            cout << "Active neighbors of " + nodeid + ":" << endl;
            cout << "\t";
            mut.lock();
            for (uint i = 0; i < conns->size(); i++)
            {
                shared_ptr<Connection> conn = conns->at(i);
                if (!conn->is_alive())
                    continue;

                string neighbor_nodeid = conn->get_neighbor_nodeid();
                if (neighbor_nodeid == nodeid)
                    continue;

                if (neighbor_nodeid != "")
                {
                    cout << neighbor_nodeid;
                    cout << "[" << conn->get_orig_socketfd() << "]";
                    if (i != conns->size() - 1)
                        cout << ",";
                }
            }
            mut.unlock();
            cout << endl;
        }
        else if (cmd == "netgraph")
        {
            mut.lock();
            string neighbors = get_neighbors(conns);
            if (neighbors.empty())
            {
                cout << nodeid << " has no active neighbors" << endl;
                graph.clear();
                mut.unlock();
                continue;
            }
            graph[nodeid] = neighbors;

            for (map<string, string>::iterator itr = graph.begin(); itr != graph.end(); itr++)
            {
                string direct_neighbors = itr->first;
                string indirect_neighbors = itr->second;
                cout << direct_neighbors << ": " << indirect_neighbors << endl;
            }
            mut.unlock();
        }
        else if (cmd == "forwarding")
        {
            mut.lock();
            map<string, shared_ptr<Node>> forwarding_table = get_forwarding_table(nodeid, conns);
            if (forwarding_table.empty())
            {
                cout << nodeid << " has empty forwarding table" << endl;
                mut.unlock();
                continue;
            }

            for (map<string, shared_ptr<Node>>::iterator itr = forwarding_table.begin(); itr != forwarding_table.end(); itr++)
            {
                string target_nodeid = itr->first;
                shared_ptr<Node> next_hop = itr->second;
                cout << target_nodeid << ": " << next_hop->get_nodeid() << endl;
            }

            mut.unlock();
        }
        else if (cmd == "rdtsend")
        {
            string target_nodeid, message;
            cin >> target_nodeid >> message;
            cout << "RDTSEND: " << target_nodeid << ": " << message << endl;

            mut.lock();
            map<string, shared_ptr<Node>> forwarding_table = get_forwarding_table(nodeid, conns);
            shared_ptr<Node> next_hop = forwarding_table[target_nodeid];
            mut.unlock();

            int seq_num = 0, N = message.length();
            for (int i = 0; i < N; i++)
            {
                shared_ptr<Connection> next_hop_conn = find_conn(next_hop->get_nodeid(), conns);
                shared_ptr<Message> rdt_message = make_shared<RDTMessage>(seq_num, 98, 1, message[i]);
                rdt_message->set_message_type("RDT");
                next_hop_conn->add_message_to_queue(rdt_message);
                cout << "Added to queue: " << message[i] << endl;

                Timer timer(msg_lifetime);
                bool done_waiting = false;
                while (!done_waiting)
                {
                    cout << "Waiting for RDTACK" << endl;
                    shared_ptr<Event> event = timer.await_timeout();
                    if (event->get_event_type() == "RDTACK")
                    {
                        cout << "Received RDTACK" << endl;
                        int peer_socketfd = next_hop_conn->get_orig_socketfd();
                        int response_seq_num = read_rdt_ack_and_get_seq_num(peer_socketfd);
                        cout << "response_seq_num: " << response_seq_num << endl;
                        if (response_seq_num == seq_num)
                        {
                            cout << "Received correct RDTACK" << endl;
                            timer.stop();
                            done_waiting = true;
                        }
                    }
                    else if (event->get_event_type() == "TIMEOUT")
                    {
                        // print timeout message
                        timer.get_timer_thread()->join();
                        next_hop_conn->add_message_to_queue(rdt_message);
                        timer = *(new Timer(msg_lifetime));
                    }
                }

                timer.get_timer_thread()->join();
                tuple<double, string> timediff_tuple = timer.stop();
                double timediff = get<0>(timediff_tuple);
                cout << "timediff: " << timediff << endl;
                if (timediff < 1)
                    usleep(1 - timediff);
                seq_num++;
            }
            // print finished message
        }
        else if (cmd == "traceroute")
        {
            string target_nodeid;
            cin >> target_nodeid;

            if (target_nodeid == nodeid)
            {
                cout << "Cannot traceroute to yourself." << endl;
                continue;
            }

            bool target_reached = false;
            for (int i = 1; i <= max_ttl; i++)
            {
                Timer timer(msg_lifetime);

                string start_time;
                get_message_id_and_start_time(nodeid, "ses", session_id, start_time);
                string message_body = "353UDT/1.0 PING " + session_id;

                mut.lock();
                map<string, shared_ptr<Node>> forwarding_table = get_forwarding_table(nodeid, conns);
                shared_ptr<Node> next_hop = forwarding_table[target_nodeid];
                mut.unlock();

                if (next_hop != NULL)
                {
                    shared_ptr<Connection> next_hop_conn = find_conn(next_hop->get_nodeid(), conns);
                    shared_ptr<Message> ucastapp_ping = make_shared<Message>(i, nodeid, target_nodeid, 1, message_body.length(), message_body);
                    next_hop_conn->add_message_to_queue(ucastapp_ping);
                }

                shared_ptr<Event> event = timer.await_timeout();
                if (event->get_event_type() == "TTLZERO")
                {
                    tuple<double, string> timediff_tuple = timer.stop();
                    cout << i << " - " << next_hop->get_nodeid() << ", " << get<1>(timediff_tuple) << endl;
                }
                else if (event->get_event_type() == "PONG")
                {
                    tuple<double, string> timediff_tuple = timer.stop();
                    cout << i << " - " << next_hop->get_nodeid() << ", " << get<1>(timediff_tuple) << endl;
                    cout << target_nodeid << " is reached in " << i << " steps" << endl;
                    timer.get_timer_thread()->join();
                    target_reached = true;
                    break;
                }
                else if (event->get_event_type() == "TIMEOUT")
                {
                    cout << i << " - *" << endl;
                }
                timer.get_timer_thread()->join();
                sleep(3);
            }

            if (!target_reached)
                cout << "traceroute: " << target_nodeid << " not reached after " << max_ttl << " steps" << endl;
        }
        else
        {
            cout << "Command not recognized. Valid commands are:" << endl;
            cout << "\tforwarding" << endl;
            cout << "\tneighbors" << endl;
            cout << "\tnetgraph" << endl;
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

void handle_http_console(vector<shared_ptr<Connection>> *conns)
{
    string cmd;
    while (true)
    {
        cout << "> ";
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

// Need LOCK
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
