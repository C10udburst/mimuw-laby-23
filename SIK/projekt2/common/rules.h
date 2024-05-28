#ifndef PROJEKT2_RULES_H
#define PROJEKT2_RULES_H

#include "card.h"
#include <array>

namespace kierki {
    enum Rule {
        // oznacza błąd
        NIL = 0,
        // nie brać lew, za każdą wziętą lewę dostaje się 1 punkt;
        ANY = 1,
        // nie brać kierów, za każdego wziętego kiera dostaje się 1 punkt;
        NO_HEARTS = 2,
        // nie brać dam, za każdą wziętą damę dostaje się 5 punktów;
        NO_QUEENS = 3,
        // nie brać panów (waletów i króli), za każdego wziętego pana dostaje się 2 punkty;
        NO_MEN = 4,
        // nie brać króla kier, za jego wzięcie dostaje się 18 punktów;
        NO_KING_OF_HEARTS = 5,
        // nie brać siódmej i ostatniej lewy, za wzięcie każdej z tych lew dostaje się po 10 punktów;
        NO_7_13 = 6,
        // rozbójnik, punkty dostaje się za wszystko wymienione powyżej.
        ALL = 7
    };

    std::ostream &operator<<(std::ostream &os, const Rule &rule);
    std::istream &operator>>(std::istream &is, Rule &rule);
}

#endif //PROJEKT2_RULES_H
