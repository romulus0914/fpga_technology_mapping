#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <stack>
#include <set>
#include <list>

#include <cstdlib>

using namespace std;

int K;
int num_nodes, num_pi, num_po;
// primary inputs & outputs
set<int> pi, po;
// circuits (adjacent list)
list<int> *circuit, *circuit_inv;

// trees cut from original graph
vector<vector<int>*> trees_inv;
// trees' nodes (in topological sort order)
vector<queue<int>> trees_sort_order;

typedef struct look_up_table {
    bool inuse;
    vector<int> fanins;
    int fanout;
    int num_fanins;
} LUT;

// LUTs
vector<LUT**> trees_luts;

void ReadAggFile(string agg_file)
{
    ifstream file;
    file.open(agg_file);

    string str;
    file >> str >> num_nodes >> num_pi >> num_po;

    int node;
    for (int i = 0; i < num_pi; i++) {
        file >> node;
        pi.insert(node);
    }
    for (int i = 0; i < num_po; i++) {
        file >> node;
        po.insert(node);
    }

    circuit = new list<int>[num_nodes + 1];
    circuit_inv = new list<int>[num_nodes + 1];
    int fanin1, fanin2, fanout;
    while (file >> fanout >> fanin1 >> fanin2) {
        circuit[fanin1].push_back(fanout);
        circuit[fanin2].push_back(fanout);

        circuit_inv[fanout].push_back(fanin1);
        circuit_inv[fanout].push_back(fanin2);
    }

    file.close();
}

void DFS(int v, stack<int> &sort_order, bool *visited)
{
    visited[v] = true;
    for (int i : circuit[v])
        if (!visited[i])
            DFS(i, sort_order, visited);

    sort_order.push(v);
}

void DFSInverse(int v, queue<int> &sort_order, bool *visited, vector<int> *tree_inv)
{
    visited[v] = true;
    for (int i : circuit_inv[v]) {
        tree_inv[v].emplace_back(i);
        if (!visited[i])
            DFSInverse(i, sort_order, visited, tree_inv);
    }

    sort_order.push(v);
}

void TopologicalSort(stack<int> &sort_order)
{
    bool visited[num_nodes + 1];
    for (int i = 0; i <= num_nodes; i++)
        visited[i] = false;

    for (int i = 1; i <= num_nodes; i++)
        if (!visited[i])
            DFS(i, sort_order, visited);
}

void CutGraphToTrees(stack<int> &sort_order)
{
    while (!sort_order.empty()) {
        int node = sort_order.top();
        sort_order.pop();

        // determine whether primary input or fanout < 2
        if (pi.find(node) != pi.end() || (po.find(node) == po.end() && circuit[node].size() < 2))
            continue;

        vector<int> *tree_inv = new vector<int>[num_nodes + 1];
        queue<int> tree_sort_order;
        bool visited[num_nodes + 1];
        for (int i = 0; i <= num_nodes; i++)
            visited[i] = false;

        DFSInverse(node, tree_sort_order, visited, tree_inv);

        trees_inv.emplace_back(tree_inv);
        trees_sort_order.emplace_back(tree_sort_order);

        // erase fanout edges
        circuit_inv[node] = list<int>();
    }
}

void GreedyMapping()
{
    int num_trees = trees_inv.size();
    for (int i = 0; i < num_trees; i++) {
        LUT **luts = new LUT*[num_nodes + 1];
        for (int i = 0; i <= num_nodes; i++)
            luts[i] = NULL;

        queue<int> &sort_order = trees_sort_order[i];
        vector<int> *tree_inv = trees_inv[i];

        while (!sort_order.empty()) {
            int node = sort_order.front();
            sort_order.pop();

            // pass PI
            if (tree_inv[node].empty()) {
                LUT *dummy_lut = new LUT();
                dummy_lut->inuse = true;
                dummy_lut->fanins.emplace_back(node);
                dummy_lut->fanout = node;
                dummy_lut->num_fanins = 1;
                luts[node] = dummy_lut;
                continue;
            }

            // determinde fanins are PI or LUT
            int num_inputs[2];
            int fanin1 = tree_inv[node][0];
            int fanin2 = tree_inv[node][1];
            LUT *fanin1_lut = luts[fanin1];
            LUT *fanin2_lut = luts[fanin2];
            if (tree_inv[fanin1].empty())
                num_inputs[0] = 1;
            else
                num_inputs[0] = fanin1_lut->num_fanins;
            if (tree_inv[fanin2].empty())
                num_inputs[1] = 1;
            else
                num_inputs[1] = fanin2_lut->num_fanins;

            // greedy packing
            if (num_inputs[0] + num_inputs[1] <= K) {
                LUT *new_lut = new LUT();
                new_lut->inuse = true;
                new_lut->fanins = fanin1_lut->fanins;
                new_lut->fanins.insert(new_lut->fanins.end(), fanin2_lut->fanins.begin(), fanin2_lut->fanins.end());
                new_lut->fanout = node;
                new_lut->num_fanins = new_lut->fanins.size();

                luts[node] = new_lut;
                fanin1_lut->inuse = false;
                fanin2_lut->inuse = false;
            }
            else {
                if (num_inputs[0] <= num_inputs[1]) {
                    if (num_inputs[0] + 1 <= K) {
                        LUT *new_lut = new LUT();
                        new_lut->inuse = true;
                        new_lut->fanins = fanin1_lut->fanins;
                        new_lut->fanins.emplace_back(fanin2);
                        new_lut->fanout = node;
                        new_lut->num_fanins = new_lut->fanins.size();

                        luts[node] = new_lut;
                        fanin1_lut->inuse = false;
                    }
                    else {
                        LUT *new_lut = new LUT();
                        new_lut->inuse = true;
                        new_lut->fanins.emplace_back(fanin1);
                        new_lut->fanins.emplace_back(fanin2);
                        new_lut->fanout = node;
                        new_lut->num_fanins = 2;

                        luts[node] = new_lut;
                    }
                }
                else {
                    if (num_inputs[1] + 1 <= K) {
                        LUT *new_lut = new LUT();
                        new_lut->inuse = true;
                        new_lut->fanins = fanin2_lut->fanins;
                        new_lut->fanins.emplace_back(fanin1);
                        new_lut->fanout = node;
                        new_lut->num_fanins = new_lut->fanins.size();

                        luts[node] = new_lut;
                        fanin2_lut->inuse = false;
                    }
                    else {
                        LUT *new_lut = new LUT();
                        new_lut->inuse = true;
                        new_lut->fanins.emplace_back(fanin1);
                        new_lut->fanins.emplace_back(fanin2);
                        new_lut->fanout = node;
                        new_lut->num_fanins = 2;

                        luts[node] = new_lut;
                    }
                }
            }
        }

        trees_luts.emplace_back(luts);
    }
}

void Output(string output_file)
{
    ofstream file;
    file.open(output_file);

    for (LUT **luts : trees_luts) {
        for (int i = 1; i <= num_nodes; i++) {
            if (luts[i] != NULL && luts[i]->inuse) {
                file << luts[i]->fanout;
                for (int j : luts[i]->fanins)
                    file << " " << j;
                file << '\n';
            }
        }
    }

    file.close();
}

int main(int argc, char *argv[])
{
    if (argc < 4) {
        cout << "[Usage]:\n";
        cout << "    ./mapping <path/to/aag_file> <LUT_size> <path/to/output_file>\n";
        exit(1);
    }

    string agg_file = argv[1];
    K = atoi(argv[2]);

    ReadAggFile(agg_file);

    stack<int> sort_order;
    TopologicalSort(sort_order);

    CutGraphToTrees(sort_order);

    GreedyMapping();

    string output_file = argv[3];
    Output(output_file);

    delete[] circuit;
    delete[] circuit_inv;
    for (vector<int> *temp : trees_inv)
        delete [] temp;
    for (LUT **luts : trees_luts) {
        for (int i = 0; i <= num_nodes; i++)
            if (luts[i] != NULL)
                delete luts[i];
        delete [] luts;
    }

    return 0;
}
