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

vector<int> BFS(int, vector<vector<Edge>>);
vector<int> dijkstra(int, vector<vector<Edge>>);
vector<vector<Edge>> init_graph(string);
vector<vector<Edge>> init_type1_graph(vector<vector<char>>);
vector<vector<Edge>> init_type2_graph(vector<vector<char>>);
int check_type(vector<vector<char>>);
bool check_row(vector<char>);
int normalize(int, int);
void log(vector<vector<char>>);
void log(vector<vector<Edge>>);
void log(vector<int>);

// High Level Overview
// 1. Parse the maze
// 2. Check if it's Type I or Type II
// 3. Convert into graph
// 4. Run BFS or Dijkstra
// 5. Output

int rows, cols;
int start_row, start_col;
int char_rows, char_cols;
int type;

int main(int argc, char *argv[])
{
    if (argc != 2 && argc != 4)
    {
        cout << "usage: pa1 [r c] filename" << endl;
        return 1;
    }
    else if (argc == 2)
    {
        start_row = 0, start_col = 0;
    }
    else
    {
        start_row = stoi(argv[1]), start_col = stoi(argv[2]);
    }

    int start_node = start_row * rows + start_col;
    vector<vector<Edge>> graph = init_graph(argv[argc - 1]);
    cout << "Typee: " << type << endl;
    // log(graph);

    vector<int> distances;
    if (type == 1)
        distances = BFS(start_node, graph);
    else // type == 2
        distances = dijkstra(start_node, graph);

    log(distances);
    // Convert back to char_matrix
    // Log char_matrix

    return 0;
}

// Breadth First Search
// Modifies the original graph
vector<int> BFS(int start_node, vector<vector<Edge>> graph)
{
    int num_nodes = rows * cols;
    vector<int> distances(num_nodes);
    vector<bool> visited(graph.size(), false);
    queue<int> q{};

    q.push(start_node);
    visited[start_node] = true;

    int level = 1;
    while (!q.empty())
    {
        int size = q.size();
        for (int i = 0; i < size; i++)
        {
            int node = q.front();
            q.pop();
            vector<Edge> &neighbors = graph.at(node);
            for (Edge edge : neighbors)
            {
                int neighbor = edge.to;
                if (visited[neighbor])
                    continue;
                distances[neighbor] = level;
                q.push(neighbor);
                visited[neighbor] = true;
            }
        }
        level++;
    }
    cout << endl;
    return distances;
}

// Dijkstra's Algorithm
// Modifies the original graph
vector<int> dijkstra(int start_node, vector<vector<Edge>> graph)
{
    const int INF = 1e9;
    int num_nodes = rows * cols;

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
    return distances;
}

vector<vector<Edge>> init_graph(string filename)
{
    // Parse the file to see if it's alright
    ifstream file(filename);
    if (!file.is_open())
    {
        cout << "Unable to open file" << endl;
        return vector<vector<Edge>>{};
    }

    vector<vector<char>> char_graph{};

    string line;
    while (getline(file, line))
    {
        vector<char> char_line{};
        for (uint i = 0; i < line.length(); i++)
            char_line.push_back(line[i]);
        char_graph.push_back(char_line);
    }

    char_rows = char_graph.size(), char_cols = char_graph.at(0).size();
    rows = (char_graph.size() - 1) / 2, cols = (char_graph.at(0).size() - 1) / 2;

    vector<vector<Edge>> graph;
    type = check_type(char_graph);
    if (type == 0)
        return vector<vector<Edge>>{};
    else if (type == 1)
        graph = init_type1_graph(char_graph);
    else
        graph = init_type1_graph(char_graph);

    return graph;
}

vector<vector<Edge>> init_type1_graph(vector<vector<char>> input_graph)
{
    cout << "Initializing..." << endl;
    vector<vector<Edge>> output_graph(rows * cols);
    for (uint i = 1; i < input_graph.size() - 1; i += 2)
    {
        vector<char> row = input_graph.at(i);
        for (uint j = 1; j < row.size() - 1; j += 2)
        {
            int curr_node = normalize(i * char_cols + j, i);
            cout << "curr: " << curr_node << endl;

            int top_node = normalize((i - 2) * char_cols + j, i - 2);
            if (i - 1 > 0)
            {
                char path = input_graph.at(i - 1).at(j);
                if (isdigit(path))
                    output_graph.at(curr_node).push_back(Edge(curr_node, top_node, atoi(&path)));
                else if (path == ' ')
                    output_graph.at(curr_node).push_back(Edge(curr_node, top_node, 0));
            }

            int btm_node = normalize((i + 2) * char_cols + j, i + 2);
            if ((int)i + 1 < char_rows - 1)
            {
                char path = input_graph.at(i + 1).at(j);
                if (isdigit(path))
                    output_graph.at(curr_node).push_back(Edge(curr_node, btm_node, atoi(&path)));
                else if (path == ' ')
                    output_graph.at(curr_node).push_back(Edge(curr_node, btm_node, 0));
            }

            int left_node = normalize(i * char_cols + (j - 2), i);
            if (j - 1 > 0)
            {
                char path = input_graph.at(i).at(j - 1);
                if (isdigit(path))
                    output_graph.at(curr_node).push_back(Edge(curr_node, left_node, atoi(&path)));
                else if (path == ' ')
                    output_graph.at(curr_node).push_back(Edge(curr_node, left_node, 0));
            }

            int right_node = normalize(i * char_cols + (j + 2), i);
            if ((int)j + 1 < char_cols - 1)
            {
                char path = input_graph.at(i).at(j + 1);
                if (isdigit(path))
                    output_graph.at(curr_node).push_back(Edge(curr_node, right_node, atoi(&path)));
                else if (input_graph.at(i).at(j + 1) == ' ')
                    output_graph.at(curr_node).push_back(Edge(curr_node, right_node, 0));
            }
        }
    }
    return output_graph;
}

// Convert input matrix indices into 0-based indices
int normalize(int skewed_index, int row)
{
    return (skewed_index - ((char_cols + 1) * ((row + 1) / 2))) / 2;
}

// Checking if input is valid and
// whether it is Type I or Type II
int check_type(vector<vector<char>> graph)
{
    cout << "Checking the input matrix..." << endl;
    int type = 1;

    // Check top and bottom rows
    if (!check_row(graph.at(0)) || !check_row(graph.at(graph.size() - 1)))
        return 0;

    int level = 0;
    for (int i = 0; i < char_rows; i++)
    {
        // Check for different number of columns
        vector<char> row = graph.at(i);
        if ((int)row.size() != char_cols)
            return 0;

        for (int j = 0; j < char_cols; j++)
        {
            // Check Type I or Type II
            char space = graph.at(i).at(j);
            if (isdigit(space))
                type = 2;
            cout << space << " ";

            // Check + | + | + pattern on boundary cols
            if (level % 2 == 0)
            {
                if (row.at(0) != '+' || row.at(row.size() - 1) != '+')
                    return 0;
            }
            else
            {
                if (row.at(0) != '|' || row.at(row.size() - 1) != '|')
                    return 0;
            }
        }
        level++;
        cout << endl;
    }
    return type;
}

// Check +-+-+ pattern on boundary rows
bool check_row(vector<char> row)
{
    if (row.at(row.size() - 1) != '+')
        return false;
    for (uint i = 0; i < row.size() - 2; i += 2)
    {
        if (row.at(i) != '+' || row.at(i + 1) != '-')
            return false;
    }
    return true;
}

// Console out the char graph for debugging
void log(vector<vector<char>> graph)
{
    cout << "Rows: " << char_rows << ", Cols: " << char_cols << endl;
    for (int i = 0; i < char_rows; i++)
    {
        for (int j = 0; j < char_cols; j++)
        {
            cout << graph.at(i).at(j) << " ";
        }
        cout << endl;
    }
    cout << endl;
}

// Console out Adjacency List for debgugging
void log(vector<vector<Edge>> graph)
{
    cout << "Graph Relationship:" << endl;
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            int curr_node = i * cols + j;
            cout << "neighbors of node " << curr_node << ": ";

            vector<Edge> neighbors = graph.at(curr_node);
            for (const Edge &neighbor : neighbors)
            {
                cout << "[" << neighbor.to << "," << neighbor.dist << "] ";
            }
            cout << endl;
        }
    }
    cout << endl;
}

void log(vector<int> distances)
{
    cout << "Distances:" << endl;
    for (uint i = 0; i < distances.size(); i++)
    {
        cout << "[" << i << "," << distances[i] << "]" << endl;
    }
}