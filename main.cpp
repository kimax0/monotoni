#include <bits/stdc++.h>
using namespace std;

// ------------------------------ Globals & Types ------------------------------
static unordered_map<string, int8_t> TT; // transposition table
static const double EPS = 1e-12;

struct Move {
    vector<int> pos;
    double area;
};

// ------------------------------ Geometry ------------------------------
vector<pair<double, double>> precomputeUnitCircle(int n) {
    vector<pair<double, double>> circle(n);
    const double tau = 2.0 * M_PI;
    for (int i = 0; i < n; ++i) {
        double ang = tau * (static_cast<double>(i) / static_cast<double>(n));
        circle[i] = {cos(ang), sin(ang)};
    }
    return circle;
}

inline vector<pair<double, double>> getCoordinates(const vector<int>& currentPos,
                                                   const vector<pair<double, double>>& circle) {
    vector<pair<double,double>> coordinates;
    coordinates.reserve(currentPos.size());
    int index = 0;
    int mod = static_cast<int>(circle.size());
    for (int gap : currentPos) {
        coordinates.push_back(circle[index % mod]);
        index += gap;
    }
    return coordinates;
}

inline double getArea(const vector<pair<double, double>>& coordinates) {
    double area2 = 0.0;
    const int k = (int)coordinates.size();
    int j = k - 1;
    for (int i = 0; i < k; ++i) {
        area2 += (coordinates[j].first + coordinates[i].first) * (coordinates[j].second - coordinates[i].second);
        j = i;
    }
    return fabs(area2 / 2.0);
}

// ------------------------------ Canonicalization ------------------------------
template <class It1, class It2>
inline int lexCompare(It1 a_begin, It1 a_end, It2 b_begin, It2 b_end) {
    for (; a_begin != a_end && b_begin != b_end; ++a_begin, ++b_begin) {
        if (*a_begin < *b_begin) return -1;
        if (*a_begin > *b_begin) return 1;
    }
    if (a_begin == a_end && b_begin == b_end) return 0;
    return (a_begin == a_end ? -1 : 1);
}

// Booth's algorithm for minimal rotation index (O(k))
int minimalRotationIndex(const vector<int>& s) {
    int n = (int)s.size();
    vector<int> ss(2*n);
    for (int i = 0; i < n; ++i) { ss[i] = ss[i+n] = s[i]; }
    int i = 0, j = 1, k = 0;
    while (i < n && j < n && k < n) {
        int a = ss[i+k], b = ss[j+k];
        if (a == b) { ++k; continue; }
        if (a > b) i = i + k + 1;
        else       j = j + k + 1;
        if (i == j) ++j;
        k = 0;
    }
    return min(i,j);
}

inline void minRotation(const vector<int>& s, vector<int>& out) {
    int n = (int)s.size();
    int idx = minimalRotationIndex(s);
    out.resize(n);
    for (int t = 0; t < n; ++t) out[t] = s[(idx + t) % n];
}

string canonicalKey(const vector<int>& pos) {
    vector<int> rot, rot_rev, rev(pos.rbegin(), pos.rend());
    minRotation(pos, rot);
    minRotation(rev, rot_rev);
    int cmp = lexCompare(rot.begin(), rot.end(), rot_rev.begin(), rot_rev.end());
    const vector<int>& best = (cmp <= 0 ? rot : rot_rev);
    string key;
    key.reserve(best.size() * 4);
    for (int x : best) { key += to_string(x); key.push_back(','); }
    return key;
}

// include player in the key so TT entries aren't reused for different players
inline string canonicalKeyWithPlayer(const vector<int>& pos, int player) {
    string k = canonicalKey(pos);
    k.push_back('|');
    k.push_back((player == 1) ? '1' : '2');
    return k;
}

// ------------------------------ Move generation ------------------------------
vector<Move> legalMoves(const vector<int>& currentPos,
                        const int n,
                        const vector<pair<double,double>>& circle)
{
    vector<Move> moves;
    const int k = (int)currentPos.size();
    const double currentArea = getArea(getCoordinates(currentPos, circle));
    unordered_set<string> seen;
    seen.reserve(k * 8);

    const string currentCanon = canonicalKey(currentPos);

    auto try_push = [&](vector<int>& newPos) {
        // skip if equivalent to current position under rotation/reflection
        string newCanon = canonicalKey(newPos);
        if (newCanon == currentCanon) return;

        // area test
        double newArea = getArea(getCoordinates(newPos, circle));
        if (newArea > currentArea + EPS) {
            if (seen.insert(newCanon).second) {
                moves.push_back({ std::move(newPos), newArea });
            }
        }
    };

    for (int i = 0; i < k; ++i) {
        for (int j = 1; j < currentPos[i]; ++j) {
            vector<int> newPos = currentPos;
            newPos[i] -= j;
            newPos[(k + i - 1) % k] += j;
            try_push(newPos);
        }
        for (int j = 1; j < currentPos[(k + i - 1) % k]; ++j) {
            vector<int> newPos = currentPos;
            newPos[i] += j;
            newPos[(i - 1 + k) % k] -= j;
            try_push(newPos);
        }
    }

    // order moves by descending resulting area -> improves alpha-beta pruning
    sort(moves.begin(), moves.end(), [](const Move& a, const Move& b){ return a.area > b.area; });
    return moves;
}

// ------------------------------ Minimax with Alpha-Beta + Memo ------------------------------
int solve(const vector<int>& currentPos,
          const int n,
          const vector<pair<double,double>>& circle,
          int player,
          int alpha,
          int beta)
{
    // TT lookup must include player to move
    const string keyWithPlayer = canonicalKeyWithPlayer(currentPos, player);
    if (auto it = TT.find(keyWithPlayer); it != TT.end()) return it->second;

    vector<Move> moves = legalMoves(currentPos, n, circle);

    if (moves.empty()) {
        // preserve original semantics: return 1 if player==1 else -1
        int result = (player == 1 ? 1 : -1);
        TT.emplace(keyWithPlayer, (int8_t)result);
        return result;
    }

    int best;
    if (player == 1) {
        best = INT_MIN;
        for (size_t i = 0; i < moves.size(); ++i) {
            int val = solve(moves[i].pos, n, circle, -1, alpha, beta);
            if (val > best) best = val;
            if (best > alpha) alpha = best;
            if (beta <= alpha) break;
        }
    } else {
        best = INT_MAX;
        for (size_t i = 0; i < moves.size(); ++i) {
            int val = solve(moves[i].pos, n, circle, +1, alpha, beta);
            if (val < best) best = val;
            if (best < beta) beta = best;
            if (beta <= alpha) break;
        }
    }

    TT.emplace(keyWithPlayer, (int8_t)best);
    return best;
}

// ------------------------------ Main ------------------------------
int main(int argc, char* argv[]) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <number_of_points> <number_of_counters>\n";
        return 1;
    }

    const int n = atoi(argv[1]);
    const int k = atoi(argv[2]);

    vector<int> startPos(k - 1, 1);
    startPos.push_back(n - k + 1);

    const vector<pair<double,double>> circle = precomputeUnitCircle(n);
    const vector<pair<double,double>> coordinates = getCoordinates(startPos, circle);
    const double area = getArea(coordinates);

    cout << "Starting positions: ";
    for (int val : startPos) cout << val << " ";
    cout << "\nCoordinates of the polygon vertices: ";
    for (auto &coord : coordinates) cout << "(" << coord.first << "," << coord.second << ") ";
    cout << "\nArea of the polygon: " << area << endl;

    const int winner = solve(startPos, n, circle, /*player=*/1, /*alpha=*/INT_MIN, /*beta=*/INT_MAX);
    cout << "Winner: ";
    if (winner == 1) cout << "Player 1\n"; 
    else if (winner == -1) cout << "Player 2\n";
    else cout << "No winner\n";

    return 0;
}

// g++ -O3 -march=native -DNDEBUG -o main main.cpp

