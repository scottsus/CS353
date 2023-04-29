/*
 * Console
 */

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>

#include "my_console.h"
#include "my_event.h"
#include "my_message.h"
#include "my_echojob.h"
#include "my_echoclient.h"
#include "my_echoserver.h"
#include "my_reaper.h"
#include "my_timer.h"
#include "my_utils.h"

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
            send_to_echoserver(NULL);
            cout << "Console thread terminated" << endl;
            break;
        }
        else if (cmd == "echoapp")
        {
            string target_nodeid;
            cin >> target_nodeid;

            string empty_line;
            getline(cin, empty_line);

            while (true)
            {
                cout << "echoapp> ";
                string line;
                getline(cin, line);

                while (line.length() > 40)
                {
                    cout << "line too long (40 characters max); please try again" << endl;
                    getline(cin, line);
                }

                mut.lock();
                map<string, shared_ptr<Node>> forwarding_table = get_forwarding_table(nodeid, conns);
                shared_ptr<Node> next_hop = forwarding_table[target_nodeid];
                mut.unlock();

                if (next_hop == NULL)
                {
                    cout << target_nodeid << " is not reachable" << endl;
                    continue;
                }

                shared_ptr<Connection> next_hop_conn = find_conn(next_hop->get_nodeid(), conns);
                if (next_hop_conn == NULL)
                    continue;

                rdt_send(nodeid, target_nodeid, line, 98, next_hop_conn, conns);
                string echoed_line = await_echoed_line();
                if (echoed_line == "")
                    break;

                cout << "echoed '" << echoed_line << "' received from " << target_nodeid << endl;
            }

            cout << "echoapp: terminated" << endl;
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

            if (target_nodeid == nodeid)
            {
                cout << "Cannot use rdtsend command to send message to yourself." << endl;
                continue;
            }

            mut.lock();
            map<string, shared_ptr<Node>> forwarding_table = get_forwarding_table(nodeid, conns);
            shared_ptr<Node> next_hop = forwarding_table[target_nodeid];
            mut.unlock();

            if (next_hop == NULL)
            {
                cout << target_nodeid << " is not reachable" << endl;
                continue;
            }

            shared_ptr<Connection> next_hop_conn = find_conn(next_hop->get_nodeid(), conns);
            if (next_hop_conn == NULL)
                continue;

            rdt_send(nodeid, target_nodeid, message, 98, next_hop_conn, conns);

            cout << "rdtsend: '" << message << "' to " << target_nodeid << " have been sent and acknowledged" << endl;
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
                    shared_ptr<UCASTAPPMessage> ucastapp_ping = make_shared<UCASTAPPMessage>(i, "", nodeid, target_nodeid, 1, message_body);
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

void rdt_send(string nodeid, string target_nodeid, string message, int app_num, shared_ptr<Connection> next_hop_conn, vector<shared_ptr<Connection>> *conns)
{
    if (message[message.length() - 1] != '\n')
        message += '\n';

    int seq_num = 0, N = message.length();
    for (int i = 0; i < N; i++)
    {
        shared_ptr<UCASTAPPMessage> ucastapp_data_message = make_shared<UCASTAPPMessage>(UCASTAPPMessage(max_ttl, "", nodeid, target_nodeid, 2, seq_num, app_num, string{message[i]}));
        next_hop_conn->add_message_to_queue(ucastapp_data_message);

        Timer timer(msg_lifetime);
        bool done_waiting = false;
        while (!done_waiting)
        {
            shared_ptr<Event> event = timer.await_timeout();
            if (event->get_event_type() == "RDTACK")
            {
                shared_ptr<RDTMessage> rdt_message = event->get_rdt_message();
                if (rdt_message->get_seq_num() == seq_num)
                {
                    timer.stop();
                    done_waiting = true;
                }
            }
            else if (event->get_event_type() == "TIMEOUT")
            {
                timer.get_timer_thread()->join();

                mut.lock();
                map<string, shared_ptr<Node>> forwarding_table = get_forwarding_table(nodeid, conns);
                shared_ptr<Node> next_hop = forwarding_table[target_nodeid];
                mut.unlock();

                if (next_hop != NULL)
                {
                    next_hop_conn = find_conn(next_hop->get_nodeid(), conns);
                    next_hop_conn->add_message_to_queue(ucastapp_data_message);
                }

                timer = *(new Timer(msg_lifetime));
                cout << "RDT sender timeout for Seq-Number: " << seq_num << " and RDT-App-Number: 0" << endl;
            }
        }

        timer.get_timer_thread()->join();
        tuple<double, string> timediff_tuple = timer.stop();
        double timediff = get<0>(timediff_tuple);
        if (timediff < 1)
            usleep((1 - timediff) * 1000000);

        seq_num = !seq_num;
    }
}

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
