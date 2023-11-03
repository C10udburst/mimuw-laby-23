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


const int K = 10 + 1;
const int N = 2e5 + 1;
const int BASE = 1 << 16;

int n, k;
ModInt tree[K][BASE*2 + 1];
int input[N];

void add(int k, int node, int v) {
    for (node+=BASE; node > 0; node /= 2) {
        tree[k][node] += v;
    }
}

ModInt query(int k, int left, int right) {
    left+=BASE; right+=BASE;

    ModInt result = tree[k][left] + tree[k][right];
    
    for (; left/2 < right/2; left /= 2, right /= 2) {
        if (left % 2 == 0) {
            result += tree[k][left+1];
        }
        if (right % 2 == 1) {
            result += tree[k][right-1];
        }
    }

    return result;
}

int main() {
    cin.tie(0);
    ios_base::sync_with_stdio(0);

    cin >> n >> k;
    for (int i = 1; i <= n; i++) {
        cin >> input[i];
    }

    for (int i = n; i >= 1; i--) {
        add(1, input[i], 1);
        for (int j = 2; j <= k; j++) {
            add(j, input[i], query(j-1, 0, input[i]-1));
        }
    }

    cout << query(k, 0, n);
}