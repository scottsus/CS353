/* C++ standard include files first */
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>

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

static int listen_socket_fd = (-1); /* there is nothing wrong with using a global variable */
static int gnDebug = 1;             /* change it to 0 if you don't want debugging messages */

static void usage()
{
    cerr << "usage: ./echo-server port_number" << endl;
    exit(-1);
}

static int get_file_size(string path)
{
    struct stat stat_buf;
    if (stat(path.c_str(), &stat_buf) != 0)
        return -1;
    return (int)stat_buf.st_size;
}

static void write_headers(int sockfd, int file_size)
{
    string h1 = "HTTP/1.1 200 OK\r\n";
    string h2 = "Server: lab4a\r\n";
    string h3 = "Content-Type: application/octet-stream\r\n";
    string h4 = "Content-Length: " + to_string(file_size) + "\r\n";
    string h5 = "\r\n";

    better_write_header(sockfd, h1.c_str(), h1.length());
    better_write_header(sockfd, h2.c_str(), h2.length());
    better_write_header(sockfd, h3.c_str(), h3.length());
    better_write_header(sockfd, h4.c_str(), h4.length());
    better_write_header(sockfd, h5.c_str(), h5.length());
}

static void write_body(int sockfd, int fd)
{
    int total_bytes_read = 0, bytes_read = 0;
    char line[4096];
    while ((bytes_read = read(fd, line, 4096)))
    {
        string line_str = string(line);
        cout << bytes_read << " bytes read from file" << endl;
        better_write(sockfd, line_str.c_str(), bytes_read);
        total_bytes_read += bytes_read;
    }
    cout << "Total bytes sent: " << total_bytes_read << endl;
}

static int choose(int argc)
{
    if (argc < 2 || argc > 3)
        usage();
    else if (argc == 2)
        return 0;
    return 1;
}

/**
 * This is the function you need to change to change the behavior of your server!
 * Returns non-zero if succeeds.
 * Otherwise, return 0;
 *
 * @param argc - number of arguments in argv.
 * @param argv - array of argc C-strings, must only use array index >= 0 and < argc.
 */
static string process_options(int argc, char *argv[])
{
    if (gnDebug)
    {
        for (int i = 0; i < argc; i++)
        {
            cerr << "[DBG-SVR]\targv[" << i << "]: '" << argv[i] << "'" << endl;
        }
    }
    /* incomplete, just demonstrate how to use argc */
    if (argc < 2 || argc > 3)
        usage();
    else if (argc == 2)
        return argv[1];
    return argv[2];
}

static void respond_to_http_client(int newsockfd)
{
    while (true)
    {
        string line;
        int bytes_received = read_a_line(newsockfd, line);

        if (gnDebug)
        {
            cerr << "[DBG-SVR] " << dec << bytes_received << " bytes received from " << get_ip_and_port_for_server(newsockfd, 0) << " (data displayed in next line, <TAB>-indented):\n\t";
            better_write(2, line.c_str(), bytes_received);
            if (!(bytes_received > 0 && line[bytes_received - 1] == '\n'))
                cerr << endl;
        }

        stringstream stream(line);
        string req_type, filepath, version;
        stream >> req_type >> filepath >> version;

        if (req_type != "GET")
        {
            // cout << "Not a GET Request: " << req_type << endl;
            return;
        }

        cout << "Request: " << req_type << ", Filepath: " << filepath << ", Version : " << version << endl;

        filepath = "." + filepath;
        int file_size = get_file_size(filepath);
        if (!file_size)
        {
            cout << "Zero file size: " << file_size << endl;
            return;
        }

        int fd = open(filepath.c_str(), O_RDONLY);
        if (!fd)
        {
            cout << "Unable to open file: " << filepath << endl;
            return;
        }

        string versionx = version.substr(0, version.length() - 1);
        if (versionx != "HTTP/1.")
        {
            cout << "Wrong version: " << version << endl;
            return;
        }

        write_headers(newsockfd, file_size);
        write_body(newsockfd, fd);
    }
}

/**
 * This is the function you need to change to change the behavior of your server!
 *
 * @param newsockfd - socket that can be used to "talk" (i.e., read/write) to the client.
 */
static void talk_to_message_client(int newsockfd)
{
    while (true)
    {
        string line;
        int bytes_received = read_a_line(newsockfd, line);
        if (bytes_received == 1)
            break;
        if (gnDebug)
        {
            cerr << "[DBG-SVR] " << dec << bytes_received << " bytes received from " << get_ip_and_port_for_server(newsockfd, 0) << " (data displayed in next line, <TAB>-indented):\n\t";
            better_write(2, line.c_str(), bytes_received);
            if (!(bytes_received > 0 && line[bytes_received - 1] == '\n'))
                cerr << endl;
            if (bytes_received > 0)
            {
                cerr << "\t";
                for (int i = 0; i < bytes_received; i++)
                {
                    cerr << "0x" << setfill('0') << setw(2) << hex << (int)line[i] << " ";
                }
                cerr << endl;
            }
        }
        better_write(newsockfd, line.c_str(), bytes_received);
    }

    shutdown(newsockfd, SHUT_RDWR);
    close(newsockfd);
}

static void talk_to_user_and_server(int client_socket_fd)
{
    cout << "Enter a string to send to the echo server: ";
    string message;

    while (true)
    {
        getline(cin, message);
        message += '\n';
        int bytes_sent = better_write(client_socket_fd, message.c_str(), message.length());
        cout << bytes_sent << " bytes sent from " << get_ip_and_port_for_client(client_socket_fd, 1) << endl;
        if (message.length() == 1 && message[0] == '\n')
            break;

        string line;
        int bytes_received = read_a_line(client_socket_fd, line);
        if (bytes_received == -1)
            break;
        cout << bytes_received << " bytes received, see next line for data..." << endl
             << line;
    }

    shutdown(client_socket_fd, SHUT_RDWR);
    close(client_socket_fd);
}

static void client(string port_str)
{
    int client_socket_fd = create_client_socket_and_connect(LOCALHOST, port_str);
    if (client_socket_fd == -1)
    {
        cerr << "Cannot connect to " << LOCALHOST << ":" << port_str << endl;
        exit(-1);
    }
    string client_ip_and_port = get_ip_and_port_for_client(client_socket_fd, 1);
    string server_ip_and_port = get_ip_and_port_for_client(client_socket_fd, 0);
    cerr << "echo-client at " << client_ip_and_port << " is connected to server at " << server_ip_and_port << endl;
    talk_to_user_and_server(client_socket_fd);
}

static void server(string port_str)
{
    listen_socket_fd = create_listening_socket(port_str);
    if (listen_socket_fd == -1)
        return;
    while (true)
    {
        int newsockfd = my_accept(listen_socket_fd);
        if (newsockfd == -1)
            break;
        respond_to_http_client(newsockfd);
    }
    shutdown(listen_socket_fd, SHUT_RDWR);
    close(listen_socket_fd);
}

int main(int argc, char *argv[])
{
    int option = choose(argc);
    string port_str = process_options(argc, argv);
    if (option == 0)
    {
        server(port_str);
    }
    else if (option == 1)
    {
        client(port_str);
    }
    return 0;
}
