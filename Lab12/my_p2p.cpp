/*
 * P2P Functions
 * Author: Scott Susanto
 */

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

#include "my_p2p.h"
#include "my_reaper.h"

void await_p2p_request(string nodeid, int neighbor_socketfd, shared_ptr<Connection> conn, vector<shared_ptr<Connection>> *conns)
{
    mut.lock();
    mut.unlock();

    string message_sender_nodeid;
    while (true)
    {
        conn->set_start_time();

        shared_ptr<Message> hello_message = await_message_from(neighbor_socketfd, conn);
        if (hello_message == NULL)
            continue;
        if (hello_message->thread_should_terminate())
            break;

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

            Message return_hello(nodeid);
            conn->add_message_to_queue(make_shared<Message>(return_hello));
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

            shared_ptr<Message> message = await_message_from(neighbor_socketfd, conn);
            if (message->thread_should_terminate())
            {
                mut.unlock();
                break;
            }

            if (!message->is_ok())
            {
                mut.unlock();
                continue;
            }

            if (message->get_target_nodeid() != "") // type UCASTAPP
            {
                log_UCASTAPP("r", message);

                if (message->get_target_nodeid() == nodeid)
                {
                    cout << "No receiver for UDT message '" << message->get_message_body() << "' from " << message->get_origin_nodeid() << "." << endl;
                    mut.unlock();
                    continue;
                }

                mut.lock();
                string target_nodeid = message->get_target_nodeid();
                map<string, shared_ptr<Node>> forwarding_table = get_forwarding_table(nodeid, conns);
                shared_ptr<Node> next_hop = forwarding_table[target_nodeid];

                string next_hop_nodeid = next_hop->get_nodeid();
                mut.unlock();

                shared_ptr<Connection> next_hop_conn = find_conn(next_hop_nodeid, conns);
                next_hop_conn->add_message_to_queue(message);
            }
            else
            {
                log_LSUPDATE("r", message);

                if (message->get_flood_reason() == 2)
                    update_graph(message->get_sender_nodeid(), conns);

                string message_id = message->get_message_id();
                if (message_cache[message_id] != NULL)
                    continue;
                message_cache[message_id] = message;

                string origin_nodeid = message->get_origin_nodeid();
                if (origin_nodeid == nodeid)
                    continue;

                if (message->get_ttl() <= 0)
                    continue;

                message = message->decr_ttl_update_sender(nodeid);

                mut.lock();
                for (shared_ptr<Connection> neighbor : *conns)
                {
                    if (conn->get_conn_number() == neighbor->get_conn_number())
                        continue;

                    neighbor->add_message_to_queue(message);
                }
                mut.unlock();

                if (graph[origin_nodeid] == "")
                    send_lsupdate_to_writer(nodeid, conns, 3);

                mut.lock();
                if (message->get_flood_reason() == 2)
                    graph.erase(origin_nodeid);
                else
                    graph[origin_nodeid] = message->get_message_body();
            }

            mut.unlock();
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
        shared_ptr<Message> message = conn->await_message_from_queue();
        if (message == NULL)
            break;

        if (message->is_hello())
        {
            int content_len = message->get_content_len();
            log_header("i", nodeid, 1, 0, content_len);
            write_hello(conn, max_ttl, "0", nodeid, content_len);
        }
        else
        {
            if (message->get_target_nodeid() != "")
            {
                string message_type = "f";
                if (message->get_origin_nodeid() == nodeid)
                    message_type = "i";

                log_UCASTAPP(message_type, message);
                write_UCASTAPP(conn, message, 0);
            }
            else
            {
                string message_type = "d";
                if (message->get_origin_nodeid() == nodeid)
                    message_type = "i";

                log_LSUPDATE(message_type, message);
                write_LSUPDATE(conn, message);
            }
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
        Message message = Message(max_ttl, reason, nodeid, nodeid, neighbors, neighbors.length());
        conn->add_message_to_queue(make_shared<Message>(message));
    }
    mut.unlock();
}

void write_hello(shared_ptr<Connection> conn, int ttl, string flood, string sender_nodeid, int content_len)
{
    string h1 = "353NET/1.0 SAYHELLO\r\n";
    string h2 = "TTL: " + to_string(ttl) + "\r\n";
    string h3 = "Flood: " + flood + "\r\n";
    string h4 = "From: " + sender_nodeid + "\r\n";
    string h5 = "Content-Length: " + to_string(content_len) + "\r\n ";
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

void write_LSUPDATE(shared_ptr<Connection> conn, shared_ptr<Message> message)
{
    string h1 = "353NET/1.0 LSUPDATE\r\n";
    string h2 = "TTL: " + to_string(max_ttl) + "\r\n";
    string h3 = "Flood: 1;reason=" + to_string(message->get_flood_reason()) + "\r\n";
    string h4 = "MessageID: " + message->get_message_id() + "\r\n";
    string h5 = "From: " + message->get_origin_nodeid() + "\r\n";
    string h6 = "OriginStartTime: " + message->get_origin_start_time() + "\r\n";
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

void write_UCASTAPP(shared_ptr<Connection> conn, shared_ptr<Message> message, int next_layer)
{
    string h1 = "353NET/1.0 UCASTAPP\r\n";
    string h2 = "TTL: " + to_string(max_ttl) + "\r\n";
    string h3 = "Flood: 0\r\n";
    string h4 = "MessageID: " + message->get_message_id() + "\r\n";
    string h5 = "From: " + message->get_origin_nodeid() + "\r\n";
    string h6 = "To: " + message->get_target_nodeid() + "\r\n";
    string h7 = "Next-Layer: " + to_string(next_layer) + "\r\n";
    string h8 = "Content-Length: " + to_string(message->get_content_len()) + "\r\n";
    string h9 = "\r\n";
    string h10 = message->get_message_body();

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
    better_write(next_socketfd, h10.c_str(), h10.length());

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
    log_header(h10 + "\r\n", conn_number);
}
