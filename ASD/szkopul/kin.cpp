#include <bits/stdc++.h>

using namespace std;

const int MOD = 1e9;

struct ModInt {
    int val;

    ModInt() : val(0) {}

    ModInt(int v) {
        val = v % MOD;
        if (val < 0) val += MOD;
    }

    operator int() const {
        return val;
    }

    struct ModInt& operator+=(const struct ModInt& rhs) {
        val += rhs.val;
        val = val % MOD;
        return *this;
    }

    struct ModInt& operator+=(const int& rhs) {
        val += rhs;
        val = val % MOD;
        return *this;
    }

    struct ModInt operator+(const struct ModInt& rhs) const {
        struct ModInt result = *this;
        result += rhs;
        return result;
    }

    struct ModInt operator+(const int& rhs) const {
        struct ModInt result = *this;
        result += rhs;
        return result;
    }

    constexpr struct ModInt& operator=(const int rhs) {
        val = rhs;
        return *this;
    }
};

const int TREE_SIZE = 1 << 15;
const int MAX_N = 2e4;
const int MAX_K = 10;

ModInt tree[TREE_SIZE * 2 + 3];
ModInt results_a[MAX_K + 3];
ModInt results_b[MAX_K + 3];
int sequence[MAX_N + 3];

int n, k;

#define DEBUG

// // następny przebieg bierze wartości z poprzedniego przebiegu
// // i wypełnia drzewo od korzenia do liści
// ModInt pass(int node, ModInt* results) {
//     tree[node] = results[node - n];
//     int parent = node / 2;
//     ModInt result = 0;
//     while (parent > 0) {
//         if (node % 2 == 0) {
//             result += tree[parent * 2 + 1];
//         }
//         tree[parent] += results[node - n];
//         node = parent;
//         parent /= 2;
//     }
//     return result;
// }

ModInt query(int node) {
    ModInt result = 0;
    for (; node > 0; node /= 2) {
        if (node % 2 == 0) {
            result += tree[node + 1];
        }
    }
    return result;
}

void update(int node, ModInt v) {
    tree[node] = v;
    int parent = node / 2;
    while (parent > 0) {
        if (node % 2 == 0) {
            tree[parent] += v;
        }
        node = parent;
        parent /= 2;
    }
}

#ifdef DEBUG
void print_tree() {
    for (int i = 0; i <= 2*n; i++) {
        cout << tree[i] << " ";
    }
    cout << "\n";
}
#endif

void reset_tree() {
    for (int i = 0; i <= 2*n; i++) {
        tree[i] = 0;
    }
}

int main() {
    cin.tie(0);
    ios_base::sync_with_stdio(0);

    cin >> n >> k;
    for (int i = 0; i < n; i++) {
        cin >> sequence[i];
    }

    // drzewo przedziałowe
    ModInt* results = results_a;
    ModInt* tmp_results = results_b;

    for (int i = 0; i <= k; i++) {
        results[i] = 1;
    }

    while(k--) {
        for (int i = 0; i < n; i++) {
            int val = sequence[i] - 1;
            int node = n + val; // liście zajmują przedział [n, 2*n)
            ModInt result = query(node);
            update(node, results[val]);
            tmp_results[val] = result;
            cout << "[" << val << "] " << result << "  ";
            #ifdef DEBUG
            print_tree();
            #endif
        }
        #ifdef DEBUG
        for (int i = 0; i < n; i++) {
            cout << results[i] << " ";
        }
        cout << "-> ";
        for (int i = 0; i < n; i++) {
            cout << tmp_results[i] << " ";
        }
        cout << "\n";
        #endif

        // zamieniamy wskaźniki
        ModInt* tmp = results;
        results = tmp_results;
        tmp_results = tmp;

        reset_tree();
    }

    ModInt final_result = 0;
    for (int i = 0; i < n; i++) {
        final_result += results[i];
    }

    cout << final_result;
}