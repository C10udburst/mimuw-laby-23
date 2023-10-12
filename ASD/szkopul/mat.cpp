#include <string>
#include <iostream>
#include <climits>

using namespace std;

int main() {
    ios_base::sync_with_stdio(0);
    cin.tie(0);
    string s;
    cin >> s;
    
    char prev_c = s[0];
    int prev_i = 0;
    int res = INT_MAX;
    
    for (int i = 1; i < s.size(); i++) {
        if (s[i] != '*') {
            if (s[i] != prev_c && prev_c != '*') {
                res = min(res, i - 1 - prev_i);
            }
            prev_c = s[i];
            prev_i = i;
        }
        if (res == 0)
            break;
    }

    if (res == INT_MAX) {
        std::cout << 1;
    } else {
        std::cout << s.size() - res;
    }
}