#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <map>
#include <set>
#include <fstream>
#include <sstream>

using namespace std;

bool printGames = false;
bool printInfo = false;
bool printWinner = true;

// --- Graph structures ---
struct NodeInfo {
    string label;
    int player;
    bool terminal;
};
map<string, NodeInfo> nodes;
set<pair<string,string>> edges;

// Helper: stringify a position
string posToString(const vector<int>& pos) {
    ostringstream oss;
    for (int v : pos) oss << v << ",";
    return oss.str();
}

// Precompute unit circle coordinates for n points
vector<pair<double, double>> precomputeUnitCircle(int n) {
    vector<pair<double, double>> circle(n);
    for (int i = 0; i < n; ++i) {
        circle[i] = {cos(2 * M_PI * i / n), sin(2 * M_PI * i / n)};
    }
    return circle;
}

// Get coordinates of the polygon vertices based on the spaces between counters
vector<pair<double, double>> getCoordinates(const vector<int>& currentPos, const vector<pair<double, double>>& circle) {
    vector<pair<double, double>> coordinates;
    int index = 0;
    for (int j : currentPos) {
        coordinates.push_back(circle[index % circle.size()]);
        index += j;
    }
    return coordinates;
}

// Shoelace formula
double getArea(const vector<pair<double, double>>& coordinates) {
    double area = 0.0;
    int k = coordinates.size();
    int j = k - 1;
    for (int i = 0; i < k; ++i) {
        area += (coordinates[j].first + coordinates[i].first) * (coordinates[j].second - coordinates[i].second);
        j = i;
    }
    return abs(area / 2.0);
}

// Compare positions with rotations and reversals
bool comparePositions(const vector<int>& pos1, vector<int> pos2) {
    int k = pos1.size();
    for (int i = 0; i < k; ++i) {
        if (pos1 == pos2 || pos1 == vector<int>(pos2.rbegin(), pos2.rend()))
            return true;
        rotate(pos2.begin(), pos2.begin() + 1, pos2.end());
    }
    return false;
}

// Generate legal moves
vector<vector<int>> legalMoves(const vector<int>& currentPos, int n, const vector<pair<double,double>>& circle) {
    vector<vector<int>> moves;
    int k = currentPos.size();
    double currentArea = getArea(getCoordinates(currentPos, circle));

    for (int i = 0; i < k; ++i) {
        // Forward redistribution
        for (int j = 1; j < currentPos[i]; ++j) {
            vector<int> newPos = currentPos;
            newPos[i] -= j;
            newPos[(k + i - 1) % k] += j;

            if (comparePositions(newPos, currentPos)) continue;

            double newArea = getArea(getCoordinates(newPos, circle));
            if (newArea > currentArea) {
                bool exists = false;
                for (auto &move : moves) if (comparePositions(move, newPos)) { exists = true; break; }
                if (!exists) moves.push_back(newPos);
            }
        }
        // Backward redistribution
        for (int j = 1; j < currentPos[(k + i - 1) % k]; ++j) {
            vector<int> newPos = currentPos;
            newPos[i] += j;
            newPos[(i - 1 + k) % k] -= j;

            if (comparePositions(newPos, currentPos)) continue;

            double newArea = getArea(getCoordinates(newPos, circle));
            if (newArea > currentArea) {
                bool exists = false;
                for (auto &move : moves) if (comparePositions(move, newPos)) { exists = true; break; }
                if (!exists) moves.push_back(newPos);
            }
        }
    }
    return moves;
}

int bruteForce(const vector<int>& currentPos, int n, int player, vector<vector<int>> path) {
    // Add the current position to the path
    path.push_back(currentPos);

    string posKey = posToString(currentPos);
    if (!nodes.count(posKey)) {
        NodeInfo info;
        info.label = posKey;
        info.player = player;
        info.terminal = false;
        nodes[posKey] = info;
    }

    vector<int> returnValues = {};
    vector<vector<int>> moves = legalMoves(currentPos, n, precomputeUnitCircle(n));
    if (moves.empty()) {
        nodes[posKey].terminal = true;
        return (player == 1 ? 1 : -1);
    }

    for (const vector<int>& move : moves) {
        string nextKey = posToString(move);
        edges.insert({posKey, nextKey});
        returnValues.push_back(bruteForce(move, n, -player, path));
    }
    
    if(player == 1) {
        int maxReturn = *max_element(returnValues.begin(), returnValues.end());
        return maxReturn;
    } else {
        int minReturn = *min_element(returnValues.begin(), returnValues.end());
        return minReturn;
    }
}

void writeDotFile(const string& filename, const string& startKey) {
    ofstream ofs(filename);
    ofs << "digraph G {\n";
    ofs << "  rankdir=TB;\n";              // top-to-bottom layout
    ofs << "  ranksep=2;\n";             // vertical spacing
    ofs << "  nodesep=0.6;\n";             // horizontal spacing
    ofs << "  splines=true;\n";            // smoother edges
    ofs << "  overlap=false;\n";           // prevent overlapping
    ofs << "  graph [dpi=120];\n";         // higher resolution
    ofs << "\n";

    for (auto &kv : nodes) {
        string key = kv.first;
        NodeInfo info = kv.second;
        string color = "white";
        if (key == startKey) color = "green";
        else if (info.terminal) color = "red";
        ofs << "  \"" << key << "\" [label=\"" << key
            << " p=" << info.player
            << "\", style=filled, fillcolor=" << color << "];\n";
    }
    for (auto &e : edges) {
        ofs << "  \"" << e.first << "\" -> \"" << e.second << "\";\n";
    }
    ofs << "}\n";
    ofs.close();
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <number_of_points> <number_of_counters>\n";
        return 1;
    }

    int n = atoi(argv[1]);
    int k = atoi(argv[2]);

    // Starting positions
    vector<int> startPos(k - 1, 1);
    startPos.push_back(n - k + 1);
    string startKey = posToString(startPos);

    // Precompute circle coordinates
    vector<pair<double,double>> circle = precomputeUnitCircle(n);

    // Get coordinates and area
    vector<pair<double,double>> coordinates = getCoordinates(startPos, circle);
    double area = getArea(coordinates);

    vector<vector<int>> moves = legalMoves(startPos, n, circle);

    if (printInfo) {
        cout << "Starting positions: ";
        for (int val : startPos) cout << val << " ";
        cout << "\nArea of the polygon: " << area << "\n";
    }

    int winner = bruteForce(startPos, n, 1, {});

    // Write .dot file
    writeDotFile("graph.dot", startKey);

    if (printWinner) {
        if (winner == 1) {
            cout << "Player 1 wins!\n";
        } else if (winner == -1) {
            cout << "Player 2 wins!\n";
        } else {
            cout << "No winner.\n";
        }
    }

    return 0;
}

// Compile: g++ main.cpp -o main
// Run: ./main <points> <counters>
// Output: graph.dot (render with: dot -Tpng graph.dot -o graph.png)
