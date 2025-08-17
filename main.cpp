#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace std;

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
            newPos[(i + 1) % k] += j;

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

void bruteForce(const vector<int>& currentPos, int n, int player) {
  vector<vector<int>> moves = legalMoves(currentPos, n, precomputeUnitCircle(n));
  if (moves.empty()) {
    if (player == 1) {
      cout << "Player 1 wins!\n";
    } else if (player == -1) {
      cout << "Player 2 wins!\n";
    } else {
      cout << "Nesto se opasno desilo.\n";
    }
    return;
  }
  
  for (const vector<int>& move : moves) {
    for (int val : currentPos) {
      cout << val << " ";
    }
    cout << " -> ";
    bruteForce(move, n, -player);
  }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <number_of_points> <number_of_counters>\n";
        return 1;
    }

    // Read number of points in polygon and number of counters from command line arguments
    int n = atoi(argv[1]);
    int k = atoi(argv[2]);

    // Starting positions
    vector<int> startPos(k - 1, 1);
    startPos.push_back(n - k + 1);

    // Precompute circle coordinates
    vector<pair<double,double>> circle = precomputeUnitCircle(n);

    // Get coordinates and area
    vector<pair<double,double>> coordinates = getCoordinates(startPos, circle);
    double area = getArea(coordinates);

    vector<vector<int>> moves = legalMoves(startPos, n, circle);

    // Output
    cout << "Starting positions: ";
    for (int val : startPos) cout << val << " ";
    cout << "\nCoordinates of the polygon vertices: ";
    for (auto &coord : coordinates) cout << "(" << coord.first << "," << coord.second << ") ";
    cout << "\nArea of the polygon: " << area << "\nLegal moves from the current position:\n";

    for (auto &move : moves) {
        for (int val : move) cout << val << " ";
        cout << endl;
    }

    bruteForce(startPos, n, 1);

    return 0;
}
