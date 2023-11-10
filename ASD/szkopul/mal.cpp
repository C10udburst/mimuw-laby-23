#include <bits/stdc++.h>

using namespace std;

const int BASE = 1 << 3;
int n, m;

enum color {
    BLACK = 0,
    WHITE = 1,
    MIXED = 2
};

struct node {
    int count;
    color c;
};

node tree[2 * BASE + 2];

void black(int begin, int end) {
    
    
}

void white(int begin, int end) {
    if (begin == end) {
        tree[begin + BASE].c = WHITE;
        tree[begin + BASE].count = 1;
        return;
    }
    int l = begin + BASE;
    int r = end + BASE;
    tree[l].c = WHITE;
    tree[r].c = WHITE;
    tree[l].count = 1;
    tree[r].count = 1;
    while (l/2 < r/2) {
        if (l % 2 == 0) {
            tree[l + 1].c = WHITE;
            tree[l + 1].count = 1;
        }
        l /= 2;
        r /= 2;
    }
}

int main() {
    cin.tie(0);
    ios_base::sync_with_stdio(0);

    cin >> n >> m;
    while(m--) {
        int begin, end;
        char color;
        cin >> begin >> end >> color;
        
        if (color == 'B')
            white(begin, end);
        else
            black(begin, end);
        
        cout << tree[1].count << '\n';
    }

}