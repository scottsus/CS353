#include <iostream>
#include <fstream>
#include <vector>
#include <queue>

using namespace std;

// Helper class
class Edge
{
public:
    int from, to, dist;

    Edge(int from, int to, int dist)
    {
        this->from = from;
        this->to = to;
        this->dist = dist;
    }
};

// Infinity
const int INF = 1e9;

// Function Declarations
void dijkstra(int, vector<vector<Edge>>);
vector<vector<Edge>> init_graph(string);
void log(vector<vector<Edge>>);

// Global Variables
int num_nodes, num_edges;

// Main
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cout << "Incorrect number of arguments!" << endl;
        return 1;
    }

    int start_node = stoi(argv[1]);
    vector<vector<Edge>> graph = init_graph(argv[2]);
    // log(graph);
    dijkstra(start_node, graph);

    return 0;
}

// Dijkstra's Algorithm
void dijkstra(int start_node, vector<vector<Edge>> graph)
{
    vector<int> distances(num_nodes, INF);
    vector<bool> visited(num_nodes, false);
    struct EdgeComparator
    {
        bool operator()(const Edge &e1, const Edge &e2)
        {
            return e1.dist > e2.dist;
        }
    };
    priority_queue<Edge, vector<Edge>, EdgeComparator> pq{};

    distances[start_node] = 0;
    pq.push(Edge(start_node, start_node, 0));

    int itr = 0;
    while (!pq.empty())
    {
        Edge edge = pq.top();
        int from = edge.from, pred = edge.to, dist = edge.dist;
        pq.pop();

        // if this node has been relaxed before
        // there's no way it can be further relaxed
        if (visited[from])
            continue;
        visited[from] = true;

        cout << "itr " << itr++ << ": add node " << from << ", distance " << dist << ", predecessor " << pred << endl;

        vector<Edge> neighbors = graph.at(from);
        for (const Edge &edge : neighbors)
        {
            int to = edge.to, dist = edge.dist;
            if (distances[from] + dist < distances[to])
            {
                distances[to] = distances[from] + dist;
                pq.push(Edge(to, from, distances[to]));
                // in this case, the Edge.to will store the info
                // about the predecessor instead of the destination node
            }
        }
    }
}

// Initializing the graph
vector<vector<Edge>> init_graph(string filename)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cout << "Unable to open file" << endl;
        return vector<vector<Edge>>{};
    }

    file >> num_nodes >> num_edges;

    vector<vector<Edge>> graph(num_nodes);
    for (int i = 0; i < num_edges; i++)
    {
        int from, to, dist;
        file >> from >> to >> dist;
        graph[from].push_back(Edge(from, to, dist));
        graph[to].push_back(Edge(to, from, dist));
    }

    file.close();
    return graph;
}

// Helper function to log the console out the graph
void log(vector<vector<Edge>> graph)
{
    for (int i = 0; i < num_nodes; i++)
    {
        cout << "neighbors of Edge " << i << ": ";
        vector<Edge> neighbors = graph[i];
        for (size_t j = 0; j < neighbors.size(); j++)
        {
            Edge neighbor = neighbors.at(j);
            cout << neighbor.to << " ";
        }
        cout << endl;
    }
}
