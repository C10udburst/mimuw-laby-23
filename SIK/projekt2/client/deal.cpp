#include "deal.h"

#include <istream>

std::istream &operator>>(std::istream &is, client::Deal &deal) {
    char rule;
    is.read(&rule, 1);
    deal.rule = static_cast<kierki::Rule>(rule - '0');
    is.read(&deal.first_player, 1);
    for (auto &card: deal.cards)
        is >> card;
    return is;
}
