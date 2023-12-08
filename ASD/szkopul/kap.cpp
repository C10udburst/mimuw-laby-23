#include <bits/stdc++.h>

using namespace std;

typedef tuple<int, int, int> island;

constexpr int id(const island& w) {
    return get<2>(w);
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(0);

    int n;
    cin >> n;
    
    vector<island> v(n);
    vector<vector<island>> g(n);

    for (int i = 0; i < n; i++) {
        int a, b;
        cin >> a >> b;
        v[i] = make_tuple(a, b, i);
    }

    island first = v.front();
    island last = v.back();
    
    sort(v.begin(), v.end(),
        [](const auto& a, const auto& b) {
            return get<0>(a) < get<0>(b);
        }
    );


    for (int i = 0; i < n; i++) {
        if (i > 0) {
            g[id(v[i])].push_back(v[i - 1]);
        }
        if (i < n - 1) {
            g[id(v[i])].push_back(v[i + 1]);
        }
    }

    sort(v.begin(), v.end(),
        [](const auto& a, const auto& b) {
            return get<1>(a) < get<1>(b);
        }
    );

    for (int i = 0; i < n; i++) {
        if (i > 0) {
            g[id(v[i])].push_back(v[i - 1]);

        }
        if (i < n - 1) {
            g[id(v[i])].push_back(v[i + 1]);
        }
    }

    vector<int> dist(n, -1);
    priority_queue<pair<int, island>> q;
    q.push(make_pair(0, first));
    dist[id(first)] = 0;
    while (!q.empty()) {
        auto [d, w] = q.top();
        q.pop();
        d = -d;
        if (d < dist[id(w)]) {
            continue;
        }
        for (auto& neigh : g[id(w)]) {
            auto [x, y, id] = neigh;
            int ndist = d + min(abs(get<0>(w) - x), abs(get<1>(w) - y));
            if (dist[id] == -1 || ndist < dist[id]) {
                dist[id] = ndist;
                q.push(make_pair(-ndist, neigh));
            }
        }
    }
    cout << dist[id(last)];
}