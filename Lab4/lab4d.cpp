/* C++ standard include files first */
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <tuple>

using namespace std;

/* C system include files next */
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>

/* C standard include files next */
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

/* your own include last */
#include "my_socket.h"
#include "my_readwrite.h"

static int server_socketfd = (-1); /* there is nothing wrong with using a global variable */
static int gnDebug = 1;            /* change it to 0 if you don't want debugging messages */

void server(string);
tuple<int, int> parse_req_headers(int);
void write_res_headers(int, int);
void write_res_body(int, int);
void respond_to_http_client(int);

void client(string, string, string, string);
int parse_res_headers(int);
void write_req_headers(int, string, string, string, string);
int read_binary_data_to_file(int, int, string);

void usage();
int get_file_size(string);
tuple<int, vector<string>> choose_mode(int, char *[]);

int main(int argc, char *argv[])
{
    tuple<int, vector<string>> mode_tuple = choose_mode(argc, argv);

    int mode = get<0>(mode_tuple);
    if (mode == 1)
    {
        vector<string> server_args = get<1>(mode_tuple);
        string port_str = server_args[0];
        server(port_str);
    }
    else if (mode == 0)
    {
        vector<string> client_args = get<1>(mode_tuple);
        string host_str = client_args[0];
        string port_str = client_args[1];
        string uri = client_args[2];
        string filename = client_args[3];
        client(host_str, port_str, uri, filename);
    }

    return 0;
}

void server(string port_str)
{
    server_socketfd = create_listening_socket(port_str);
    if (server_socketfd == -1)
        return;

    while (true)
    {
        int client_socketfd = my_accept(server_socketfd);
        if (client_socketfd == -1)
            break;
        respond_to_http_client(client_socketfd);
    }

    shutdown(server_socketfd, SHUT_RDWR);
    close(server_socketfd);
}

void client(string host, string port, string uri, string filename)
{
    int client_socketfd = create_client_socket_and_connect(host, port);
    if (client_socketfd == -1)
    {
        cerr << "Cannot connect to " << host << ":" << port << endl;
        exit(-1);
    }

    write_req_headers(client_socketfd, host, port, "GET", uri);
    int content_len = parse_res_headers(client_socketfd);
    read_binary_data_to_file(client_socketfd, content_len, filename);
}

void respond_to_http_client(int server_socketfd)
{
    tuple<int, int> file_details = parse_req_headers(server_socketfd);
    int fd = get<0>(file_details), file_size = get<1>(file_details);
    write_res_headers(server_socketfd, file_size);
    write_res_body(server_socketfd, fd);
}

void write_req_headers(int client_socketfd, string host, string port, string req_type, string uri)
{
    string h1 = req_type + " " + uri + " " + "HTTP/1.1\r\n";
    string h2 = "User-Agent: lab4b\r\n";
    string h3 = "Accept: */*\r\n";
    string h4 = "Host: " + host + ":" + port + "\r\n";
    string h5 = "\r\n";

    better_write_header(client_socketfd, h1.c_str(), h1.length());
    better_write_header(client_socketfd, h2.c_str(), h2.length());
    better_write_header(client_socketfd, h3.c_str(), h3.length());
    better_write_header(client_socketfd, h4.c_str(), h4.length());
    better_write_header(client_socketfd, h5.c_str(), h5.length());

    cout << '\t' + h1 << '\t' + h2 << '\t' + h3 << '\t' + h4 << '\t' + h5 << endl;
}

void write_res_headers(int server_socketfd, int file_size)
{
    string h1 = "HTTP/1.1 200 OK\r\n";
    string h2 = "Server: lab4a\r\n";
    string h3 = "Content-Type: application/octet-stream\r\n";
    string h4 = "Content-Length: " + to_string(file_size) + "\r\n";
    string h5 = "\r\n";

    better_write_header(server_socketfd, h1.c_str(), h1.length());
    better_write_header(server_socketfd, h2.c_str(), h2.length());
    better_write_header(server_socketfd, h3.c_str(), h3.length());
    better_write_header(server_socketfd, h4.c_str(), h4.length());
    better_write_header(server_socketfd, h5.c_str(), h5.length());

    cout << '\t' + h1 << '\t' + h2 << '\t' + h3 << '\t' + h4 << '\t' + h5 << endl;
}

tuple<int, int> parse_req_headers(int server_socketfd)
{
    string line;
    int bytes_received = read_a_line(server_socketfd, line);

    stringstream stream(line);
    string req_type, filepath, version;
    stream >> req_type >> filepath >> version;

    if (req_type != "GET")
    {
        // cout << "Not a GET Request: " << req_type << endl;
        return make_tuple(0, 0);
    }

    // cout << "Request: " << req_type << ", Filepath: " << filepath << ", Version : " << version << endl;

    filepath = "lab4data" + filepath;
    int file_size = get_file_size(filepath);
    if (!file_size)
    {
        cout << "Zero file size: " << file_size << endl;
        return make_tuple(0, 0);
    }

    int fd = open(filepath.c_str(), O_RDONLY);
    if (!fd)
    {
        cout << "Unable to open file: " << filepath << endl;
        return make_tuple(0, 0);
    }

    string versionx = version.substr(0, version.length() - 1);
    if (versionx != "HTTP/1.")
    {
        cout << "Wrong version: " << version << endl;
        return make_tuple(0, 0);
    }

    while (bytes_received > 2)
    {
        bytes_received = read_a_line(server_socketfd, line);
    }

    return make_tuple(fd, file_size);
}

int parse_res_headers(int client_socketfd)
{
    string line;
    int content_len = 0;
    while (true)
    {
        int bytes_received = read_a_line(client_socketfd, line);
        if (bytes_received <= 2)
            break;

        cout << "\t" + line;
        stringstream stream(line);
        string header_key;
        stream >> header_key;

        if (header_key == "Content-Length:")
            stream >> content_len;
    }

    return content_len;
}

void write_res_body(int sockfd, int fd)
{
    int total_bytes_read = 0, bytes_read = 0;
    char line[4096];
    while ((bytes_read = read(fd, line, 4096)))
    {
        string line_str = string(line);
        better_write(sockfd, line_str.c_str(), bytes_read);
        total_bytes_read += bytes_read;
    }
}

void usage()
{
    cerr << "Please use the following format: lab4b [-c] HOST PORT URI OUTPUTFILE" << endl;
    exit(-1);
}

int get_file_size(string path)
{
    struct stat stat_buf;
    if (stat(path.c_str(), &stat_buf) != 0)
        return -1;
    return (int)stat_buf.st_size;
}

tuple<int, vector<string>> choose_mode(int argc, char *argv[])
{
    int is_client = 0;

    if (argc < 2)
        usage();

    if (strcmp(argv[1], "-c") == 0)
        is_client = 1;

    if (is_client)
    {
        if (argc != 6)
            usage();
        string host_str = argv[2];
        string port_str = argv[3];
        string uri = argv[4];
        if (uri[0] != '/')
            uri = "/" + uri;
        string filename = argv[5];
        vector<string> client_args = {host_str, port_str, uri, filename};
        return make_tuple(0, client_args);
    }
    else
    {
        if (argc != 2)
            usage();
        string port_str = argv[1];
        vector<string> server_args = {port_str};
        return make_tuple(1, server_args);
    }

    return make_tuple(-1, vector<string>{});
}

int read_binary_data_to_file(int client_socketfd, int bytes_expected, string filename)
{
    ofstream outfile(filename);
    if (!outfile.is_open())
    {
        cerr << "Unable to write to file: " << filename << endl;
        return -1;
    }

    char buf[1024];
    int bytes_left = bytes_expected;
    while (bytes_left > 0)
    {
        int bytes_to_read = (bytes_left > 1024) ? 1024 : bytes_left;
        int bytes_read = read(client_socketfd, buf, bytes_to_read);
        outfile.write(buf, bytes_read);

        if (bytes_read <= 0)
            return -1;

        bytes_left -= bytes_read;
    }

    outfile.close();
    return 0;
}