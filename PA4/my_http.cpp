/*
 * HTTP Functions
 * Author: Scott Susanto
 */

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>

#include "my_http.h"
#include "my_reaper.h"
#include "my_readwrite.h"
#include "my_utils.h"

void await_http_request(shared_ptr<Connection> client_conn, vector<shared_ptr<Connection>> *conns)
{
    mut.lock();
    mut.unlock();

    int client_socketfd = client_conn->get_curr_socketfd();
    string client_ip_and_port = client_conn->get_ip_port();

    while (true)
    {
        client_conn->set_start_time();

        string line;
        int bytes_received = read_a_line(client_socketfd, line);
        if (bytes_received == -1)
            break;
        if (bytes_received <= 2)
            continue;

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
            Message error_message(404, "", "");
            client_conn->add_message_to_queue(make_shared<Message>(error_message));
            client_conn->set_reason("unexpectedly");
        }

        Message res(200, file_path, calc_md5(file_path));
        client_conn->add_message_to_queue(make_shared<Message>(res));
    }

    client_conn->lock();
    if (client_conn->is_alive())
    {
        shutdown(client_conn->get_orig_socketfd(), SHUT_RDWR);
        close(client_conn->get_orig_socketfd());
        client_conn->set_curr_socketfd(-2);
    }
    client_conn->unlock();
    send_to_reaper(client_conn);
}

void send_http_response(shared_ptr<Connection> client_conn, vector<shared_ptr<Connection>> *conns)
{
    mut.lock();
    mut.unlock();

    int conn_number = client_conn->get_conn_number();
    string client_ip_port = client_conn->get_ip_port();

    while (true)
    {
        shared_ptr<Message> message = client_conn->await_message_from_queue();
        if (message == NULL)
            break;

        int status_code = message->get_status_code();
        if (status_code != 200)
        {
            log("RESPONSE[" + to_string(conn_number) + "]: " + client_ip_port + ", status=" + to_string(status_code));
            write_res_headers(status_code, client_conn, "");
        }

        log("RESPONSE[" + to_string(conn_number) + "]: " + client_ip_port + ", status=" + to_string(status_code) + ", " + client_conn->get_shaper_params());
        write_res_headers(status_code, client_conn, message->get_md5_hash());
        write_res_body(client_conn, message->get_file_path());
    }

    log("Socket-writing thread has terminated");
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

        client_conn->lock();
        better_write(client_curr_socketfd, line, bytes_read);
        client_conn->unlock();

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
