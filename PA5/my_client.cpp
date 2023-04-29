/*
 *  Client
 */

#include <iostream>
#include <fstream>
#include <sstream>

#include <unistd.h>

#include "my_client.h"
#include "my_readwrite.h"
#include "my_socket.h"

void run_client(vector<string> client_args)
{
    string host = client_args.at(0), port = client_args.at(1);
    int client_socketfd = create_client_socket_and_connect(host, port);
    if (client_socketfd == -1)
    {
        cerr << "Failed to connect to server at " << host << ":" << port << ". Program terminated" << endl;
        return;
    }

    string client_ip_and_port = get_ip_and_port_for_client(client_socketfd, 1);
    cout << "pa3 client at " << client_ip_and_port << " connected to server at " << host << ":" << port << endl;

    for (uint i = 2; i < client_args.size(); i += 2)
    {
        string uri = client_args.at(i), filename = client_args.at(i + 1);
        write_req_headers(client_socketfd, host, port, "GET", uri);

        tuple<int, string> content_len_err = parse_res_headers_and_get_content_len(client_socketfd, client_ip_and_port, uri);
        int content_len = get<0>(content_len_err);
        string err = get<1>(content_len_err);
        if (content_len == 0)
            err = "No Content-Length in response header when downloading " + uri + " from server at " + client_ip_and_port + ". Program terminated";

        if (err != "")
        {
            cerr << err << endl;
            return;
        }

        int data_written = write_data_to_file(client_socketfd, content_len, filename);
        if (data_written == 0)
            cout << uri << " saved into " << filename << endl
                 << endl;
    }
}

void write_req_headers(int client_socketfd, string host, string port, string method, string uri)
{
    if (uri[0] != '/')
        uri = '/' + uri;
    string h1 = "GET " + uri + " HTTP/1.1\r\n";
    string h2 = "Host: " + host + ":" + port + "\r\n";
    string h3 = "Accept: text/html, */*\r\n";
    string h4 = "User-Agent: pa2 (scottsus@usc.edu)\r\n";
    string h5 = "\r\n";

    better_write_header(client_socketfd, h1.c_str(), h1.length());
    better_write_header(client_socketfd, h2.c_str(), h2.length());
    better_write_header(client_socketfd, h3.c_str(), h3.length());
    better_write_header(client_socketfd, h4.c_str(), h4.length());
    better_write_header(client_socketfd, h5.c_str(), h5.length());

    cout << "\t" + h1 << "\t" + h2 << "\t" + h3 << "\t" + h4 << "\t" + h5;
}

tuple<int, string> parse_res_headers_and_get_content_len(int client_socketfd, string client_ip_and_port, string uri)
{
    int content_len = 0;
    string err = "";

    bool is_first_header = true;
    while (true)
    {
        string line;
        int bytes_received = read_a_line(client_socketfd, line);
        if (bytes_received <= 2)
        {
            cout << "\r\n";
            break;
        }

        cout << "\t" + line;
        stringstream ss(line);

        if (is_first_header)
        {
            string version, status, reason;
            ss >> version >> status >> reason;

            string versionx = version.substr(0, 5);
            if (versionx != "HTTP/")
            {
                err = "Unrecognized response from server at " + client_ip_and_port + ". Program terminated";
            }

            if (stoi(status) != 200)
            {
                err = "Failed to send request for " + uri + " to server at " + client_ip_and_port;
            }

            is_first_header = false;
        }

        string header_key;
        ss >> header_key;
        if (header_key == "Content-Length:")
            ss >> content_len;
    }

    return make_tuple(content_len, err);
}

int write_data_to_file(int client_socketfd, int bytes_expected, string filename)
{
    ofstream outfile(filename);
    if (!outfile.is_open())
    {
        cerr << "Cannot open " << filename << " for writing. Download for " << filename << " is skipped." << endl;
        return -1;
    }

    const int BUFFER_SIZE = 1024;

    char buf[BUFFER_SIZE];
    int bytes_left = bytes_expected;
    while (bytes_left > 0)
    {
        int bytes_to_read = (bytes_left > BUFFER_SIZE) ? BUFFER_SIZE : bytes_left;
        int bytes_read = read(client_socketfd, buf, bytes_to_read);
        outfile.write(buf, bytes_read);

        // Indication of error
        if (bytes_read <= 0)
            return -1;

        bytes_left -= bytes_read;
    }

    outfile.close();
    return 0;
}
