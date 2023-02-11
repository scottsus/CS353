#include <iostream>
#include <fstream>
#include <sstream>

// Data Structures
#include <vector>
#include <tuple>

using namespace std;

void usage();
tuple<vector<string>, vector<tuple<string, string, string>>> init_inifile(string);
void log(string);

string target_section_name = "";
string target_key_name = "";
string target_filename = "";

vector<string> section_names;
vector<tuple<string, string, string>> config;

int main(int argc, char *argv[])
{
    if (argc < 3)
        usage();

    string command = argv[1];
    if (command == "sections")
    {
        target_filename = argv[2];
    }
    else if (command == "keys")
    {
        target_section_name = argv[2];
        target_filename = argv[3];
    }
    else if (command == "value")
    {
        target_section_name = argv[2];
        target_key_name = argv[3];
        target_filename = argv[4];
    }

    tuple<vector<string>, vector<tuple<string, string, string>>> inifile = init_inifile(target_filename);
    section_names = get<0>(inifile);
    config = get<1>(inifile);

    log(command);
}

tuple<vector<string>, vector<tuple<string, string, string>>> init_inifile(string filename)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "Unable to open file: " << filename << endl;
        exit(-1);
    }

    vector<string> section_names{};
    vector<tuple<string, string, string>> config{};

    string line, curr_section = "";
    while (getline(file, line))
    {
        if (line.empty() || line[0] == ';')
            continue;

        if (line[0] == '[')
        {
            if (line[line.length() - 1] != ']')
                continue;

            string section_name = line.substr(1, line.length() - 2);
            curr_section = section_name;
            section_names.push_back(section_name);
            continue;
        }

        stringstream ss(line);
        string key, val = "";
        while (getline(ss, key, '='))
        {
            getline(ss, val);
            tuple<string, string, string> section_key_val = make_tuple(curr_section, key, val);
            config.push_back(section_key_val);
        }
    }

    return make_tuple(section_names, config);
}

void log(string command)
{
    if (command == "sections")
    {
        for (string section_name : section_names)
            cout << section_name << endl;
    }
    else if (command == "keys")
    {
        bool found_section = false;
        for (tuple<string, string, string> section_key_val : config)
        {
            string section = get<0>(section_key_val), key = get<1>(section_key_val);
            if (section == target_section_name)
            {
                cout << key << endl;
                found_section = true;
            }
        }
        if (!found_section)
            cerr << "Cannot find the [" << target_section_name << "] section in " << target_filename << endl;
    }
    else if (command == "value")
    {
        bool found_section = false, found_key = false;
        for (tuple<string, string, string> section_key_val : config)
        {
            string section = get<0>(section_key_val), key = get<1>(section_key_val), val = get<2>(section_key_val);
            if (section == target_section_name && key == target_key_name)
            {
                found_section = true;
                if (val == "")
                {
                    cerr << "There is no value for the 'host' key in the " << target_section_name << " section in " << target_filename << endl;
                    return;
                }
                cout << val << endl;
                found_key = true;
            }
        }
        if (!found_section)
        {
            cerr << "Cannot find the [" << target_section_name << "] section in " << target_filename << endl;
            return;
        }
        if (!found_key)
            cerr << "Cannot find the '" << target_key_name << "' key in the " << target_section_name << " section in " << target_filename << endl;
    }
}

void usage()
{
    cerr << "Incorrect arguments, please use the following convention: lab5a [COMMAND] INIFILE" << endl;
    exit(-1);
}