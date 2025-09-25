#include <iostream>
#include <map>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>

using namespace std;

// ---------- Helpers for memo keys ----------

string stateToString(const vector<int>& state) {
    stringstream ss;
    for (size_t i = 0; i < state.size(); ++i) {
        if (i) ss << ",";
        ss << state[i];
    }
    return ss.str();
}

vector<int> stringToState(const string& s) {
    vector<int> state;
    stringstream ss(s);
    string item;
    while (getline(ss, item, ',')) {
        state.push_back(stoi(item));
    }
    return state;
}

string pairKey(const vector<int>& state, int player) {
    return stateToString(state) + ":" + to_string(player);
}

// ---------- Global memoization maps ----------

map<string, int> bruteForceMemo;
map<string, int> boothMemo;
map<string, string> normalizeMemo;

// ---------- Precompute unit circle points ----------

vector<pair<double, double>> precomputeUnitCircle(int n) {
    vector<pair<double, double>> circle;
    for (int i = 0; i < n; ++i) {
        double angle = 2 * M_PI * i / n;
        circle.emplace_back(cos(angle), sin(angle));
    }
    return circle;
}

// ---------- Utility functions ----------

// Get coordinates of the polygon vertices based on the spaces between counters
vector<pair<double, double>> getStateCounterCoordinates(const vector<int>& state, const vector<pair<double, double>>& circle) {
    vector<pair<double, double>> coordinates;
    int index = 0;
    for (int j : state) {
        coordinates.push_back(circle[index % circle.size()]);
        index += j;
    }
    return coordinates;
}

// Calculate area using the shoelace formula
double getStateArea(const vector<int>& state, const vector<pair<double,double>>& circle) {
    double area = 0.0;
    vector<pair<double, double>> coordinates = getStateCounterCoordinates(state, circle);
    int k = coordinates.size();
    int j = k - 1;
    for (int i = 0; i < k; ++i) {
        area += (coordinates[j].first + coordinates[i].first) * (coordinates[j].second - coordinates[i].second);
        j = i;
    }
    return abs(area / 2.0);
}

// Booth's algorithm to find the lexicographically minimal rotation
int booth(const vector<int>& state) {
    string key = stateToString(state);
    auto it = boothMemo.find(key);
    if (it != boothMemo.end()) return it->second;

    int n = state.size();
    vector<int> boothState(2*n);
    for (int i = 0; i < n; i++) boothState[i] = boothState[i+n] = state[i];

    int i = 0, j = 1, k = 0;
    while (i < n && j < n && k < n) {
        if (boothState[i+k] == boothState[j+k]) {
            k++;
            continue;
        }
        if (boothState[i+k] > boothState[j+k]) {
            i = i + k + 1;
            if (i <= j) i = j + 1;
        } else {
            j = j + k + 1;
            if (j <= i) j = i + 1;
        }
        k = 0;
    }
    boothMemo[key] = min(i, j);
    return min(i, j);
}

// Normalize state by finding its lexicographically smallest rotation or its reverse's smallest rotation
void normalizeState(vector<int>& state) {
    string key = stateToString(state);
    auto it = normalizeMemo.find(key);
    if (it != normalizeMemo.end()) {
        state = stringToState(it->second);
        return;
    }

    int n = state.size();

    // Smallest rotation of v
    int start1 = booth(state);
    vector<int> cand1(n);
    for (int i = 0; i < n; i++) cand1[i] = state[(start1 + i) % n];

    // Smallest rotation of reversed(v)
    vector<int> reverseState(state.rbegin(), state.rend());
    int start2 = booth(reverseState);
    vector<int> cand2(n);
    for (int i = 0; i < n; i++) cand2[i] = reverseState[(start2 + i) % n];

    // Pick lexicographically smaller
    if (cand2 < cand1) {
        state = cand2;
        normalizeMemo[key] = stateToString(cand2);
    }
    else {
        state = cand1;
        normalizeMemo[key] = stateToString(cand1);
    }
}

// Generate legal moves from current state
vector<vector<int>> legalMoves(int n, int k, const vector<int>& state, const vector<pair<double,double>>& circle) {
    double currentArea = getStateArea(state, circle);
    vector<vector<int>> moves;

    for (int i = 0; i < k; ++i) {
        // Forward redistribution
        for (int j = 1; j < state[i]; ++j) {
            vector<int> newState = state;
            newState[i] -= j;
            newState[(k + i - 1) % k] += j;

            normalizeState(newState);

            double newArea = getStateArea(newState, circle);
            if (newArea > currentArea + 1e-9) {
                bool exists = false;
                for (auto &move : moves) if (move == newState) { exists = true; break; }
                if (!exists) moves.push_back(newState);
            }
        }
        // Backward redistribution
        for (int j = 1; j < state[(k + i - 1) % k]; ++j) {
            vector<int> newState = state;
            newState[i] += j;
            newState[(i - 1 + k) % k] -= j;

            normalizeState(newState);

            double newArea = getStateArea(newState, circle);
            if (newArea > currentArea + 1e-9) {
                bool exists = false;
                for (auto &move : moves) if (move == newState) { exists = true; break; }
                if (!exists) moves.push_back(newState);
            }
        }
    }
    return moves;
}

int bruteForce(const vector<int>& currentPos, int n, int k, int player, const vector<pair<double,double>>& circle) {
    string key = pairKey(currentPos, player);
    auto it = bruteForceMemo.find(key);
    if (it != bruteForceMemo.end()) return it->second;
    
    vector<int> returnValues = {};

    vector<vector<int>> moves = legalMoves(n, k, currentPos, circle);
    if (moves.empty()) {
        int result = (player == 1 ? -1 : 1);
        bruteForceMemo[key] = result;
        return result;
    }

    for (const vector<int>& move : moves) {
        returnValues.push_back(bruteForce(move, n, k, -player, circle));
    }
    
    int best;
    if (player == 1) {
        best = *max_element(returnValues.begin(), returnValues.end());
    } else {
        best = *min_element(returnValues.begin(), returnValues.end());
    }
    bruteForceMemo[key] = best;
    return best;
}

int main(int argc, char** argv) {
   if (argc != 3) {
        cout << "Usage: " << argv[0] << " <number_of_points> <number_of_counters>\n";
        return 1;
    }

    int n = atoi(argv[1]);
    int k = atoi(argv[2]);
    if (k < 3 || n < k+2) {
        cout << "Invalid parameters. Ensure that k >= 3 and n >= k+2.\n";
        return 1;
    }

    const vector<pair<double, double>> circle = precomputeUnitCircle(n);

    // Add start state
    vector<int> startState(k - 1, 1);
    startState.push_back(n - k + 1);
    vector<pair<double, double>> startCoordinates = getStateCounterCoordinates(startState, circle);
    double startArea = getStateArea(startState, circle);

    cout << "Starting state: ";
    for (int val : startState) cout << val << " ";
    cout << endl;
    cout << "Starting coordinates: ";
    for (const auto& coord : startCoordinates) cout << "(" << coord.first << ", " << coord.second << ") ";
    cout << endl;
    cout << "Starting area: " << startArea << endl;

    int winner = bruteForce(startState, n, k, 1, circle);
    if (winner == 1) {
        cout << "Player 1 wins!\n";
    } else if (winner == -1) {
        cout << "Player 2 wins!\n";
    } else {
        cout << "No winner.\n";
    }

    return 0;
}
