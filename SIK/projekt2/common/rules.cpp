#include "rules.h"
#include <sstream>

int32_t kierki::score(Rule rule, std::array<Card, 4> &hand, int trick) {
    int32_t score = 0;
    if (rule == Rule::ANY || rule == Rule::ALL)
        score++;
    if (rule == Rule::NO_HEARTS || rule == Rule::ALL) {
        for (auto &card: hand) {
            if (card.suit == Suit::HEARTS)
                score++;
        }
    }
    if (rule == Rule::NO_QUEENS || rule == Rule::ALL) {
        for (auto &card: hand) {
            if (card.rank == Rank::QUEEN)
                score += 5;
        }
    }
    if (rule == Rule::NO_MEN || rule == Rule::ALL) {
        for (auto &card: hand) {
            if (card.rank == Rank::JACK || card.rank == Rank::KING)
                score += 2;
        }
    }
    if (rule == Rule::NO_KING_OF_HEARTS || rule == Rule::ALL) {
        for (auto &card: hand) {
            if (card.rank == Rank::KING && card.suit == Suit::HEARTS)
                score += 18;
        }
    }
    if (rule == Rule::NO_7_13 || rule == Rule::ALL) {
        if (trick == 7 || trick == 13)
            score += 10;
    }
    return score;
}

std::ostream &kierki::operator<<(std::ostream &os, const kierki::Rule &rule) {
    os << static_cast<int>(rule);
    return os;
}

std::istream &kierki::operator>>(std::istream &is, kierki::Rule &rule) {
    char type;
    is.read(&type, 1);
    if (type < '0' || type > '7') {
        rule = Rule::NIL;
        return is;
    }
    rule = static_cast<Rule>(type - '0');
    return is;
}
