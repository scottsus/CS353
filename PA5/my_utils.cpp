/*
 *  Utility functions
 */

#include <iostream>
#include <algorithm>

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

    string config_file = argv[1];
    map<string, map<string, string>> config = get_config(config_file);

    if (config["server"]["mode"] == "http")
        return make_tuple(1, vector<string>{config_file});

    return make_tuple(0, vector<string>{config_file});
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

            if (section_name == "*")
                config["server"]["mode"] = "http";
            if (section_name == "topology")
                config["server"]["mode"] = "p2p";
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
    string section = "*";
    if (config.count(file_type))
        section = file_type;

    try
    {
        int P = stoi(config[section]["P"]);
        int MAXR = stoi(config[section]["MAXR"]);
        int DIAL = stoi(config[section]["DIAL"]);

        return vector<int>{P, MAXR, DIAL};
    }
    catch (exception &e)
    {
        return vector<int>{2, 5, 100};
    }
}

void BFS(shared_ptr<Node> start_node, map<string, map<string, shared_ptr<Node>>> adj_list, map<string, shared_ptr<Node>> *nodes)
{
    map<string, bool> visited{};
    queue<shared_ptr<Node>> q{};

    int level = 1;
    visited[start_node->get_nodeid()] = true;
    q.push(start_node);
    while (!q.empty())
    {
        shared_ptr<Node> node = q.front();
        q.pop();

        map<string, shared_ptr<Node>> neighbors = adj_list[node->get_nodeid()];
        for (map<string, shared_ptr<Node>>::iterator itr = neighbors.begin(); itr != neighbors.end(); itr++)
        {
            shared_ptr<Node> child_node = itr->second;
            if (visited[child_node->get_nodeid()])
                continue;
            visited[child_node->get_nodeid()] = true;

            child_node->set_level(level);
            child_node->set_pred(node);
            (*nodes)[child_node->get_nodeid()] = child_node;
            q.push(child_node);
        }
        level++;
    }
}

map<string, shared_ptr<Node>> get_forwarding_table(string nodeid, vector<shared_ptr<Connection>> *conns)
{
    // 0. Make sure graph is updated with real-time connections status
    graph[nodeid] = get_neighbors(conns);

    // 1. Build the adjacency list and populate with empty pointers
    map<string, map<string, shared_ptr<Node>>> adj_list{};
    adj_list[nodeid] = map<string, shared_ptr<Node>>{};
    for (map<string, string>::iterator itr = graph.begin(); itr != graph.end(); itr++)
    {
        string netgraph_key = itr->first;
        string netgraph_val = itr->second;

        adj_list[netgraph_key] = map<string, shared_ptr<Node>>{};

        stringstream ss(netgraph_val);
        string neighbor_nodeid;
        while (getline(ss, neighbor_nodeid, ','))
        {
            if (neighbor_nodeid == "")
                continue;

            Node node(neighbor_nodeid, 0, NULL);
            adj_list[netgraph_key][neighbor_nodeid] = make_shared<Node>(node);
        }

        if (is_neighbor(netgraph_key, conns))
        {
            Node direct_neighbor(netgraph_key, 0, NULL);
            adj_list[nodeid][netgraph_key] = make_shared<Node>(direct_neighbor);
        }
    }

    // 2. Run BFS to populate adjacency list and set `pred` pointers
    map<string, shared_ptr<Node>> nodes{};
    shared_ptr<Node> start_node = make_shared<Node>(Node(nodeid, 0, NULL));
    BFS(start_node, adj_list, &nodes);

    map<string, shared_ptr<Node>> forwarding_table{};
    for (map<string, shared_ptr<Node>>::iterator itr = nodes.begin(); itr != nodes.end(); itr++)
    {
        shared_ptr<Node> node = itr->second;
        if (node->get_nodeid() == nodeid)
            continue;

        if (node->get_pred() == NULL)
            continue;

        string key_nodeid = node->get_nodeid();
        while (node->get_pred() != NULL && node->get_pred()->get_nodeid() != nodeid)
            node = node->get_pred();

        forwarding_table[key_nodeid] = node;
    }

    return forwarding_table;
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

shared_ptr<Connection> find_conn(string target_nodeid, vector<shared_ptr<Connection>> *conns)
{
    shared_ptr<Connection> conn = NULL;
    mut.lock();
    for (shared_ptr<Connection> c : *conns)
        if (c->get_neighbor_nodeid() == target_nodeid)
            conn = c;
    mut.unlock();
    return conn;
}

bool neighbor_is_up(vector<shared_ptr<Connection>> *conns, string neighbor_nodeid)
{
    for (shared_ptr<Connection> conn : *conns)
        if (conn->get_neighbor_nodeid() == neighbor_nodeid)
            return true;
    return false;
}

void remove_from_vector(string target, vector<string> &vec)
{
    vec.erase(remove(vec.begin(), vec.end(), target), vec.end());
}

void update_graph(string removed_nodeid, vector<shared_ptr<Connection>> *conns)
{
    mut.lock();
    graph.erase(removed_nodeid);
    for (map<string, string>::iterator itr = graph.begin(); itr != graph.end();)
    {
        string neighbor_key = itr->first;
        string neighbors_str = itr->second;
        stringstream ss(neighbors_str);

        string new_neighbors_str = "", neighbor;
        while (getline(ss, neighbor, ','))
        {
            if (neighbor == removed_nodeid)
                break;

            new_neighbors_str += neighbor + ",";
        }

        if (new_neighbors_str.length() > 0)
        {
            new_neighbors_str.pop_back();
            graph[neighbor_key] = new_neighbors_str;
            itr++;
        }
        else
            itr = graph.erase(itr);
    }
    mut.unlock();
}

void get_message_id_and_start_time(string nodeid, const char *obj_category, string &hexstring_of_unique_obj_id, string &origin_start_time)
{
    static unsigned long seq_no = 1L;
    static int first_time = 1;
    static struct timeval node_start_time;
    static char origin_start_time_buf[18];
    char hexstringbuf_of_unique_obj_id[80];

    if (first_time)
    {
        gettimeofday(&node_start_time, NULL);
        snprintf(origin_start_time_buf, sizeof(origin_start_time_buf),
                 "%010d.%06d", (int)(node_start_time.tv_sec), (int)(node_start_time.tv_usec));
        first_time = 0;
    }
    seq_no++;
    struct timeval now;
    gettimeofday(&now, NULL);
    snprintf(hexstringbuf_of_unique_obj_id, sizeof(hexstringbuf_of_unique_obj_id),
             "%s_%1ld_%010d.%06d", nodeid.c_str(), (long)seq_no, (int)(now.tv_sec), (int)(now.tv_usec));
    hexstring_of_unique_obj_id = hexstringbuf_of_unique_obj_id;
    origin_start_time = origin_start_time_buf;
}

bool is_neighbor(string neighbor_nodeid, vector<shared_ptr<Connection>> *conns)
{
    for (shared_ptr<Connection> conn : *conns)
        if (conn->get_neighbor_nodeid() == neighbor_nodeid)
            return true;
    return false;
}

string get_neighbors(vector<shared_ptr<Connection>> *conns)
{
    string neighbors = "";
    for (shared_ptr<Connection> conn : *conns)
        neighbors += conn->get_neighbor_nodeid() + ",";
    if (!neighbors.empty())
        neighbors.pop_back();

    return neighbors;
}

shared_ptr<HelloMessage> construct_hello(int peer_socketfd, shared_ptr<Connection> conn)
{
    string err = "";

    try
    {
        string ttl_line;
        int bytes_received = read_a_line(peer_socketfd, ttl_line);
        if (bytes_received <= 2)
            err = "TTL";
        // int ttl = stoi(extract_header_value(ttl_line));

        string flood_line;
        bytes_received = read_a_line(peer_socketfd, flood_line);
        if (bytes_received <= 2)
            err = "FLOOD";
        // int flood_reason = stoi(extract_header_value(flood_line));

        string from_line;
        bytes_received = read_a_line(peer_socketfd, from_line);
        if (bytes_received <= 2)
            err = "FROM";
        string from_nodeid = extract_header_value(from_line);

        string content_len_line;
        bytes_received = read_a_line(peer_socketfd, content_len_line);
        if (bytes_received <= 2)
            err = "CONTENT_LEN";
        // int content_len = stoi(extract_header_value(content_len_line));

        string newline;
        bytes_received = read_a_line(peer_socketfd, newline);

        shared_ptr<HelloMessage> hello = make_shared<HelloMessage>(HelloMessage(from_nodeid));
        log_hello("r", hello);

        return hello;
    }
    catch (exception &e)
    {
        cout << "[HELLO]: " << err << endl;
        return make_shared<HelloMessage>();
    }
}

shared_ptr<LSUPDATEMessage> construct_lsupdate(int peer_socketfd, shared_ptr<Connection> conn)
{
    string err = "";

    try
    {
        string ttl_line;
        int bytes_received = read_a_line(peer_socketfd, ttl_line);
        if (bytes_received <= 0)
            err = "[LSUPDATE]: TTL";
        int ttl = stoi(extract_header_value(ttl_line));

        string flood_str;
        bytes_received = read_a_line(peer_socketfd, flood_str);
        if (bytes_received <= 0)
            err = "[LSUPDATE]: FLOOD";
        int flood = stoi(get<1>(str_split(flood_str, '=')));

        string messageid_line;
        bytes_received = read_a_line(peer_socketfd, messageid_line);
        if (bytes_received <= 0)
            err = "[LSUPDATE]: MESSAGEID";
        string messageid = extract_header_value(messageid_line);

        string origin_nodeid_line;
        bytes_received = read_a_line(peer_socketfd, origin_nodeid_line);
        if (bytes_received <= 0)
            err = "[LSUPDATE]: ORIGIN_NODEID";
        string origin_nodeid = extract_header_value(origin_nodeid_line);

        string origin_start_time_line;
        bytes_received = read_a_line(peer_socketfd, origin_start_time_line);
        if (bytes_received <= 0)
            err = "[LSUPDATE]: ORIGIN_START_TIME";
        string origin_start_time = extract_header_value(origin_start_time_line);

        string content_len_line;
        bytes_received = read_a_line(peer_socketfd, content_len_line);
        if (bytes_received <= 0)
            err = "[LSUPDATE]: CONTENT_LEN";
        int content_len = stoi(extract_header_value(content_len_line));

        string newline;
        bytes_received = read_a_line(peer_socketfd, newline);

        char neighbors[content_len + 1];
        bytes_received = read(peer_socketfd, neighbors, content_len);
        if (bytes_received <= 0)
            err = "[LSUPDATE]: NEIGHBORS";
        neighbors[content_len] = '\0';

        shared_ptr<LSUPDATEMessage> lsupdate = make_shared<LSUPDATEMessage>(LSUPDATEMessage(ttl, flood, messageid, origin_nodeid, origin_start_time, neighbors));
        log_LSUPDATE("r", lsupdate);

        return lsupdate;
    }
    catch (exception &e)
    {
        cout << "[LSUPDATE]: " << err << endl;
        return make_shared<LSUPDATEMessage>();
    }
}

shared_ptr<UCASTAPPMessage> construct_ucastapp(int peer_socketfd, shared_ptr<Connection> conn)
{
    string err = "";

    try
    {
        string ttl_line;
        int bytes_received = read_a_line(peer_socketfd, ttl_line);
        if (bytes_received <= 0)
            err = "[UCASTAPP]: TTL";
        int ttl = stoi(extract_header_value(ttl_line));

        string flood_str;
        bytes_received = read_a_line(peer_socketfd, flood_str);
        if (bytes_received <= 0)
            err = "[UCASTAPP]: FLOOD";
        // int flood = stoi(extract_header_value(flood_str));

        string message_id_line;
        bytes_received = read_a_line(peer_socketfd, message_id_line);
        if (bytes_received <= 0)
            err = "[UCASTAPP]: MESSAGE_ID";
        string message_id = extract_header_value(message_id_line);

        string from_line;
        bytes_received = read_a_line(peer_socketfd, from_line);
        if (bytes_received <= 0)
            err = "[UCASTAPP]: FROM";
        string src_nodeid = extract_header_value(from_line);

        string to_line;
        bytes_received = read_a_line(peer_socketfd, to_line);
        if (bytes_received <= 0)
            err = "[UCASTAPP]: TO";
        string dest_nodeid = extract_header_value(to_line);

        string next_layer_line;
        bytes_received = read_a_line(peer_socketfd, next_layer_line);
        if (bytes_received <= 0)
            err = "[UCASTAPP]: NEXT_LAYER";
        int next_layer = stoi(extract_header_value(next_layer_line));

        string content_len_line;
        bytes_received = read_a_line(peer_socketfd, content_len_line);
        if (bytes_received <= 0)
            err = "[UCASTAPP]: CONTENT_LEN";
        int content_len = stoi(extract_header_value(content_len_line));

        string newline;
        bytes_received = read_a_line(peer_socketfd, newline);

        char message_body[content_len + 1];
        bytes_received = read(peer_socketfd, message_body, content_len);
        if (bytes_received <= 0)
            err = "[UCASTAPP]: MESSAGE_BODY";
        message_body[content_len] = '\0';

        shared_ptr<UCASTAPPMessage> ucastapp = make_shared<UCASTAPPMessage>(UCASTAPPMessage(ttl, message_id, src_nodeid, dest_nodeid, next_layer, message_body));
        log_UCASTAPP("r", ucastapp);

        return ucastapp;
    }
    catch (exception &e)
    {
        cout << "[UCASTAPP]: " << err << endl;
        return make_shared<UCASTAPPMessage>();
    }
}

shared_ptr<P2PMessage> await_message(int peer_socketfd, shared_ptr<Connection> conn)
{
    string protocol;
    int bytes_received = read_a_line(peer_socketfd, protocol);
    if (bytes_received == -1)
        return NULL;
    if (bytes_received <= 2)
        return make_shared<P2PMessage>(P2PMessage());

    stringstream ss(protocol);
    string version, message_type;
    ss >> version >> message_type;

    if (message_type == "SAYHELLO")
        return construct_hello(peer_socketfd, conn);

    else if (message_type == "LSUPDATE")
        return construct_lsupdate(peer_socketfd, conn);

    else if (message_type == "UCASTAPP")
        return construct_ucastapp(peer_socketfd, conn);

    return NULL;
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

pair<string, string> str_split(string str, char delimeter)
{
    stringstream ss(str);
    string key, val;
    getline(ss, key, delimeter);
    getline(ss, val);

    if (val[0] == ' ')
        val = val.substr(1);
    if (val[val.length() - 1] == '\r')
        val = val.substr(0, val.length() - 1);

    return make_pair(key, val);
}

string extract_header_value(string header)
{
    pair<string, string> key_val = str_split(header, ':');
    return get<1>(key_val);
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

void log_hello(string type, shared_ptr<HelloMessage> hello)
{
    string log = "";
    log += "[" + get_timestamp_now() + "] ";
    log += type + " ";
    log += "SAYHELLO ";
    log += hello->get_origin_nodeid() + " ";
    log += "1 - ";
    log += to_string(hello->get_content_len()) + "\n";

    if (SAYHELLO == 1 && logfile.is_open())
    {
        logfile << log;
        logfile.flush();
    }
    else
        cout << log;
}

void log_LSUPDATE(string type, shared_ptr<LSUPDATEMessage> lsupdate)
{
    bool logging_to_logfile = (LSUPDATE == 1 && logfile.is_open());

    string log = "";
    if (!logging_to_logfile && type == "r")
        log = "\n";

    log += "[" + get_timestamp_now() + "] ";
    log += type + " ";
    log += "LSUPDATE ";
    log += lsupdate->get_sender_nodeid() + " ";
    log += to_string(lsupdate->get_ttl()) + " ";
    log += to_string(1) + " ";
    log += to_string(lsupdate->get_content_len()) + " ";
    log += "{" + lsupdate->get_messageid().substr(0, 8) + "} ";
    log += lsupdate->get_origin_starttime() + " ";
    log += lsupdate->get_origin_nodeid() + " ";
    log += "(" + lsupdate->get_message_body() + ")\n";

    if (logging_to_logfile)
    {
        logfile << log;
        logfile.flush();
    }
    else
        cout << log;
}

void log_UCASTAPP(string type, shared_ptr<UCASTAPPMessage> ucastapp)
{
    bool logging_to_logfile = (UCASTAPP == 1 && logfile.is_open());

    string log = "";
    log += "[" + get_timestamp_now() + "] ";
    log += type + " ";
    log += "UCASTAPP ";
    log += ucastapp->get_sender_nodeid() + " ";
    log += to_string(ucastapp->get_ttl());
    log += " - ";
    log += to_string(ucastapp->get_content_len()) + " ";
    log += "{" + ucastapp->get_messageid().substr(0, 8) + "} ";
    log += ucastapp->get_origin_nodeid() + " ";
    log += ucastapp->get_target_nodeid() + " ";
    log += to_string(ucastapp->get_next_layer()) + " ";

    shared_ptr<RDTMessage> rdt = ucastapp->get_rdt_message();

    if (rdt->get_type() == "RDTDATA")
    {
        log += to_string(rdt->get_seq_num()) + " ";

        int app_num = rdt->get_app_num();
        if (app_num == 0)
            log += "- ";
        else
            log += to_string(app_num) + " ";

        log += to_string(rdt->get_rdt_content_len()) + " ";

        int rdt_content_len = rdt->get_rdt_content_len();
        if (rdt_content_len == 1 && rdt->get_rdt_message_body()[rdt_content_len - 1] == '\n')
            log += "\\n\n";
        else
            log += rdt->get_rdt_message_body() + "\n";
    }
    else if (rdt->get_type() == "RDTACK")
        log += ucastapp->get_message_body() + "\n";

    if (logging_to_logfile)
    {
        logfile << log;
        logfile.flush();
    }
    else
        cout << log << endl;
}

void usage()
{
    cerr << "Please use the following command: ./pa2 [-c] HOST PORT URI_1 FILE_1 [... URI_N FILE_N]" << endl;
    exit(-1);
}
