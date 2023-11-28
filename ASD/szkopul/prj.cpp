#include <bits/stdc++.h>

using namespace std;

const int MAXN = 1e5 + 5;

int n, m, k, maxW;

// v ∈ G[u] <=> v jest zależne od u
vector<int> G[MAXN];
int dependences[MAXN];
int weights[MAXN];

int main() {
    ios_base::sync_with_stdio(0);
    cin.tie(0);

    cin >> n >> m >> k;
    for (int i = 1; i <= n; i++)
        cin >> weights[i];
    int dependant, dependency;
    for (int i = 0; i < m; i++) {
        cin >> dependant >> dependency;
        G[dependency].push_back(dependant);
        dependences[dependant]++;
    }
    priority_queue<pair<int, int>> Q;
    for (int i = 1; i <= n; i++)
        if (dependences[i] == 0)
            Q.push({-weights[i], i});

    while (k--) {
        auto [w, u] = Q.top();
        Q.pop();
        maxW = max(maxW, -w);
        for (auto v : G[u]) {
            dependences[v]--;
            if (dependences[v] == 0)
                Q.push({-weights[v], v});
        }
    }

    cout << maxW << '\n';
}