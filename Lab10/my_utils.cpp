/*
 *  Utility functions
 */

#include <iostream>

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/md5.h>

#include "my_utils.h"

using namespace std;

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

    if (argc <= 2)
        return make_tuple(1, vector<string>{argv[1]});

    return make_tuple(0, vector<string>{argv[1], argv[2]});
}

map<string, map<string, string>> get_config(string config_file)
{
    ifstream file(config_file);
    if (!file.is_open())
    {
        cerr << "Unable to open file: " << config_file << endl;
        exit(-1);
    }

    map<string, map<string, string>> config{};

    string line;
    while (getline(file, line))
    {
        if (line.empty() || line[0] == ';')
            continue;

        string section_name = "";
        if (line[0] == '[')
        {
            if (line[line.length() - 1] != ']')
                continue;

            section_name = line.substr(1, line.length() - 2);
            config[section_name] = map<string, string>{};
        }

        while (getline(file, line))
        {
            if (line.length() <= 1)
                break;

            stringstream ss(line);
            string key, val;
            getline(ss, key, '=');
            getline(ss, val);
            config[section_name][key] = val;
        }
    }

    return config;
}

vector<int> get_throttlers(string file_type)
{
    if (mode == 0)
        return vector<int>{2, 5, 100};

    string section = "*";
    if (config.count(file_type))
        section = file_type;

    int P = stoi(config[section]["P"]);
    int MAXR = stoi(config[section]["MAXR"]);
    int DIAL = stoi(config[section]["DIAL"]);

    return vector<int>{P, MAXR, DIAL};
}

shared_ptr<Connection> find_conn(int conn_number, vector<shared_ptr<Connection>> *conns)
{
    shared_ptr<Connection> conn = NULL;
    mut.lock();
    for (shared_ptr<Connection> c : *conns)
        if (c->get_conn_number() == conn_number)
            conn = c;
    mut.unlock();
    return conn;
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

bool is_digit(string str)
{
    try
    {
        stoi(str);
    }
    catch (exception &e)
    {
        return false;
    }
    return true;
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

// Can only be called when lock is held
bool has_active_conns(vector<shared_ptr<Connection>> conns)
{
    for (shared_ptr<Connection> conn : conns)
        if (conn->is_alive())
            return true;

    return false;
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

void log_header(string header, int conn_number)
{
    string log = "\t[" + to_string(conn_number) + "]\t" + header;

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
