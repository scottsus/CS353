/*
 * P2P Functions
 * Author: Scott Susanto
 */

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

#include "my_p2p.h"
#include "my_timer.h"
#include "my_rdtstate.h"
#include "my_reaper.h"

void await_p2p_request(string nodeid, int neighbor_socketfd, shared_ptr<Connection> conn, vector<shared_ptr<Connection>> *conns)
{
    mut.lock();
    mut.unlock();

    string message_sender_nodeid;
    while (true)
    {
        conn->set_start_time();

        shared_ptr<P2PMessage> hello_message = await_message(neighbor_socketfd, conn);
        if (hello_message == NULL)
            break;

        if (!hello_message->is_ok())
            continue;

        message_sender_nodeid = hello_message->get_origin_nodeid();
        break;
    }

    bool is_duplicate_conn = false;
    if (conn->get_neighbor_nodeid() == "")
    {
        mut.lock();
        for (shared_ptr<Connection> neighbor : *conns)
        {
            if (neighbor->get_conn_number() == conn->get_conn_number())
                continue;

            if (neighbor->get_neighbor_nodeid() == message_sender_nodeid)
                is_duplicate_conn = true;
        }
        mut.unlock();

        if (!is_duplicate_conn)
        {
            mut.lock();
            conn->set_neighbor_nodeid(message_sender_nodeid);

            shared_ptr<HelloMessage> return_hello = make_shared<HelloMessage>(nodeid);
            conn->add_message_to_queue(return_hello);
            mut.unlock();
        }
    }
    else
    {
        mut.lock();
        for (shared_ptr<Connection> neighbor : *conns)
        {
            if (neighbor->get_conn_number() == conn->get_conn_number())
                continue;

            if (neighbor->get_neighbor_nodeid() == message_sender_nodeid)
                is_duplicate_conn = true;
        }
        mut.unlock();
    }

    if (!is_duplicate_conn)
    {
        send_lsupdate_to_writer(nodeid, conns, 1);
        while (true)
        {
            mut.lock();
            if (server_socketfd == -1)
            {
                mut.unlock();
                break;
            }
            mut.unlock();

            shared_ptr<P2PMessage> message = await_message(neighbor_socketfd, conn);
            if (message == NULL)
                break;

            if (!message->is_ok())
                continue;

            message->decr_ttl_update_sender(nodeid);

            if (message->get_message_type() == "UCASTAPP")
            {
                shared_ptr<UCASTAPPMessage> ucastapp_message = static_pointer_cast<UCASTAPPMessage>(message);
                shared_ptr<RDTMessage> rdt_message = make_shared<RDTMessage>(RDTMessage(message->get_message_body()));
                if (rdt_message == NULL)
                    continue;

                string target_nodeid = ucastapp_message->get_target_nodeid();
                if (target_nodeid != nodeid)
                {
                    mut.lock();
                    map<string, shared_ptr<Node>> forwarding_table = get_forwarding_table(nodeid, conns);
                    shared_ptr<Node> next_hop = forwarding_table[target_nodeid];
                    mut.unlock();

                    if (next_hop == NULL)
                        continue;

                    string next_hop_nodeid = next_hop->get_nodeid();
                    shared_ptr<Connection> next_hop_conn = find_conn(next_hop_nodeid, conns);
                    next_hop_conn->add_message_to_queue(message);

                    continue;
                }

                if (rdt_message->get_type() == "RDTDATA")
                {
                    string src_nodeid = message->get_origin_nodeid();
                    int initial_seq_num = rdt_message->get_seq_num();
                    int initial_app_num = rdt_message->get_app_num();

                    shared_ptr<RDTState> session = sessions[src_nodeid];
                    if (session == NULL)
                    {
                        session = make_shared<RDTState>(RDTState(src_nodeid, initial_seq_num, initial_app_num, "", ""));
                        sessions[src_nodeid] = session;
                    }

                    mut.lock();
                    map<string, shared_ptr<Node>> forwarding_table = get_forwarding_table(nodeid, conns);
                    shared_ptr<Node> next_hop = forwarding_table[message->get_origin_nodeid()];
                    mut.unlock();

                    if (next_hop == NULL)
                        continue;

                    shared_ptr<Connection> next_hop_conn = find_conn(next_hop->get_nodeid(), conns);
                    shared_ptr<UCASTAPPMessage> ucastapp_ack_message = make_shared<UCASTAPPMessage>(UCASTAPPMessage(max_ttl, "", nodeid, src_nodeid, 1, initial_seq_num, initial_app_num, ""));

                    bool is_done_receiving = false;
                    while (true)
                    {
                        if (rdt_message->get_type() == "RDTACK")
                        {
                            // Let the sender thread handle the ACKs
                            send_to_timer_q(make_shared<Event>(Event("RDTACK", rdt_message)));
                        }

                        else
                        {
                            if (rdt_message->get_seq_num() == session->get_seq_num())
                            {
                                string data = rdt_message->get_rdt_message_body();
                                if (data != "\\n")
                                    session->append_message_received(data);

                                ucastapp_ack_message = make_shared<UCASTAPPMessage>(UCASTAPPMessage(max_ttl, "", nodeid, src_nodeid, 1, session->get_seq_num(), session->get_app_num(), ""));
                                next_hop_conn->add_message_to_queue(ucastapp_ack_message);

                                if (data == "\\n")
                                {
                                    is_done_receiving = true;
                                    break;
                                }

                                session->update_seq_num(!session->get_seq_num());
                            }
                            else
                            {
                                next_hop_conn->add_message_to_queue(ucastapp_ack_message);
                            }
                        }

                        shared_ptr<P2PMessage> message = await_message(neighbor_socketfd, conn);
                        if (message == NULL)
                            break;

                        rdt_message = make_shared<RDTMessage>(RDTMessage(message->get_message_body()));
                    }

                    if (is_done_receiving)
                    {
                        sessions.erase(src_nodeid);
                        cout << "RDT message '" << session->get_message_received() << "' received from " << src_nodeid << endl;
                    }
                }
                else if (rdt_message->get_type() == "RDTACK")
                {
                    send_to_timer_q(make_shared<Event>(Event("RDTACK", rdt_message)));
                }
                else // Push the UCASTAPP to the next node
                {
                    stringstream ss(message->get_message_body());
                    string protocol, message_type, message_session_id;
                    ss >> protocol >> message_type >> message_session_id;

                    if (message->get_ttl() <= 0 && target_nodeid != nodeid)
                    {
                        string src_nodeid = message->get_origin_nodeid();

                        mut.lock();
                        map<string, shared_ptr<Node>> forwarding_table = get_forwarding_table(nodeid, conns);
                        shared_ptr<Node> next_hop = forwarding_table[src_nodeid];
                        mut.unlock();

                        string next_hop_nodeid = next_hop->get_nodeid();
                        shared_ptr<Connection> next_hop_conn = find_conn(next_hop_nodeid, conns);

                        string zero_body = "353UDT/1.0 TTLZERO " + message_session_id;
                        shared_ptr<UCASTAPPMessage> ucastapp_zero = make_shared<UCASTAPPMessage>(UCASTAPPMessage(max_ttl, "", nodeid, src_nodeid, 1, zero_body));
                        next_hop_conn->add_message_to_queue(ucastapp_zero);

                        continue;
                    }

                    if (message_type == "TTLZERO")
                    {
                        if (message_session_id == session_id)
                            send_to_timer_q(make_shared<Event>("TTLZERO"));
                    }
                    else if (message_type == "PONG")
                    {
                        if (message_session_id == session_id)
                            send_to_timer_q(make_shared<Event>("PONG"));
                    }
                    else if (message_type == "PING")
                    {
                        string src_nodeid = message->get_origin_nodeid();

                        mut.lock();
                        map<string, shared_ptr<Node>> forwarding_table = get_forwarding_table(nodeid, conns);
                        shared_ptr<Node> next_hop = forwarding_table[src_nodeid];
                        mut.unlock();

                        string next_hop_nodeid = next_hop->get_nodeid();
                        shared_ptr<Connection> next_hop_conn = find_conn(next_hop_nodeid, conns);

                        string pong_body = "353UDT/1.0 PONG " + message_session_id;
                        shared_ptr<UCASTAPPMessage> ucastapp_pong = make_shared<UCASTAPPMessage>(UCASTAPPMessage(max_ttl, "", nodeid, src_nodeid, 1, pong_body));
                        next_hop_conn->add_message_to_queue(ucastapp_pong);
                    }
                }
            }
            else // type LSUPDATE
            {
                shared_ptr<LSUPDATEMessage> lsupdate_message = static_pointer_cast<LSUPDATEMessage>(message);
                log_LSUPDATE("r", lsupdate_message);

                if (lsupdate_message->get_flood_reason() == 2)
                    update_graph(lsupdate_message->get_sender_nodeid(), conns);

                string message_id = lsupdate_message->get_messageid();
                if (message_cache[message_id] != NULL)
                    continue;
                message_cache[message_id] = lsupdate_message;

                string origin_nodeid = lsupdate_message->get_origin_nodeid();
                if (origin_nodeid == nodeid)
                    continue;

                if (lsupdate_message->get_ttl() <= 0)
                    continue;

                mut.lock();
                for (shared_ptr<Connection> neighbor : *conns)
                {
                    if (conn->get_conn_number() == neighbor->get_conn_number())
                        continue;

                    neighbor->add_message_to_queue(lsupdate_message);
                }
                mut.unlock();
                if (graph[origin_nodeid] == "")
                    send_lsupdate_to_writer(nodeid, conns, 3);

                mut.lock();
                if (lsupdate_message->get_flood_reason() == 2)
                    graph.erase(origin_nodeid);
                else
                    graph[origin_nodeid] = lsupdate_message->get_message_body();
                mut.unlock();
            }
        }
    }

    send_lsupdate_to_writer(nodeid, conns, 2);
    update_graph(conn->get_neighbor_nodeid(), conns);
    conn->add_message_to_queue(NULL);
    conn->get_writer_thread()->join();

    conn->lock();
    if (conn->is_alive())
    {
        shutdown(conn->get_orig_socketfd(), SHUT_RDWR);
        close(conn->get_orig_socketfd());
        conn->set_curr_socketfd(-2);
    }
    conn->unlock();
    send_to_reaper(conn);
}

void send_p2p_response(string nodeid, shared_ptr<Connection> conn)
{
    mut.lock();
    mut.unlock();

    while (true)
    {
        shared_ptr<Message> generic_message = conn->await_message_from_queue();
        shared_ptr<P2PMessage> message = static_pointer_cast<P2PMessage>(generic_message);
        if (message == NULL)
            break;

        if (message->get_message_type() == "HELLO")
        {
            shared_ptr<HelloMessage> hello = static_pointer_cast<HelloMessage>(message);
            log_hello("i", hello);
            write_hello(conn, hello);
        }
        else if (message->get_message_type() == "UCASTAPP")
        {
            shared_ptr<UCASTAPPMessage> ucastapp_message = static_pointer_cast<UCASTAPPMessage>(message);
            shared_ptr<RDTMessage> rdt_message = make_shared<RDTMessage>(RDTMessage(ucastapp_message->get_message_body()));

            string ucastapp_type = rdt_message->get_type();
            if (ucastapp_type == "RDTDATA" || ucastapp_type == "RDTACK")
            {
                log_UCASTAPP("i", ucastapp_message);
                write_UCASTAPP(conn, ucastapp_message);
            }
            // else
            // {
            //     cout << "Normal UCASTAPP" << endl;
            //     string message_type = "i";
            //     if (message->get_origin_nodeid() != nodeid)
            //         message_type = "f";

            //     log_UCASTAPP(message_type, ucastapp_message);
            //     write_UCASTAPP(conn, ucastapp_message);
            // }
        }
        else // type LSUPDATE
        {
            shared_ptr<LSUPDATEMessage> lsupdate_message = static_pointer_cast<LSUPDATEMessage>(message);
            string message_type = "d";
            if (message->get_origin_nodeid() == nodeid)
                message_type = "i";

            log_LSUPDATE(message_type, lsupdate_message);
            write_LSUPDATE(conn, lsupdate_message);
        }
    }

    log("Socket-writing thread has terminated");
}

void send_lsupdate_to_writer(string nodeid, vector<shared_ptr<Connection>> *conns, int reason)
{
    mut.lock();
    for (shared_ptr<Connection> conn : *conns)
    {
        string neighbors = get_neighbors(conns);
        shared_ptr<LSUPDATEMessage> lsupdate_message = make_shared<LSUPDATEMessage>(LSUPDATEMessage(max_ttl, reason, "", nodeid, "TODO: start time", neighbors));
        conn->add_message_to_queue(lsupdate_message);
    }
    mut.unlock();
}

void write_hello(shared_ptr<Connection> conn, shared_ptr<HelloMessage> hello)
{
    string h1 = "353NET/1.0 SAYHELLO\r\n";
    string h2 = "TTL: " + to_string(hello->get_ttl()) + "\r\n";
    string h3 = "Flood: 0\r\n";
    string h4 = "From: " + hello->get_sender_nodeid() + "\r\n";
    string h5 = "Content-Length: " + to_string(hello->get_content_len()) + "\r\n ";
    string h6 = "\r\n";

    int neighbor_socketfd = conn->get_orig_socketfd();
    better_write_header(neighbor_socketfd, h1.c_str(), h1.length());
    better_write_header(neighbor_socketfd, h2.c_str(), h2.length());
    better_write_header(neighbor_socketfd, h3.c_str(), h3.length());
    better_write_header(neighbor_socketfd, h4.c_str(), h4.length());
    better_write_header(neighbor_socketfd, h5.c_str(), h5.length());
    better_write_header(neighbor_socketfd, h6.c_str(), h6.length());

    int conn_number = conn->get_conn_number();
    log_header(h1, conn_number);
    log_header(h2, conn_number);
    log_header(h3, conn_number);
    log_header(h4, conn_number);
    log_header(h5, conn_number);
    log_header(h6, conn_number);
}

void write_LSUPDATE(shared_ptr<Connection> conn, shared_ptr<LSUPDATEMessage> message)
{
    string h1 = "353NET/1.0 LSUPDATE\r\n";
    string h2 = "TTL: " + to_string(max_ttl) + "\r\n";
    string h3 = "Flood: 1;reason=" + to_string(message->get_flood_reason()) + "\r\n";
    string h4 = "MessageID: " + message->get_messageid() + "\r\n";
    string h5 = "From: " + message->get_origin_nodeid() + "\r\n";
    string h6 = "OriginStartTime: " + message->get_origin_starttime() + "\r\n";
    string h7 = "Content-Length: " + to_string(message->get_content_len()) + "\r\n";
    string h8 = "\r\n";
    string h9 = message->get_message_body();

    int neighbor_socketfd = conn->get_orig_socketfd();
    better_write_header(neighbor_socketfd, h1.c_str(), h1.length());
    better_write_header(neighbor_socketfd, h2.c_str(), h2.length());
    better_write_header(neighbor_socketfd, h3.c_str(), h3.length());
    better_write_header(neighbor_socketfd, h4.c_str(), h4.length());
    better_write_header(neighbor_socketfd, h5.c_str(), h5.length());
    better_write_header(neighbor_socketfd, h6.c_str(), h6.length());
    better_write_header(neighbor_socketfd, h7.c_str(), h7.length());
    better_write_header(neighbor_socketfd, h8.c_str(), h8.length());
    better_write(neighbor_socketfd, h9.c_str(), h9.length());

    int conn_number = conn->get_conn_number();
    log_header(h1, conn_number);
    log_header(h2, conn_number);
    log_header(h3, conn_number);
    log_header(h4, conn_number);
    log_header(h5, conn_number);
    log_header(h6, conn_number);
    log_header(h7, conn_number);
    log_header(h8, conn_number);
    log_header(h9 + "\r\n", conn_number);
}

// void write_UCASTAPP(shared_ptr<Connection> conn, shared_ptr<UCASTAPPMessage> message, int next_layer)
// {
//     string h1 = "353NET/1.0 UCASTAPP\r\n";
//     string h2 = "TTL: " + to_string(message->get_ttl()) + "\r\n";
//     string h3 = "Flood: 0\r\n";
//     string h4 = "MessageID: " + message->get_messageid() + "\r\n";
//     string h5 = "From: " + message->get_origin_nodeid() + "\r\n";
//     string h6 = "To: " + message->get_target_nodeid() + "\r\n";
//     string h7 = "Next-Layer: " + to_string(next_layer) + "\r\n";
//     string h8 = "Content-Length: " + to_string(message->get_content_len()) + "\r\n";
//     string h9 = "\r\n";
//     string h10 = message->get_message_body();

//     int next_socketfd = conn->get_orig_socketfd();
//     better_write_header(next_socketfd, h1.c_str(), h1.length());
//     better_write_header(next_socketfd, h2.c_str(), h2.length());
//     better_write_header(next_socketfd, h3.c_str(), h3.length());
//     better_write_header(next_socketfd, h4.c_str(), h4.length());
//     better_write_header(next_socketfd, h5.c_str(), h5.length());
//     better_write_header(next_socketfd, h6.c_str(), h6.length());
//     better_write_header(next_socketfd, h7.c_str(), h7.length());
//     better_write_header(next_socketfd, h8.c_str(), h8.length());
//     better_write_header(next_socketfd, h9.c_str(), h9.length());
//     better_write(next_socketfd, h10.c_str(), h10.length());

//     int conn_number = conn->get_conn_number();
//     log_header(h1, conn_number);
//     log_header(h2, conn_number);
//     log_header(h3, conn_number);
//     log_header(h4, conn_number);
//     log_header(h5, conn_number);
//     log_header(h6, conn_number);
//     log_header(h7, conn_number);
//     log_header(h8, conn_number);
//     log_header(h9, conn_number);
//     log_header(h10 + "\r\n", conn_number);
// }

void write_UCASTAPP(shared_ptr<Connection> conn, shared_ptr<UCASTAPPMessage> ucastapp_message)
{
    string h1 = "353NET/1.0 UCASTAPP\r\n";
    string h2 = "TTL: " + to_string(ucastapp_message->get_ttl()) + "\r\n";
    string h3 = "Flood: 0\r\n";
    string h4 = "MessageID: " + ucastapp_message->get_messageid() + "\r\n";
    string h5 = "From: " + ucastapp_message->get_origin_nodeid() + "\r\n";
    string h6 = "To: " + ucastapp_message->get_target_nodeid() + "\r\n";
    string h7 = "Next-Layer: " + to_string(ucastapp_message->get_next_layer()) + "\r\n";

    string message_body = ucastapp_message->get_message_body();
    string h8 = "Content-Length: " + to_string(message_body.length()) + "\r\n";
    string h9 = "\r\n";

    int next_socketfd = conn->get_orig_socketfd();
    better_write_header(next_socketfd, h1.c_str(), h1.length());
    better_write_header(next_socketfd, h2.c_str(), h2.length());
    better_write_header(next_socketfd, h3.c_str(), h3.length());
    better_write_header(next_socketfd, h4.c_str(), h4.length());
    better_write_header(next_socketfd, h5.c_str(), h5.length());
    better_write_header(next_socketfd, h6.c_str(), h6.length());
    better_write_header(next_socketfd, h7.c_str(), h7.length());
    better_write_header(next_socketfd, h8.c_str(), h8.length());
    better_write_header(next_socketfd, h9.c_str(), h9.length());
    better_write(next_socketfd, message_body.c_str(), message_body.length());

    int conn_number = conn->get_conn_number();
    log_header(h1, conn_number);
    log_header(h2, conn_number);
    log_header(h3, conn_number);
    log_header(h4, conn_number);
    log_header(h5, conn_number);
    log_header(h6, conn_number);
    log_header(h7, conn_number);
    log_header(h8, conn_number);
    log_header(h9, conn_number);
    log_header(message_body + "\r\n", conn_number);
}
