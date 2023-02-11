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

static int gnDebug = 1; /* change it to 0 if you don't want debugging messages */

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

    cout << '\t' + h1 << '\t' + h2 << '\t' + h3 << '\t' + h4 << '\t' + h5 << endl;
}

static void write_body(int sockfd, int fd)
{
    int bytes_read = 0;
    char line[4096];
    while ((bytes_read = read(fd, line, 4096)))
        better_write(sockfd, string(line).c_str(), bytes_read);
}

static void respond_to_http_client(int newsockfd)
{
    int header_count = 0;
    string line;
    int bytes_received = read_a_line(newsockfd, line);
    if (gnDebug && header_count == 0)
        cerr << "[DBG-SVR] " << dec << bytes_received << " bytes received from " << get_ip_and_port_for_server(newsockfd, 0) << endl;
    header_count = (header_count + 1) % 5;

    stringstream stream(line);
    string req_type, filepath, version;
    stream >> req_type >> filepath >> version;

    if (req_type != "GET")
    {
        // cout << "Not a GET Request: " << req_type << endl;
        return;
    }

    // cout << "Request: " << req_type << ", Filepath: " << filepath << ", Version : " << version << endl;

    filepath = "lab4data" + filepath;
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
    cout << "WRITING" << endl;

    while (bytes_received > 2)
    {
        bytes_received = read_a_line(newsockfd, line);
    }

    write_headers(newsockfd, file_size);
    write_body(newsockfd, fd);
}

int main(int argc, char *argv[])
{
    int listen_socket_fd = create_listening_socket(argv[1]);
    if (listen_socket_fd == -1)
        return 0;

    while (true)
    {
        int newsockfd = my_accept(listen_socket_fd);
        if (newsockfd == -1)
            break;
        respond_to_http_client(newsockfd);
    }

    shutdown(listen_socket_fd, SHUT_RDWR);
    close(listen_socket_fd);
    return 0;
}
