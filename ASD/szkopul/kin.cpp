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

int n, k, base;

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
    for (; node > 0; node /= 2) {
        tree[node] += v;
    }
}


void reset_tree() {
    for (int i = 0; i <= 2*n + 1; i++) {
        tree[i] = 0;
    }
}

int calculate2power(int n) {
    int result = 1;
    while (result < n) {
        result <<= 1;
    }
    return result;
}

int main() {
    cin.tie(0);
    ios_base::sync_with_stdio(0);

    cin >> n >> k;
    for (int i = 0; i < n; i++) {
        cin >> sequence[i];
        sequence[i] -= 1;
    }

    base = calculate2power(n);

    // drzewo przedziałowe
    ModInt* results = results_a;
    ModInt* tmp_results = results_b;

    for (int i = 0; i < n; i++) {
        results[i] = 1;
    }

    while(--k) {
        for (int i = 0; i < n; i++) {
            int val = sequence[i];
            int node = base + val; // liście zajmują przedział [base, 2*base)
            ModInt result = query(node);
            update(node, results[val]);
            tmp_results[val] = result;
        }

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