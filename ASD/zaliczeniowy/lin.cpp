#include <bits/stdc++.h>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>

using namespace std;
using namespace __gnu_pbds;

// (t, a, b) - t - czas, a - początek, b - koniec
using punkt = tuple<int, int, int>;

int main()
{
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int n, k;

    cin >> n >> k;
    vector<punkt> punkty(n);
    
    for (int i = 0; i < n; i++)
    {
        tuple<int, int, int> a;
        cin >> get<1>(a) >> get<2>(a) >> get<0>(a);
        punkty[i] = a;
    }

    sort(punkty.begin(), punkty.end());

    tree<pair<int, int>, null_type, less<pair<int, int>>, rb_tree_tag, tree_order_statistics_node_update> poczatki;
    tree<pair<int, int>, null_type, less<pair<int, int>>, rb_tree_tag, tree_order_statistics_node_update> konce;

    long long kolizje = 0;

    priority_queue<tuple<int, int, int, int>> dodane;

    int i = 0;

    for (auto punkt: punkty)
    {
        while(!dodane.empty() && get<0>(punkt) + get<0>(dodane.top()) > k) // koszt zaamortyzowany wszystkich usunięć to O(n log n)
        {
            auto top = dodane.top();
            dodane.pop();
            auto pocz = poczatki.find({get<1>(top), get<3>(top)});
            poczatki.erase(pocz);
            auto kon = konce.find({get<2>(top), get<3>(top)});
            konce.erase(kon);
        }
        int a = poczatki.order_of_key({get<2>(punkt) + 1, 0});
        int b = konce.order_of_key({get<1>(punkt), 0});
        kolizje += (long long) (a - b);
        poczatki.insert({get<1>(punkt), i});
        konce.insert({get<2>(punkt), i});
        dodane.push({-get<0>(punkt), get<1>(punkt), get<2>(punkt), i});
        i++;
    }

    cout << kolizje;
}