#include <iostream>
#include <fstream>
#include <vector>
#include <queue>

using namespace std;

void BFS(int, vector<vector<int>>);
vector<vector<int>> init_graph(string);
void log(vector<vector<int>>);

int num_nodes, num_edges;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cout << "Incorrect number of arguments!" << endl;
        return 1;
    }

    int start_node = stoi(argv[1]);
    vector<vector<int>> graph = init_graph(argv[2]);
    BFS(start_node, graph);

    return 0;
}

void BFS(int start_node, vector<vector<int>> graph)
{
    vector<bool> visited(graph.size(), false);
    queue<int> q{};

    q.push(start_node);
    visited[start_node] = true;
    cout << "level 0: " << start_node << endl;

    int level = 1;
    while (!q.empty())
    {
        int node = q.front();
        q.pop();
        vector<int> neighbors = graph.at(node);
        for (const int &neighbor : neighbors)
        {
            if (visited[neighbor])
                continue;
            cout << "level " << level << ": " << neighbor << " " << endl;
            q.push(neighbor);
            visited[neighbor] = true;
        }
        level++;
    }
}

vector<vector<int>> init_graph(string filename)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cout << "Unable to open file" << endl;
        return vector<vector<int>>{};
    }

    file >> num_nodes >> num_edges;

    vector<vector<int>> graph(num_nodes);
    for (int i = 0; i < num_edges; i++)
    {
        int src, dest;
        file >> src >> dest;
        graph[src].push_back(dest);
        graph[dest].push_back(src);
    }

    file.close();
    return graph;
}

void log(vector<vector<int>> graph)
{
    for (int i = 0; i < num_nodes; i++)
    {
        cout << "neighbors of node " << i << ": ";
        for (size_t j = 0; j < graph[i].size(); j++)
            cout << graph[i][j] << " ";
        cout << endl;
    }
}