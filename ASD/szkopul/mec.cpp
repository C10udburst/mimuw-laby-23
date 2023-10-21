#include <bits/stdc++.h>

using namespace std;

const int max_n = 4e4;
const int max_m = 50;

bitset<max_m> players[max_n];

int main() {
    cin.tie(0);
    ios_base::sync_with_stdio(0);

    int n, m;
    cin >> n >> m;

    int player;
    for (int match = 0; match < m; match++) {
        for (int i = 0; i < n; i++) {
            cin >> player;
            players[player - 1][match] = (i >= (n / 2)) ? 1 : 0; 
        }
    }

    unordered_set<bitset<max_m>> player_set;

    for (int i = 0; i < n; i++) {
        if (!player_set.insert(players[i]).second) {
            cout << "NIE";
            return 0;
        }
    }

    cout << "TAK";
    return 0;
}