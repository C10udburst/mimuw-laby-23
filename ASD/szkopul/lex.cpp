#include <bits/stdc++.h>

using namespace std;

const int N = 300000 + 7;
const long long MOD = 1000696969;
const long long BASE = 37;

inline int length(int l, int r) { return r - l + 1; }

int n, m;
string s;
long long s_hash[N], pow_t[N];

void s_hash_make() {
    for (int i = 1; i < s.size(); i++) 
        s_hash[i] = ((s_hash[i - 1] * BASE + MOD) % MOD + s[i] + MOD) % MOD;
}

void pow_make() {
    pow_t[0] = 1;
    for (int i = 1; i < N; i++)
        pow_t[i] = ((pow_t[i - 1] * BASE + MOD) % MOD + MOD) % MOD;

}

inline long long range_hash(int l, int r) {
    return (s_hash[r] - (s_hash[l - 1] * pow_t[length(l, r)] + MOD) % MOD + MOD) % MOD;
}

int length(int l1, int r1, int l2, int r2) {
    int l = 0;
    int r = min(length(l1, r1), length(l2, r2));

    while (l != r) {
        int mid = (r + l) / 2;

        if (range_hash(l1, l1 + mid) == range_hash(l2, l2 + mid))
            l = mid + 1;
        else
            r = mid;
    }

    return r;
}

int query(int l1, int r1, int l2, int r2) {
    int foundLength = length(l1, r1, l2, r2);

    if (foundLength == length(l1, r1) && foundLength == length(l2, r2))
        return 0;
    if (foundLength == length(l1, r1)) // len(s1) < len(s2)
        return -1;
    if (foundLength == length(l2, r2)) // len(s1) > len(s2)
        return 1;
    if (s[l1 + foundLength] < s[l2 + foundLength]) // s1 < s2
        return -1;

    return 1;
}

constexpr char format(int result) {
    if (result == 0)
        return '=';
    if (result == -1)
        return '<';
    return '>';
}

int main() {
    ios_base::sync_with_stdio(0);
    cin.tie(0);

    cin >> n >> m >> s;
    s = "#" + s; // strażnik

    pow_make();
    s_hash_make();

    int a, b, c, d;
    for (int i = 0; i < m; i++) {
        cin >> a >> b >> c >> d;
        cout << format(query(a, b, c, d)) << "\n";
    }
}