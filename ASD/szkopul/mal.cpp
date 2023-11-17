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