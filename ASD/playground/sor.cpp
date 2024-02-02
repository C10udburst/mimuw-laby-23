#define IO_SPEED 1
#define LOCAL 0

//region Imports
#include <bits/stdc++.h>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>
#include <ext/pb_ds/trie_policy.hpp>
#include <ext/pb_ds/tag_and_trait.hpp>
#include <ext/rope>

using namespace std;
using namespace __gnu_pbds;
using namespace __gnu_cxx;
//endregion

//region Trees
template <class key, class cmp = less<key>>
using ordered_set = tree<key, null_type, cmp, rb_tree_tag, tree_order_statistics_node_update>;
template <class key, class cmp = less_equal<key>>
using ordered_multiset = tree<key, null_type, cmp, rb_tree_tag, tree_order_statistics_node_update>;
//* find_by_order(k) - returns an iterator to the k-th largest element (counting from zero)
//* order_of_key(k)  - the number of items in a set that are strictly smaller than k.
template <class key, class value, class cmp = less<key>>
using ordered_map = tree<key, value, cmp, rb_tree_tag, tree_order_statistics_node_update>;
using pref_trie = trie<string, null_type, trie_string_access_traits<>, pat_trie_tag, trie_prefix_search_node_update>;
// * prefix_range(s) - return iterator range, prefix equal to s
//endregion

//region ModInt
template <class T, int MOD>
struct modular
{
    T val;
    explicit operator T() const { return val; }
    modular() { val = 0; }
    explicit modular(const long long &v)
    {
        val = (-MOD <= v && v <= MOD) ? v : v % MOD;
        if (val < 0)
            val += MOD;
    }

    friend ostream &operator<<(ostream &os, const modular &a) { return os << a.val; }

    friend bool operator==(const modular &a, const modular &b) { return a.val == b.val; }
    friend bool operator!=(const modular &a, const modular &b) { return !(a == b); }
    friend bool operator<(const modular &a, const modular &b) { return a.val < b.val; }

    modular operator-() const { return modular(-val); }
    modular &operator+=(const modular &m)
    {
        if ((val += m.val) >= MOD)
            val -= MOD;
        return *this;
    }
    modular &operator-=(const modular &m)
    {
        if ((val -= m.val) < 0)
            val += MOD;
        return *this;
    }
    modular &operator*=(const modular &m)
    {
        val = (long long)val * m.val % MOD;
        return *this;
    }
    friend modular pow(modular a, long long p)
    {
        modular ans = 1;
        for (; p; p /= 2, a *= a)
            if (p & 1)
                ans *= a;
        return ans;
    }
    friend modular inv(const modular &a)
    {
        auto i = invGeneral(a.val, MOD);
        assert(i != -1);
        return i;
    } // equivalent to return exp(b,MOD-2) if MOD is prime
    modular &operator/=(const modular &m) { return (*this) *= inv(m); }

    friend modular operator+(modular a, const modular &b) { return a += b; }
    friend modular operator-(modular a, const modular &b) { return a -= b; }
    friend modular operator*(modular a, const modular &b) { return a *= b; }
    friend modular operator/(modular a, const modular &b) { return a /= b; }
};
//endregion

//region Misc Functions

template <class T>
inline T ceil(T a, T b) { return (a + b - 1) / b; }

#define all(x) (x).begin(), (x).end()

#define forit(s) for (auto &it : s)

//endregion

//region IO
template <typename A, typename B>
istream &operator>>(istream &in, pair<A, B> &a) { return in >> a.first >> a.second; }
template <typename A, typename B>
ostream &operator<<(ostream &out, pair<A, B> &a) { return out << a.first << ' ' << a.second; }

template <typename T>
inline void read_n(T *arr, int n)
{
    for (int i = 0; i < n; i++)
        cin >> arr[i];
}
template <typename T>
inline void read_vec(vector<T> &arr, int n)
{
    arr.resize(n);
    for (int i = 0; i < n; i++)
        cin >> arr[i];
}
//endregion

//region Debug print
namespace debug
{

#ifdef LOCAL
#define cerr cout
#define here                                                 \
    {                                                        \
        cerr << "\n[" << __LINE__ << "]\n"; \
    }
#define debug(...) _debug(#__VA_ARGS__, __VA_ARGS__)
template <typename _T>
inline void _debug(const char *s, _T x) { cerr << s << " = " << x << "\n"; }
template <typename _T, typename... args>
void _debug(const char *s, _T x, args... a)
{
    while (*s != ',')
        cerr << *s++;
    cerr << " = " << x << ',';
    _debug(s + 1, a...);
}
#else
#define debug(...) 1999
#define cerr \
    if (0)   \
    cout
#define here
#endif
} // namespace debug
using namespace debug;
//endregion

constexpr int MOD = 1000696969;
#define endl '\n'

using modint = modular<int, MOD>;

/*
Pierwszy wiersz wejścia zawiera dwie liczby całkowite n i m (1 ≤ n, m ≤ 250 000) oznaczające długość ciągu c
oraz liczbę zapytań do Twojego programu. Drugi wiersz zawiera ciąg zadany jako napis długości n składający
się z cyfr 0 i 1. Zakładamy, że pozycje ciągu są ponumerowane od 1 do n, od lewej do prawej.
W kolejnych m wierszach zapisano zapytania do programu. Każde zapytanie składa się ze znaku oraz co
najmniej jednej liczby. Dopuszczalne są trzy typy zapytań:
• „< i j” – posortuj fragment ciągu ci
, . . . , cj niemalejąco
• „> i j” – posortuj fragment ciągu ci
, . . . , cj nierosnąco
• „? i” – podaj aktualną wartość i-tej cyfry ciągu.
Możesz założyć, że będzie co najmniej jedno zapytanie typu ?.
 */

// ------

#define min(a, b) ((a) < (b) ? (a) : (b))

void sort_bitset(bitset<250000> &arr, int i, int j, bool asc) {
    int ones = 0;
    bitset<250000> mask;

    int len = j - i + 1;
    while (len > 0) {
        mask = (1 << min(len, 64)) - 1;
        len -= 64;
    }
    mask <<= i;

    mask &= arr;
    ones = (int)mask.count();

    arr &= ~mask;

    if (asc) {
        while (ones > 0) {
            mask = (1 << min(ones, 64)) - 1;
            mask <<= j - min(ones, 64) + 1;
            arr |= mask;
            ones -= 64;
            j -= min(ones, 64);
        }
    } else  {
        while (ones > 0) {
            mask = (1 << min(ones, 64)) - 1;
            mask <<= i;
            arr |= mask;
            ones -= 64;
            i += min(ones, 64);
        }
    }
}

//#define sort_bitset sort_bitset_slow

/*
void sort_bitset_slow(bitset<250000> &arr, int i, int j, bool asc) {
    int ones = 0;
    for (int k = i; k <= j; k++) {
        ones += arr.test(k);
        arr.reset(k);
    }
    if (asc) {
        for (int k = j; k > j - ones; k--)
                arr.set(k);
    } else {
        for (int k = i; k < i + ones; k++)
            arr.set(k);
    }
}
 */

// ------

int main(int argc, char **argv) {
#if IO_SPEED > 0
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);
    cout.tie(nullptr);
    cerr.tie(nullptr);
#endif

    // ------

    int n, m;
    cin >> n >> m;
    char c;
    bitset<250000> arr;
    for (int i = 0; i < n; i++) {
        cin >> c;
        arr[i] = c - '0';
    }

    int i, j;
    while (m--) {
        cin >> c;
        if (c == '<') {
            cin >> i >> j;
            sort_bitset(arr, i - 1, j - 1, true);
        } else if (c == '>') {
            cin >> i >> j;
            sort_bitset(arr, i - 1, j - 1, false);
        } else {
            cin >> i;
            cout << (arr.test(i - 1) ? 1 : 0) << endl;
        }
    }

    // ------

    return 0;
}