#ifndef PROJEKT2_DEAL_H
#define PROJEKT2_DEAL_H

#include "../common/rules.h"

namespace client {
    struct Deal {
        kierki::Rule rule;
        char first_player;
        std::array<kierki::Card, 13> cards;
    };
}

std::istream &operator>>(std::istream &is, client::Deal &deal);

#endif //PROJEKT2_DEAL_H
