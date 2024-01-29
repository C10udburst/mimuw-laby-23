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

// ------



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
    ordered_set<int> from_start; // we will invert nums to store max at find_by_order(0)
    ordered_set<int> to_end;

    cin >> n >> m;
    char c;
    int l, r;
    while (m--) {
        cin >> c >> l >> r;
        if (c == '+') {
            if (l == 1) { // from_start
                from_start.insert(-r);
            } else { // to_end
                to_end.insert(l);
            }
        } else { // c == '-'
            if (l == 1) { // from_start
                from_start.erase(-r);
            } else { // to_end
                to_end.erase(l);
            }
        }
        int uncovered = n; // amount of ints not covered by any group
        if (!from_start.empty()) {
            int max_from_start = -*from_start.find_by_order(0);
            uncovered -= max_from_start; // we cover all integers from 1 to max_from_start
        }
        if (!to_end.empty()) {
            int min_to_end = *to_end.find_by_order(0);
            uncovered -= n - min_to_end + 1; // we cover all integers from min_to_end to n
        }
        uncovered = max(uncovered, 0); // edge case: all integers are covered
        cout << uncovered << endl;
    }


    // ------

    return 0;
}