#include <bits/stdc++.h>

using namespace std;

const int BASE_LOG = 20;
const int BASE = 1 << BASE_LOG;
int n, m;

enum color {
    BLACK = 'B',
    WHITE = 'W',
    MIXED = 'M'
};

int tree[2 * BASE + 2];

color get_colour(int v, int h) {
    if (tree[v] == 0) return BLACK;
    if (tree[v] == (1<<h)) return WHITE;
    else return MIXED;
}

color get_color(int v, int h) {
    color c = get_colour(v, h);
    return c;
}

void set_color(int v, int h, color c) {
    if (c == WHITE)
        tree[v] = 1<<h;
    if (c == BLACK)
        tree[v] = 0;
    if (c == MIXED)
        tree[v] = tree[2*v] + tree[2*v + 1];
}

constexpr color inverse(color c) {
    if (c == WHITE) return BLACK;
    if (c == BLACK) return WHITE;
    return MIXED;
}

void update_path(int end) {
    int h = BASE_LOG;
    int i = 1;
    while (i < end && h > 0) {
        color i_color = get_color(i, h);
        if (i_color != MIXED) {
            set_color(2*i, h-1, i_color);
            set_color(2*i + 1, h-1, i_color);
        }
        int left = 2*i;
        int left_end = (left<<(h-1)) + (1<<(h-1)) - 1;
        if (left_end >= end) {
            i = left;
            h--;
        } else {
            i = left + 1;
            h--;
        }
    }
}

void color_range(int begin, int end, color c) {
    begin += BASE - 1; end += BASE - 1;
    update_path(begin);
    update_path(end);
    int h = 0;
    set_color(begin, h, c);
    set_color(end, h, c);
    int begin_parent = begin / 2;
    int end_parent = end / 2;
    while (begin_parent != end_parent) {
        if (get_color(begin_parent, h+1) != c) {
            if (begin % 2 == 0) {
                set_color(begin+1, h, c);
                set_color(begin_parent, h+1, MIXED);
            } else {
                if (get_color(begin_parent, h+1) == inverse(c)) {
                    set_color(begin - 1, h, inverse(c));
                }
                
                set_color(begin_parent, h+1, MIXED);
            }
        }

        if (get_color(end_parent, h+1) != c) {
            if (end % 2 == 1) {
                set_color(end-1, h, c);
                set_color(end_parent, h+1, MIXED);
            } else {
                if (get_color(end_parent, h+1) == inverse(c)) {
                    set_color(end + 1, h, inverse(c));
                }
                
                set_color(end_parent, h+1, MIXED);
            }
        }

        begin /= 2;
        end /= 2;
        begin_parent /= 2;
        end_parent /= 2;
        h++;
    }

    while (begin_parent > 0) {
        set_color(begin_parent, 0, MIXED);
        begin_parent /= 2;
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
            color_range(begin, end, WHITE);
        else
            color_range(begin, end, BLACK);
        
        cout << tree[1] << '\n';
    }

}