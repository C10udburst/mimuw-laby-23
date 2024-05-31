#include <array>
#include "autoplayer.h"

kierki::Card client::AutoPlayer::play(std::array<kierki::Card, 4> table, client::Player &player) {
    if (table[0].isnull()) {
        // if we are the first to play, we play the smallest card we have
        // regardless of suit
        auto min = player.hand[0];
        for (auto card : player.hand) {
            if (min.isnull() || (!card.isnull() && card.rank < min.rank))
                min = card;
        }
        return min;
    } else {
        // we find who played the biggest card
        auto table_max = table[0];
        for (auto card : table) {
            if (card > table_max)
                table_max = card;
        }
        // we find the biggest card we have that is smaller than the biggest card on the table
        auto minmax = kierki::Card{};
        for (auto card : player.hand) {
            // card < table_max is only true if card matches the suit of table_max
            if (card < table_max && card > minmax)
                minmax = card;
        }
        if (minmax.isnull()) {
            // minmax is null, meaning we don't have a card that matches the suit of table_max
            // we choose the biggest card that matches the suit
            for (auto card : player.hand) {
                if (card.suit == table_max.suit && card > minmax)
                    minmax = card;
            }
            // if we still don't have a card that matches the suit of table_max
            // we choose the biggest card we have
            for (auto card : player.hand) {
                if (card > minmax)
                    minmax = card;
            }
        }
        return minmax;
    }
}
