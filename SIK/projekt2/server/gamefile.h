#ifndef PROJEKT2_SERVER_GAMEFILE_H
#define PROJEKT2_SERVER_GAMEFILE_H

#include <mutex>
#include <fstream>
#include "../common/rules.h"

namespace server {
    struct Game {
        kierki::Rule rule = kierki::Rule::NIL;
        int8_t first_seat{};
        kierki::Card cards[4][13];
        kierki::Card cards_initial[4][13];
        kierki::Card table[4] = {};
    public:
         bool play_card(kierki::Card card, int seat);
         int calculate_score(int deal);
         int get_loser();
         void reset_table();
    };

    struct Gamefile {
        int current_game = -1;
        std::mutex mutex;
        Game loaded_game;
        std::ifstream file;

    public:
        explicit Gamefile(const std::string &filename);
        Game* get_game(int game_number);
        ~Gamefile();
    };
}

std::istream &operator>>(std::istream &is, server::Game &game);

#endif //PROJEKT2_SERVER_GAMEFILE_H
