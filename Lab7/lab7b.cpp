/* C++ standard files */
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <thread>
#include <chrono>

using namespace std;

/* C++ helper files */
#include <string.h>

/* C system files*/
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>

/* C standard files */
#include <fcntl.h>
#include <openssl/md5.h>

/* Custom files */
#include "my_socket.h"
#include "my_readwrite.h"
#include "my_timestamp.h"

void run_server_with_logfile(string port, string logfile);
void serve_client(int, int);
map<string, string> get_config(string);
tuple<int, int, string> parse_req_headers_and_find_file(int, string);
void write_res_headers(int, int, int, string);
void write_res_body(int, string, string, int, double);

void run_client(vector<string>);
void write_req_headers(int, string, string, string, string);
tuple<int, string> parse_res_headers_and_get_content_len(int, string, string);
int write_data_to_file(int, int, string);

tuple<int, vector<string>> choose_mode(int, char *[]);
bool contains_complex_chars(string);
int get_file_size(string);
string calc_md5(string);
string hexDump(unsigned char *, unsigned long);

void log(string);
void log_header(string);
void usage();

bool with_config_file;
string rootdir = "";
ofstream logfile;

const int TOTAL_REQUESTS = 100;

int main(int argc, char *argv[])
{
    tuple<int, vector<string>> mode_tuple = choose_mode(argc, argv);

    int mode = get<0>(mode_tuple);
    if (mode == 1)
    {
        vector<string> server_args = get<1>(mode_tuple);
        string port = server_args.at(0), logfile = server_args.at(1);
        run_server_with_logfile(port, logfile);
    }
    else
    {
        vector<string> client_args = get<1>(mode_tuple);
        run_client(client_args);
    }

    return 0;
}

void run_server_with_logfile(string port, string logfile_name)
{
    logfile.open(logfile_name);

    int server_socketfd = create_listening_socket(port);
    if (server_socketfd == -1)
    {
        cerr << "Unable to create server socket" << endl;
        return;
    }

    rootdir = "lab4data";
    int connection_number = 0;
    thread threads[TOTAL_REQUESTS];

    while (true)
    {
        int client_socketfd = my_accept(server_socketfd);
        if (client_socketfd == -1)
        {
            cerr << "Unable to connect to client" << endl;
            continue;
        }
        connection_number++;
        threads[connection_number] = thread(serve_client, client_socketfd, connection_number);
    }

    shutdown(server_socketfd, SHUT_RDWR);
    close(server_socketfd);
}

void run_server_with_config_file(string config_file)
{
    map<string, string> config = get_config(config_file);

    ofstream pidfile(config["pidfile"]);
    if (!pidfile.is_open())
    {
        cerr << "Unable to open PID file: " << config["pidfile"] << endl;
        return;
    }
    pidfile << (int)getpid() << endl;
    pidfile.close();

    rootdir = config["rootdir"];
    string port = config["port"];

    logfile.open(config["logfile"]);
    // log("START: port=" + port + ", rootdir=" + rootdir);

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

        string client_ip_and_port = get_ip_and_port_for_client(client_socketfd, 0);
        // log("CONNECT: " + client_ip_and_port);

        while (true)
        {
            tuple<int, int, string> file_info = parse_req_headers_and_find_file(client_socketfd, client_ip_and_port);
            int status_code = get<0>(file_info);

            string err = get<2>(file_info);
            if (status_code != 200)
            {
                if (err != "")
                    cerr << err << endl;
                break;
            }

            // log("RESPONSE: " + client_ip_and_port + ", status=" + to_string(status_code));

            int file_size = get<1>(file_info);
            string file_path = get<2>(file_info);
            string md5_hash = calc_md5(file_path);

            // write_res_headers(client_socketfd, status_code, file_size, md5_hash);
            // write_res_body(client_socketfd, file_path);
        }

        // log("CLOSE: " + client_ip_and_port);
    }
}

void serve_client(int client_socketfd, int connection_number)
{
    const double SPEED = 1.0;
    string client_ip_and_port = get_ip_and_port_for_client(client_socketfd, 0);

    while (true)
    {
        tuple<int, int, string> file_info = parse_req_headers_and_find_file(client_socketfd, client_ip_and_port);
        int status_code = get<0>(file_info);

        // 3rd string is either a filename if it exists
        // or an error message if if does not exist
        string err = get<2>(file_info);
        if (status_code != 200)
        {
            if (err != "")
                cerr << err << endl;
            break;
        }

        int file_size = get<1>(file_info);
        string file_path = get<2>(file_info);
        string md5_hash = calc_md5(file_path);

        log("[" + to_string(connection_number) + "]\tClient connected from " + client_ip_and_port + " and requesting " + file_path);
        write_res_headers(client_socketfd, status_code, file_size, md5_hash);
        write_res_body(client_socketfd, file_path, client_ip_and_port, connection_number, SPEED);
    }

    log("[" + to_string(connection_number) + "]\tConnection closed with client at " + client_ip_and_port);
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

tuple<int, int, string> parse_req_headers_and_find_file(int client_socketfd, string client_ip_and_port)
{
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

    // log("REQUEST: " + client_ip_and_port + ", uri=/" + uri);

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

    // log_header(line);
    while (bytes_received > 2)
    {
        bytes_received = read_a_line(client_socketfd, line);
        // log_header(line);
    }

    if (err != "")
        return make_tuple(404, 0, err);
    return make_tuple(200, file_size, file_path);
}

void write_res_headers(int client_socketfd, int status_code, int file_size, string md5_hash)
{
    string status = "404 NOT FOUND\r\n";
    if (status_code == 200)
        status = "200 OK\r\n";

    string h1 = "HTTP/1.1 " + status;
    string h2 = "Server: pa2 (scottsus@usc.edu)\r\n";
    string h3 = "Content-Type: application/octet-stream\r\n";
    string h4 = "Content-Length: " + to_string(file_size) + "\r\n";
    string h5 = "Content-MD5: " + md5_hash + "\r\n";
    string h6 = "\r\n";

    better_write_header(client_socketfd, h1.c_str(), h1.length());
    better_write_header(client_socketfd, h2.c_str(), h2.length());
    better_write_header(client_socketfd, h3.c_str(), h3.length());
    better_write_header(client_socketfd, h4.c_str(), h4.length());
    better_write_header(client_socketfd, h5.c_str(), h5.length());
    better_write_header(client_socketfd, h6.c_str(), h6.length());

    // log_header(h1);
    // log_header(h2);
    // log_header(h3);
    // log_header(h4);
    // log_header(h5);
    // log_header("");
}

void write_res_body(int client_socketfd, string file_path, string client_ip_and_port, int connection_number, double speed)
{
    const int MEMORY_BUFFER = 1024;

    int fd = open(file_path.c_str(), O_RDONLY);
    if (!fd)
    {
        cerr << "Unable to open file in 'write_res_body'" << endl;
        return;
    }

    int P = 1, b1 = P;
    struct timeval start, now;
    gettimeofday(&start, NULL);

    int total_bytes_read = 0, bytes_read = 0, kilobytes_read = 0;
    char line[MEMORY_BUFFER];
    while ((bytes_read = read(fd, line, MEMORY_BUFFER)))
    {
        log("[" + to_string(connection_number) + "]\tSent " + to_string(kilobytes_read) + " KB to " + client_ip_and_port);
        better_write(client_socketfd, line, bytes_read);
        total_bytes_read += bytes_read;
        kilobytes_read++;
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
                int time_to_sleep = timestamp_diff_in_seconds(&later, &now);
                int usec_to_sleep = time_to_sleep * 1000000;
                usleep(usec_to_sleep);
            }
        }
    }
    // cout << "Total bytes sent: " << total_bytes_read << endl;
}

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
    cout << "pa2 client at " << client_ip_and_port << " connected to server at " << host << ":" << port << endl;

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

        return make_tuple(2, client_args);
    }

    if (argc < 3)
        usage();

    return make_tuple(1, vector<string>{argv[1], argv[2]});
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

int get_file_size(string file_path)
{
    struct stat stat_buf;
    if (stat(file_path.c_str(), &stat_buf) != 0)
        return -1;
    return (int)stat_buf.st_size;
}

string calc_md5(string file_path)
{
    const int MEMORY_BUFFER = 1024;

    int fd = open(file_path.c_str(), O_RDONLY);
    if (!fd)
    {
        cerr << "Unable to open file in 'calc_md5'" << endl;
        return "";
    }

    MD5_CTX md5_ctx;
    MD5_Init(&md5_ctx);

    int total_bytes_read = 0, bytes_read = 0;
    char line[MEMORY_BUFFER];
    while ((bytes_read = read(fd, line, MEMORY_BUFFER)))
    {
        string line_str = string(line);
        MD5_Update(&md5_ctx, line, bytes_read);
        total_bytes_read += bytes_read;
    }

    unsigned char md5_buf[MD5_DIGEST_LENGTH];
    MD5_Final(md5_buf, &md5_ctx);
    return hexDump(md5_buf, sizeof(md5_buf));
}

string hexDump(unsigned char *buf, unsigned long len)
{
    string s;
    static char hexchar[] = "0123456789abcdef";

    for (unsigned long i = 0; i < len; i++)
    {
        unsigned char ch = buf[i];
        unsigned int hi_nibble = (unsigned int)((ch >> 4) & 0x0f);
        unsigned int lo_nibble = (unsigned int)(ch & 0x0f);

        s += hexchar[hi_nibble];
        s += hexchar[lo_nibble];
    }
    return s;
}

void log(string message)
{
    string log = "[" + get_timestamp_now() + "] " + message + "\n";
    if (logfile.is_open())
    {
        logfile << log;
        logfile.flush();
    }
    else
        cout << log;
}

void log_header(string header)
{
    string log = "\t" + header;
    if (header == "")
        log = "\r\n";

    if (logfile.is_open())
    {
        logfile << log;
        logfile.flush();
    }
    else
        cout << log;
}

void usage()
{
    cerr << "Please use the following command: ./pa2 [-c] HOST PORT URI_1 FILE_1 [... URI_N FILE_N]" << endl;
    exit(-1);
}