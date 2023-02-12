/* C++ standard files */
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <tuple>
#include <map>

using namespace std;

/* C++ helper files */
#include <string.h>

/* C system files*/
#include <unistd.h>
#include <sys/stat.h>

/* C standard files */
#include <fcntl.h>

/* CUstom files */
#include "my_socket.h"
#include "my_readwrite.h"
#include "my_timestamp.h"

void run_server(string);
map<string, string> get_config(string);
tuple<int, int, int> parse_req_headers_and_find_file(int);
void write_res_headers(int, int, int, int);
void write_res_body(int, int);

void run_client(vector<string>);

tuple<int, vector<string>> choose_mode(int, char *[]);
bool contains_complex_chars(string);
int get_file_size(string);

void log(string);
void log_header(string);
void usage();

string rootdir = "";
ofstream logfile;

int main(int argc, char *argv[])
{
    tuple<int, vector<string>> mode_tuple = choose_mode(argc, argv);

    int mode = get<0>(mode_tuple);
    if (mode == 0)
    {
        vector<string> server_args = get<1>(mode_tuple);
        string config_file = server_args.at(0);
        run_server(config_file);
    }
    else if (mode == 1)
    {
        vector<string> client_args = get<1>(mode_tuple);
        run_client(client_args);
    }

    return 0;
}

void run_server(string config_file)
{
    map<string, string> config = get_config(config_file);
    rootdir = config["rootdir"];
    string port = config["port"];

    logfile.open(config["logfile"]);
    log("START: port=" + port + ", rootdir=" + rootdir);

    int server_socketfd = create_listening_socket(port);
    if (server_socketfd == 0)
    {
        cerr << "Unable to open server socket at: " << port << endl;
        return;
    }

    while (true)
    {
        int client_socketfd = my_accept(server_socketfd);
        if (client_socketfd == 0)
        {
            cerr << "Unable to connect to client" << endl;
            continue;
        }

        string client_ip_and_port = get_ip_and_port_for_client(client_socketfd, 1);
        log("CONNECT: " + client_ip_and_port);

        while (true)
        {
            tuple<int, int, int> file_info = parse_req_headers_and_find_file(client_socketfd);
            int status_code = get<0>(file_info);

            // custom STATUS_CODE signifying
            // we are done with client
            if (status_code == 0)
                break;

            int fd = get<1>(file_info), file_size = get<2>(file_info);

            log("RESPONSE: " + client_ip_and_port + ", status=" + to_string(status_code));

            write_res_headers(client_socketfd, status_code, fd, file_size);
            write_res_body(client_socketfd, fd);
        }

        log("CLOSE: " + client_ip_and_port);
    }
}

map<string, string> get_config(string config_file)
{
    ifstream file(config_file);
    if (!file.is_open())
    {
        cerr << "Unable to open file: " << config_file << endl;
        exit(-1);
    }

    map<string, string> config{};

    string line;
    while (getline(file, line))
    {
        if (line.empty() || line[0] == ';')
            continue;

        if (line[0] == '[')
        {
            if (line[line.length() - 1] == ']')
                continue;

            string section_name = line.substr(1, line.length() - 2);
            config["section"] = section_name;
        }

        stringstream ss(line);

        string key;
        while (getline(ss, key, '='))
        {
            string val = "";
            getline(ss, val);
            config[key] = val;
        }
    }

    return config;
}

tuple<int, int, int> parse_req_headers_and_find_file(int client_socketfd)
{
    string line;
    int bytes_received = read_a_line(client_socketfd, line);
    if (bytes_received <= 2)
        return make_tuple(0, 0, 0);

    cout << "\t" + line;

    stringstream ss(line);
    string method, uri, version;
    ss >> method >> uri >> version;

    if (method != "GET")
    {
        cerr << "Not a GET request: " << method << endl;
        return make_tuple(404, 0, 0);
    }

    if (uri[0] != '/' || uri[uri.length() - 1] == '/' || contains_complex_chars(uri))
    {
        cerr << "Malformed URI: " << uri << endl;
        return make_tuple(404, 0, 0);
    }

    string filepath = rootdir + "/" + uri;
    int file_size = get_file_size(filepath);
    if (file_size <= 0)
    {
        cerr << "File size is zero for " << filepath << endl;
        return make_tuple(404, 0, 0);
    }

    int fd = open(filepath.c_str(), O_RDONLY);
    if (!fd)
    {
        cerr << "Unable to open file: " << filepath << endl;
        return make_tuple(404, 0, 0);
    }

    string versionx = version.substr(0, version.length() - 1);
    if (versionx != "HTTP/1.")
    {
        cerr << "Wrong HTTP version: " << version << endl;
        return make_tuple(404, 0, 0);
    }

    while (bytes_received > 2)
    {
        bytes_received = read_a_line(client_socketfd, line);
        cout << "\t" + line;
    }

    return make_tuple(200, fd, file_size);
}

void write_res_headers(int client_socketfd, int status_code, int fd, int file_size)
{
    string md5_hash = "";

    string status = " 404 NOT FOUND\r\n";
    if (status_code == 200)
        status = " 200 OK\r\n";

    string h1 = "HTTP/1.1" + status;
    string h2 = "Server: pa2 (scottsus@usc.edu)\r\n";
    string h3 = "Content-Type: application/octet-stream\r\n";
    string h4 = "Content-Length: " + to_string(file_size) + "\r\n";
    string h5 = "Content-MD5: " + md5_hash + "\r\n";

    better_write_header(client_socketfd, h1.c_str(), h1.length());
    better_write_header(client_socketfd, h2.c_str(), h2.length());
    better_write_header(client_socketfd, h3.c_str(), h3.length());
    better_write_header(client_socketfd, h4.c_str(), h4.length());
    better_write_header(client_socketfd, h5.c_str(), h5.length());

    log_header(h1);
    log_header(h2);
    log_header(h3);
    log_header(h4);
    log_header(h5);
    log_header("");
}

void write_res_body(int client_socketfd, int fd)
{
    const int MEMORY_BUFFER = 1024;

    int total_bytes_read = 0, bytes_read = 0;
    char line[MEMORY_BUFFER];
    while ((bytes_read = read(fd, line, MEMORY_BUFFER)))
    {
        string line_str = string(line);
        better_write(client_socketfd, line_str.c_str(), sizeof(line));
        total_bytes_read += bytes_read;
        usleep(250000); // sleep for 250ms
    }
}

void run_client(vector<string> client_args)
{
}

tuple<int, vector<string>> choose_mode(int argc, char *argv[])
{
    bool is_client = false;

    if (argc < 2)
        usage();

    if (strcmp(argv[1], "-c") == 0)
        is_client = true;

    if (is_client)
    {

        if (argc < 6 || argc % 2 != 0)
            usage();

        string host = argv[2];
        string port = argv[3];
        vector<string> client_args = {host, port};

        for (int i = 4; i < argc; i += 2)
        {
            string uri = argv[i], filename = argv[i + 1];
            client_args.push_back(uri);
            client_args.push_back(filename);
        }

        return make_tuple(1, client_args);
    }
    else
    {
        if (argc != 2)
            usage();

        return make_tuple(0, vector<string>{argv[1]});
    }
}

bool contains_complex_chars(string uri)
{
    for (uint i = 0; i < uri.length(); i++)
    {
        char ch = uri[i];
        if (ch == '?' || ch == '#')
            return true;
    }
    return false;
}

int get_file_size(string filepath)
{
    struct stat stat_buf;
    if (stat(filepath.c_str(), &stat_buf) != 0)
        return -1;
    return (int)stat_buf.st_size;
}

void log(string message)
{
    string log = "[" + get_timestamp_now() + "] " + message + "\n";
    if (logfile.is_open())
        logfile << log;
    else
        cout << log;
}

void log_header(string header)
{
    string log = "\t" + header;
    if (header == "")
        log = "\r\n";

    if (logfile.is_open())
        logfile << log;
    else
        cout << log;
}

void usage()
{
    cerr << "Please use the following command: ./pa2 [-c] HOST PORT URI_1 FILE_1 [... URI_N FILE_N]" << endl;
    exit(-1);
}