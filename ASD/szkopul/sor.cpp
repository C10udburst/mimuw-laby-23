#include <cstdio>
#include <iostream>

using namespace std;

const int MAX_N = 1e3;
const int MOD = 1e9;
const int LEFT = 0;
const int RIGHT = 1;

struct ModInt {
    int val;

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

ModInt tab[MAX_N][MAX_N][2];
int n;
int s[MAX_N];

ModInt T(int i, int j, int dir) {
    // (i, j) przedział oryginalnej tablicy który pozostał po jakiejś ilości cofnięć
    // dir - kierunek ostatniego cofnięcia
    if (tab[i][j][dir] != -1)
        return tab[i][j][dir]; // już policzone wcześniej

    tab[i][j][dir] = 0;

    if (i == j) { // przedział jednoelementowy
        // ponieważ nie chcemy podwójnie liczyć, załóżmy że cofnięcie było w lewo
        tab[i][j][dir] = dir == LEFT ? 1 : 0;
        return tab[i][j][dir];
    }

    if (dir == LEFT) {
        if (s[i] < s[i + 1]) {
            tab[i][j][dir] += T(i + 1, j, LEFT);
        }
        if (s[i] < s[j]) {
            tab[i][j][dir] += T(i + 1, j, RIGHT);
        }
    } else {
        if (s[j] > s[j - 1]) {
            tab[i][j][dir] += T(i, j - 1, RIGHT);
        }
        if (s[j] > s[i]) {
            tab[i][j][dir] += T(i, j - 1, LEFT);
        }
    }

    return tab[i][j][dir];
}

int main() {

    scanf("%d", &n);
    for (int i = 0; i < n; i++) {
        scanf("%d", &s[i]);
    }
    
    // wypełnianie tablicy -1
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++)
            tab[i][j][LEFT] = tab[i][j][RIGHT] = -1;
    }

    printf("%d\n", T(0, n - 1, LEFT) + T(0, n - 1, RIGHT));

    return 0;
}
