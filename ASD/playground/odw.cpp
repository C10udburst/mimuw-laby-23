#define IO_SPEED 0

#include <bits/stdc++.h>
using namespace std;

/*int main() {
    #if IO_SPEED>0
        std::ios_base::sync_with_stdio(false);
        std::cin.tie(nullptr);
    #endif
    // środek -> lista początków odwróceń
    map<int, vector<int>> odw;
    map<int, int> chunk_size;

    int n, k;
    string str;
    cin >> n >> k >> str;
    while (k--) {
        int a, b;
        cin >> a >> b;

        int s = (a + b) / 2;
        chunk_size[s] = max(chunk_size[s], b - a + 1);
        int size = (b - a + 1) / 2;
        if (odw[s].size() < size) {
            odw[s].resize(size, 0);
        }
    }

    for (const auto& [s, v] : odw) {
        for (int i = 1; i < v.size(); i++) {
            odw[s][i] = (v[i] + v[i - 1]) % 2;
        }
    }

    int offset = 0;
    while (offset < str.size()) {
        for (const auto& [s, v]: odw) {
            for (int i = 0; i < v.size(); i++) {
                int idx = i;
                if (v[i] == 1) {
                    idx = chunk_size[s] - i - 1;
                }
                cout << str[idx + offset];
            }
            if (chunk_size[s] % 2 == 1) {
                cout << str[offset + s];
            }
            for (int i = 0; i < v.size(); i++) {
                int idx = v.size() - i - 1;
                if (v[v.size() - i - 1] == 0) {
                    idx = chunk_size[s] - i - 1;
                }
                cout << str[idx + offset];
            }
            offset += chunk_size[s];
        }
    }
}*/

int main() {
    #if IO_SPEED>0
        std::ios_base::sync_with_stdio(false);
        std::cin.tie(nullptr);
    #endif

    int n, k;
    string str;
    bool* odw = (bool*)malloc(n * sizeof(bool));

}