/* C++ standard include files first */
#include <iostream>
#include <iomanip>
#include <string>

using namespace std;

/* C system include files next */
#include <arpa/inet.h>
#include <netdb.h>

/* C standard include files next */
#include <unistd.h>
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

/**
 * This is the function you need to change to change the behavior of your server!
 *
 * @param newsockfd - socket that can be used to "talk" (i.e., read/write) to the client.
 */
static void talk_to_client(int newsockfd)
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
        talk_to_client(newsockfd);
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
