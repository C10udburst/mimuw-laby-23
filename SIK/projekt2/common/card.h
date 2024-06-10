#ifndef PROJEKT2_CARD_H
#define PROJEKT2_CARD_H

#include <ostream>

namespace kierki {

    enum class Rank : int16_t {
        TWO = 0,
        THREE = 1,
        FOUR = 2,
        FIVE = 3,
        SIX = 4,
        SEVEN = 5,
        EIGHT = 6,
        NINE = 7,
        TEN = 8,
        JACK = 9,
        QUEEN = 10,
        KING = 11,
        ACE = 12
    };

    enum class Suit : char {
        NIL = -1,
        CLUBS = 0,
        DIAMONDS = 1,
        HEARTS = 2,
        SPADES = 3,
    };

    struct Card {
    public:
        Rank rank;
        Suit suit = Suit::NIL;

        void marknull();
        [[nodiscard]] std::string to_string() const;

        [[nodiscard]] bool isnull() const;
    };
}

bool operator==(const kierki::Card &lhs, const kierki::Card &rhs);

bool operator<(const kierki::Card &lhs, const kierki::Card &rhs);

bool operator>(const kierki::Card &lhs, const kierki::Card &rhs);

std::istream &operator>>(std::istream &is, kierki::Card &card);

std::ostream &operator<<(std::ostream &os, const kierki::Card &card);

#endif //PROJEKT2_CARD_H
