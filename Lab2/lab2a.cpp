#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cout << "Not enough command line arguments!" << endl;
        return 1;
    }

    ifstream file(argv[1]);
    if (!file.is_open())
    {
        cout << "Unable to open file" << endl;
        return 1;
    }

    int num_nodes, num_edges;
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

    for (int i = 0; i < num_nodes; i++)
    {
        cout << "neighbors of node " << i << ": ";
        for (size_t j = 0; j < graph[i].size(); j++)
            cout << graph[i][j] << " ";
        cout << endl;
    }

    return 0;
}