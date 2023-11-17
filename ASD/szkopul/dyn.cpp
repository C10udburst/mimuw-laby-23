#include "splay.hpp"
#include <bits/stdc++.h>

using namespace std;

struct NodeData {

};

bool operator<(const NodeData &a, const NodeData &b) {
  return false;
}


splay_tree<NodeData> tree;
int n = 0;
int w = 0;

void do_insert() {
    // k - count, x - value, j - position
    int jp, x, k;
    cin >> jp >> x >> k;
    int j = (jp + w) % (n+1);
    // TODO: insert
    n += k;
}

void do_get() {
    // j - position
    int jp;
    cin >> jp;
    int j = (jp + w) % n;
    // TODO: get into w
    cout << w << '\n';
}

int main() {
    int m;
    cin >> m;
    while (m--) {
        char c;
        cin >> c;
        if (c == 'i') do_insert();
        else if (c == 'g') do_get();
    }
}