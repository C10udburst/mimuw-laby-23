#include <bits/stdc++.h>

using namespace std;

const int MAX_N = 1e4 + 2;
const int MAX_K = 1e4 + 2;
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

ModInt wyniki[MAX_N];
ModInt ile_ciagow[MAX_K];

int main() {
    cin.tie(0);
    ios_base::sync_with_stdio(0);

    int n, k;
    cin >> n >> k;

    // domyślnie (0), (1), ..., (k)
    for (int ik = 0; ik <= k; ik++)
        ile_ciagow[ik] = 1;
    
    wyniki[1] = k+1;
    
    // obliczamy korzystając z dynamicznego programowania
    // ile_ciagow[ik] - ile jest ciągów długości i, które kończą się na k
    for (int d=2; d<=n; d++) {
        ModInt poprzedni = -1;
        for (int ik = 0; ik <= k; ik++) {
            ModInt nowy = ile_ciagow[ik];
            if (ik > 0)
                nowy += poprzedni;
            if (ik < k)
                nowy += ile_ciagow[ik+1];
            poprzedni = ile_ciagow[ik];
            ile_ciagow[ik] = nowy;
            wyniki[d] += nowy;
            
        }
    }

    while(n--) {
        int d;
        cin >> d;
        cout << wyniki[d] << " ";
    }
}