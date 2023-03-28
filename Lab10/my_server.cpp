/*
 *  Server
 */

#include <iostream>
#include <fstream>
#include <mutex>
#include <map>
#include <algorithm>

#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "my_server.h"
#include "my_reaper.h"
#include "my_message.h"
#include "my_utils.h"

using namespace std;

void run_server(string config_file)
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
    thread neighbors_thread(find_neighbors, config["topology"][nodeid], &conns);

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
        client_conn->set_reader_thread(make_shared<thread>(thread(await_request, client_conn, &conns)));
        client_conn->set_writer_thread(make_shared<thread>(thread(send_response, nodeid, client_conn)));
        conns.push_back(client_conn);
        mut.unlock();
    }

    console_thread.join();
    reaper_thread.join();
    neighbors_thread.join();

    log("STOP: port=" + port);
}

void await_request(shared_ptr<Connection> client_conn, vector<shared_ptr<Connection>> *conns)
{
    mut.lock();
    mut.unlock();

    int client_socketfd = client_conn->get_curr_socketfd();
    string client_ip_and_port = client_conn->get_ip_port();

    bool is_http = true;
    while (true)
    {
        client_conn->set_start_time();

        string line;
        int bytes_received = read_a_line(client_socketfd, line);
        if (bytes_received <= 2)
            continue;

        stringstream ss(line);
        string method, uri, version;
        ss >> method >> uri >> version;

        if (method == "353NET/1.0")
        {
            is_http = false;
            break;
        }

        processHTTPRequest(client_socketfd, client_conn, method, uri, version);
    }

    if (!is_http)
        processP2PRequest(client_socketfd, client_conn, conns);

    mut.lock();
    if (client_conn->get_curr_socketfd() >= 0)
        shutdown(client_conn->get_orig_socketfd(), SHUT_RDWR);
    client_conn->set_curr_socketfd(-1);
    mut.unlock();

    client_conn->add_msg_to_queue(NULL);
    client_conn->get_writer_thread()->join();
    send_to_reaper(client_conn);
}

void processP2PRequest(int client_socketfd, shared_ptr<Connection> client_conn, vector<shared_ptr<Connection>> *conns)
{
    string line;
    for (int i = 0; i < 3; i++)
        read_a_line(client_socketfd, line);

    stringstream ss1(line);
    string from, message_sender_nodeid;
    ss1 >> from >> message_sender_nodeid;

    read_a_line(client_socketfd, line);
    stringstream ss2(line);
    string content_len_key, content_len_val;
    ss2 >> content_len_key >> content_len_val;

    int content_len = stoi(content_len_val);
    log_header("r", message_sender_nodeid, 1, 0, content_len);

    string receiver_nodeid = client_conn->get_neighbor_nodeid();

    bool is_duplicate_conn = false;
    if (receiver_nodeid == "")
    {
        mut.lock();
        for (shared_ptr<Connection> neighbor : *conns)
        {
            if (neighbor->get_conn_number() == client_conn->get_conn_number())
                continue;

            if (neighbor->get_neighbor_nodeid() == message_sender_nodeid)
                is_duplicate_conn = true;
        }
        mut.unlock();

        if (!is_duplicate_conn)
        {
            mut.lock();
            client_conn->set_neighbor_nodeid(message_sender_nodeid);
            string ip_port = get_ip_and_port_for_client(client_socketfd, 1);

            stringstream ss(ip_port);
            string ip, port;
            getline(ss, ip, ':');
            getline(ss, port);
            string nodeid = ":" + port;
            Message return_hello(nodeid, content_len);

            client_conn->add_msg_to_queue(make_shared<Message>(return_hello));
            mut.unlock();
        }
    }
    else
    {
        mut.lock();
        for (shared_ptr<Connection> neighbor : *conns)
        {
            if (neighbor->get_conn_number() == client_conn->get_conn_number())
                continue;

            if (neighbor->get_neighbor_nodeid() == message_sender_nodeid)
                is_duplicate_conn = true;
        }
        mut.unlock();
    }

    if (!is_duplicate_conn)
    {
        while (true)
        {
            string line;
            int bytes_received = read_a_line(client_socketfd, line);
            mut.lock();
            if (server_socketfd == -1 || bytes_received <= 0)
            {
                mut.unlock();
                break;
            }
            mut.unlock();
            // Process message
        }
    }
}

void processHTTPRequest(int client_socketfd, shared_ptr<Connection> client_conn, string method, string uri, string version)
{
    string err = "";
    if (method != "GET")
    {
        err = "Not a GET request: " + method;
    }

    if (uri[uri.length() - 1] == '/' || contains_complex_chars(uri))
    {
        err = "Malformed URI: " + uri;
    }

    if (uri[0] == '/')
        uri = uri.substr(1);
    client_conn->set_uri(uri);

    uint dot_idx = uri.find_last_of(".");
    string file_type = uri.substr(dot_idx + 1);
    client_conn->set_file_type(file_type);

    vector<int> throttlers = get_throttlers(file_type);
    client_conn->set_throttlers(throttlers);

    int conn_number = client_conn->get_conn_number();
    log("REQUEST[" + to_string(conn_number) + "]: " + client_conn->get_ip_port() + ", uri=/" + uri);

    string file_path = rootdir + "/" + uri;
    int file_size = get_file_size(file_path);
    if (file_size <= 0)
    {
        client_conn->set_content_len(0);
        err = "File size is zero for " + file_path;
    }
    client_conn->set_content_len(file_size);

    string versionx = version.substr(0, version.length() - 1);
    if (versionx != "HTTP/1.")
    {
        err = "Wrong HTTP version: " + version;
    }

    string line;
    log_header(line, conn_number);

    int bytes_received = 0;
    while (bytes_received > 2)
    {
        bytes_received = read_a_line(client_socketfd, line);
        log_header(line, conn_number);
    }

    if (err != "")
    {
        Message error_msg(404, "", "");
        client_conn->add_msg_to_queue(make_shared<Message>(error_msg));
        client_conn->set_reason("unexpectedly");
    }

    Message res(200, file_path, calc_md5(file_path));
    client_conn->add_msg_to_queue(make_shared<Message>(res));
    return;
}

void send_response(string sender_nodeid, shared_ptr<Connection> client_conn)
{
    mut.lock();
    mut.unlock();

    int conn_number = client_conn->get_conn_number();
    string client_ip_port = client_conn->get_ip_port();

    while (true)
    {
        shared_ptr<Message> msg = client_conn->await_msg_from_queue();
        if (msg == NULL)
            break;

        if (!msg->is_mode_http())
        {
            int content_len = msg->get_content_len();
            write_hello(client_conn, config["params"]["max_ttl"], "0", sender_nodeid, content_len);
            log_header("i", sender_nodeid, 1, 0, content_len);
            return;
        }

        int status_code = msg->get_status_code();
        if (status_code != 200)
        {
            log("RESPONSE[" + to_string(conn_number) + "]: " + client_ip_port + ", status=" + to_string(status_code));
            write_res_headers(status_code, client_conn, "");
        }

        log("RESPONSE[" + to_string(conn_number) + "]: " + client_ip_port + ", status=" + to_string(status_code) + ", " + client_conn->get_shaper_params());
        write_res_headers(status_code, client_conn, msg->get_md5_hash());
        write_res_body(client_conn, msg->get_file_path());
    }

    log("Socket-writing thread has terminated");
}

void write_hello(shared_ptr<Connection> neighbor_conn, string ttl, string flood, string sender_nodeid, int content_len)
{
    string h1 = "353NET/1.0 SAYHELLO\r\n";
    string h2 = "TTL: " + ttl + "\r\n";
    string h3 = "Flood: " + flood + "\r\n";
    string h4 = "From: " + sender_nodeid + "\r\n";
    string h5 = "Content-Length: " + to_string(content_len) + "\r\n ";
    string h6 = "\r\n";

    int neighbor_socketfd = neighbor_conn->get_orig_socketfd();
    better_write_header(neighbor_socketfd, h1.c_str(), h1.length());
    better_write_header(neighbor_socketfd, h2.c_str(), h2.length());
    better_write_header(neighbor_socketfd, h3.c_str(), h3.length());
    better_write_header(neighbor_socketfd, h4.c_str(), h4.length());
    better_write_header(neighbor_socketfd, h5.c_str(), h5.length());
    better_write_header(neighbor_socketfd, h6.c_str(), h6.length());

    int conn_number = neighbor_conn->get_conn_number();
    log_header(h1, conn_number);
    log_header(h2, conn_number);
    log_header(h3, conn_number);
    log_header(h4, conn_number);
    log_header(h5, conn_number);
    log_header(h6, conn_number);
}

void write_res_headers(int status_code, shared_ptr<Connection> client_conn, string md5_hash)
{
    string status = "404 NOT FOUND\r\n";
    if (status_code == 200)
        status = "200 OK\r\n";

    string content_type = "application/octet-stream\r\n";
    if (client_conn->get_file_type() == "html")
        content_type = "text/html\r\n";

    string h1 = "HTTP/1.1 " + status;
    string h2 = "Server: pa3 (scottsus@usc.edu)\r\n";
    string h3 = "Content-Type: " + content_type;
    string h4 = "Content-Length: " + to_string(client_conn->get_content_len()) + "\r\n";
    string h5 = "Content-MD5: " + md5_hash + "\r\n";
    string h6 = "\r\n";

    int client_socketfd = client_conn->get_orig_socketfd();
    better_write_header(client_socketfd, h1.c_str(), h1.length());
    better_write_header(client_socketfd, h2.c_str(), h2.length());
    better_write_header(client_socketfd, h3.c_str(), h3.length());
    better_write_header(client_socketfd, h4.c_str(), h4.length());
    better_write_header(client_socketfd, h5.c_str(), h5.length());
    better_write_header(client_socketfd, h6.c_str(), h6.length());

    int conn_number = client_conn->get_conn_number();
    log_header(h1, conn_number);
    log_header(h2, conn_number);
    log_header(h3, conn_number);
    log_header(h4, conn_number);
    log_header(h5, conn_number);
    log_header(h6, conn_number);
}

void write_res_body(shared_ptr<Connection> client_conn, string file_path)
{
    const int MEMORY_BUFFER = 1024;

    int fd = open(file_path.c_str(), O_RDONLY);
    if (!fd)
    {
        cerr << "Unable to open file in 'write_res_body'" << endl;
        return;
    }

    int P = client_conn->get_P(), b1 = P;
    struct timeval start, now;
    gettimeofday(&start, NULL);

    int total_bytes_read = 0, bytes_read = 0, kilobytes_read = 0;
    char line[MEMORY_BUFFER];
    while ((bytes_read = read(fd, line, MEMORY_BUFFER)))
    {
        int client_curr_socketfd = client_conn->get_curr_socketfd();
        if (client_curr_socketfd < 0)
            break;

        better_write(client_curr_socketfd, line, bytes_read);
        total_bytes_read += bytes_read;
        kilobytes_read++;
        client_conn->incr_kb();

        double speed = client_conn->get_speed();
        bool not_enough_tokens = true;
        while (not_enough_tokens)
        {
            gettimeofday(&now, NULL);
            int n = (int)(speed * timestamp_diff_in_seconds(&start, &now));
            if ((n > 1) || (b1 == P && b1 - P + n >= P) || (b1 < P && b1 + n >= P))
            {
                struct timeval temp;
                add_seconds_to_timestamp(&start, (1 / speed), &temp);
                start = temp;
                b1 = P;
                not_enough_tokens = false;
            }
            else
            {
                b1 = 0;
                struct timeval later;
                add_seconds_to_timestamp(&start, (1 / speed), &later);
                long time_to_sleep = timestamp_diff_in_seconds(&now, &later);
                long usec_to_sleep = time_to_sleep * 1000000;
                if (usec_to_sleep > 250000)
                    usleep(250000);
                else if (usec_to_sleep > 0)
                    usleep(usec_to_sleep);
            }
        }
    }

    if (client_conn->get_reason().length() == 0)
        client_conn->set_reason("done");
    client_conn->reset_kb();
}
