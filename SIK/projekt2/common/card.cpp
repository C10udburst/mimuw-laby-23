#include "card.h"
#include <string>
#include <sstream>

const std::basic_string rank2str = "23456789TJQKA";
const std::basic_string suit2str = "CDHS";

bool operator==(const kierki::Card &lhs, const kierki::Card &rhs) {
    if (lhs.isnull()) return false;
    return lhs.rank == rhs.rank && lhs.suit == rhs.suit;
}

bool operator<(const kierki::Card &lhs, const kierki::Card &rhs) {
    if (lhs.isnull()) return false;
    if (rhs.isnull()) return true;
    if (lhs.suit == rhs.suit) return lhs.rank < rhs.rank;
    return false;
}

bool operator>(const kierki::Card &lhs, const kierki::Card &rhs) {
    if (lhs.isnull()) return false;
    if (rhs.isnull()) return true;
    if (lhs.suit == rhs.suit) return lhs.rank > rhs.rank;
    return false;
}

std::istream &operator>>(std::istream &is, kierki::Card &card) {
    char rank, suit;
    is.read(&rank, 1);
    if (rank == '1') {
        // read the 0
        is.read(&rank, 1);
        if (rank != '0') {
            card.marknull();
            return is;
        }
        rank = 'T';
    }
    is.read(&suit, 1);
    auto found = rank2str.find(rank);
    if (found == std::string::npos) {
        card.marknull();
        return is;
    }
    card.rank = static_cast<kierki::Rank>(found);
    found = suit2str.find(suit);
    if (found == std::string::npos) {
        card.marknull();
        return is;
    }
    card.suit = static_cast<kierki::Suit>(found);

    return is;
}

std::ostream &operator<<(std::ostream &os, const kierki::Card &card) {
    if (card.isnull())
        return os;

    if (card.rank != kierki::Rank::TEN)
        os.write(rank2str.data() + static_cast<int>(card.rank), 1);
    else
        os.write("10", 2);

    os.write(suit2str.data() + static_cast<int>(card.suit), 1);

    return os;
}

void kierki::Card::marknull() {
    suit = Suit::NIL;
}

bool kierki::Card::isnull() const {
    return suit == Suit::NIL;
}