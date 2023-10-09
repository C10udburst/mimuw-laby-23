#include <algorithm>
#include <cstdio>
#include <climits>
using namespace std;

typedef long long ll;

const int MAX_N = 1000000 + 1;
const int MINFTY = INT_MIN;
const int INFTY = INT_MAX;


int n, cena[MAX_N];
int max_par[MAX_N], max_npar[MAX_N];
ll wynik[MAX_N + 1];

void odczyt() {
    scanf("%d", &n);
    for (int i = 0; i < n; i++)
        scanf("%d", cena + i);
    // sort(cena, cena + n); dane są posortowane
}

void preprocess() {
    /*
        obliczamy max_par i max_npar
        czyli najw. l. t.że ich idx w tab. cena jest < niż k (indeks w tej tab.)
        max_npar[i] = cena[n-1-k] if cena[n-1-k]%2 else cena[n-1-k-1]
    */

    max_par[n] = max_npar[n] = MINFTY;
    for (int k = n-1; k > 0; k--) {
        if (cena[n-1-k] % 2) { // nieparzysta
            max_par[k] = max_par[k+1];
            max_npar[k] = cena[n-1-k];
        } else { // parzysta
            max_par[k] = cena[n-1-k];
            max_npar[k] = max_npar[k+1];
        }
    }

    /*
        wyliczanie wyników dla kolejnych k
        min_par, min_npar - najmniejsze parzyste i nieparzyste t.że ich idx w tab. cena jest >= niż k
    */

    ll sumaR = 0;
    int min_par = INFTY, min_npar = INFTY;
    for (int k = 1; k <= n; k++) {
        sumaR += cena[n-k];
        
        if (cena[n-k] % 2) // nieparzysta
            min_npar = cena[n-k];
        else // parzysta
            min_par = cena[n-k];
        
        if (sumaR % 2) // nieparzysta, mamy wynik
            wynik[k] = sumaR;
        else { // parzysta, trzeba coś zamienić
            wynik[k] = -1; // domyślnie suma nie istnieje

            // sprawdzamy czy można zamienić parzystą na nieparzystą
            if (min_par != INFTY && max_par[k] != MINFTY)
                wynik[k] = sumaR - min_par + max_par[k];
            
            // sprawdzamy czy można zamienić nieparzystą na parzystą
            if (min_npar != INFTY && max_npar[k] != MINFTY)
                wynik[k] = max(wynik[k], sumaR - min_npar + max_npar[k]);
        }
    }
}

int main() {
    odczyt();lst
    preprocess();
    int m;
    scanf("%d", &m);
    while (m--) {
        int k;
        scanf("%d", &k);
        printf("%lld\n", wynik[k]);
    }
    return 0;
}
