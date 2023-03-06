/*
 *  Server
 */

#include <iostream>
#include <fstream>
#include <mutex>
#include <map>

#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "my_server.h"
#include "my_utils.h"

using namespace std;

void run_server(string config_file)
{
    config = get_config(config_file);
    map<string, string> startup_opts = config["startup"];

    ofstream pidfile(startup_opts["pidfile"]);
    if (!pidfile.is_open())
    {
        cerr << "Unable to open PID file: " << startup_opts["pidfile"] << endl;
        return;
    }
    pidfile << (int)getpid() << endl;
    pidfile.close();

    rootdir = startup_opts["rootdir"];
    string port = startup_opts["port"];

    logfile.open(startup_opts["logfile"]);
    log("START: port=" + port + ", rootdir=" + rootdir);

    server_socketfd = create_listening_socket(port);
    if (server_socketfd == -1)
    {
        cerr << "Unable to create server socket" << endl;
        return;
    }
    string server_ip_and_port = get_ip_and_port_for_server(server_socketfd, 1);

    int conn_number = 1;
    thread console_thread(handle_console, &conns);
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
        shared_ptr<Connection> conn_ptr = make_shared<Connection>(Connection(conn_number++, client_socketfd));
        conn_ptr->set_conn_thread(make_shared<thread>(thread(serve_client, conn_ptr)));
        conns.push_back(conn_ptr);
        mut.unlock();
    }

    console_thread.join();
    reaper_thread.join();

    log("STOP: port=" + port);
}

void handle_console(vector<shared_ptr<Connection>> *conns)
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
            shared_ptr<Connection> conn = NULL;

            mut.lock();
            for (shared_ptr<Connection> c : *conns)
                if (c->get_conn_number() == target_conn_number)
                    conn = c;
            mut.unlock();

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
            cout << "Token rate at " << conn->get_speed_str() << " tokens/s. ";

            log("Shaper-Params[" + to_string(conn->get_conn_number()) + "]: " + conn->get_shaper_params());
        }
        else if (cmd == "close")
        {
            string target_conn_str;
            cin >> target_conn_str;
            int target_conn = stoi(target_conn_str);

            mut.lock();
            shared_ptr<Connection> conn = NULL;
            for (shared_ptr<Connection> c : *conns)
                if (c->get_conn_number() == target_conn)
                    conn = c;
            mut.unlock();

            if (!conn)
            {
                cout << "No such connection: " << target_conn << endl;
                continue;
            }

            shutdown(conn->get_orig_socketfd(), SHUT_RDWR);
            conn->set_reason("at user's request");
            conn->set_curr_socketfd(-2);
            cout << "Closing connection " << to_string(conn->get_conn_number()) << " ..." << endl;
            mut.unlock();
        }
        else
        {
            cout << "Command not recognized. Valid commands are:" << endl;
            cout << "\tclose #" << endl;
            cout << "\tdial # percent" << endl;
            cout << "\tquit" << endl;
            cout << "\tstatus" << endl;
        }
    }
    mut.lock();
    shutdown(server_socketfd, SHUT_RDWR);
    close(server_socketfd);
    server_socketfd = -1;

    for (shared_ptr<Connection> c : *conns)
    {
        if (c->is_alive())
        {
            shutdown(c->get_orig_socketfd(), SHUT_RDWR);
            c->set_curr_socketfd(-2);
        }
    }
    mut.unlock();
    return;
}

void reap_threads(vector<shared_ptr<Connection>> *conns)
{
    while (true)
    {
        usleep(250000);
        mut.lock();
        if (server_socketfd == -1 && conns->empty())
        {
            mut.unlock();
            return;
        }

        for (vector<shared_ptr<Connection>>::iterator itr = conns->begin(); itr != conns->end();)
        {
            shared_ptr<Connection> conn = (*itr);
            if (conn->get_curr_socketfd() == -1)
            {
                conn->get_conn_thread()->join();
                itr = conns->erase(itr);
            }
            else
                itr++;
        }
        mut.unlock();
    }
}

void serve_client(shared_ptr<Connection> client_conn)
{
    mut.lock();
    mut.unlock();

    int client_socketfd = client_conn->get_curr_socketfd();
    int conn_number = client_conn->get_conn_number();
    string client_ip_and_port = client_conn->get_ip_port();

    while (true)
    {
        tuple<int, int, string> file_info = process_request(client_socketfd, client_conn);
        int status_code = get<0>(file_info);

        // 3rd string is either a filename if it exists
        // or an error message if if does not exist
        string err = get<2>(file_info);
        if (status_code != 200)
        {
            if (err != "")
            {
                client_conn->set_reason("unexpectedly");
                cerr << err << endl;
            }
            break;
        }

        int file_size = get<1>(file_info);
        string file_path = get<2>(file_info);
        string md5_hash = calc_md5(file_path);
        client_conn->set_content_len(file_size);

        if (status_code != 200)
            log("RESPONSE[" + to_string(conn_number) + "]: " + client_ip_and_port + ", status=" + to_string(status_code));
        else
            log("RESPONSE[" + to_string(conn_number) + "]: " + client_ip_and_port + ", status=" + to_string(status_code) + ", " + client_conn->get_shaper_params());

        write_res_headers(status_code, client_conn, md5_hash);
        write_res_body(client_conn, file_path);
    }

    shutdown(client_socketfd, SHUT_RDWR);
    close(client_socketfd);

    mut.lock();
    client_conn->set_curr_socketfd(-1);
    log("CLOSE[" + to_string(conn_number) + "]: (" + client_conn->get_reason() + ") " + client_ip_and_port);
    mut.unlock();
}

tuple<int, int, string> process_request(int client_socketfd, shared_ptr<Connection> client_conn)
{
    client_conn->set_start_time();

    string line;
    int bytes_received = read_a_line(client_socketfd, line);
    if (bytes_received <= 2)
        return make_tuple(0, 0, "");

    stringstream ss(line);
    string method, uri, version;
    ss >> method >> uri >> version;

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
        err = "File size is zero for " + file_path;
    }

    string versionx = version.substr(0, version.length() - 1);
    if (versionx != "HTTP/1.")
    {
        err = "Wrong HTTP version: " + version;
    }

    log_header(line, conn_number);
    while (bytes_received > 2)
    {
        bytes_received = read_a_line(client_socketfd, line);
        log_header(line, conn_number);
    }

    if (err != "")
        return make_tuple(404, 0, err);
    return make_tuple(200, file_size, file_path);
}

void write_res_headers(int status_code, shared_ptr<Connection> client_conn, string md5_hash)
{
    string status = "404 NOT FOUND\r\n";
    if (status_code == 200)
        status = "200 OK\r\n";

    string h1 = "HTTP/1.1 " + status;
    string h2 = "Server: pa3 (scottsus@usc.edu)\r\n";
    string h3 = "Content-Type: application/octet-stream\r\n";
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
        mut.lock();
        better_write(client_conn->get_curr_socketfd(), line, bytes_read);
        total_bytes_read += bytes_read;
        kilobytes_read++;
        client_conn->incr_kb();
        mut.unlock();

        bool not_enough_tokens = true;
        while (not_enough_tokens)
        {
            double speed = client_conn->get_speed();
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
                int time_to_sleep = timestamp_diff_in_seconds(&later, &now);
                int usec_to_sleep = time_to_sleep * 1000000;
                if (usec_to_sleep > 250000)
                    usleep(250000);
                else
                    usleep(usec_to_sleep);
            }
        }
    }

    if (client_conn->get_reason().length() == 0)
        client_conn->set_reason("done");
}
