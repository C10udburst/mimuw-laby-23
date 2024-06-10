#include <iostream>
#include <sstream>
#include "humanplayer.h"
#include "utils.h"

namespace client {
    kierki::Card HumanPlayer::parse_cmd(const std::string &line, const Player &player) {
        if (line.starts_with("!")) {
            auto ss2 = std::stringstream(line.substr(1));
            kierki::Card card;
            ss2 >> card;
            return card;
        }
        if (line.starts_with("cards")) {
            std::cout << "Your cards: ";
            client::print_list(player.hand);
            std::cout << std::endl;
        }
        if (line.starts_with("tricks")) {
            std::cout << "Taken tricks: \n";
            for (auto &trick: player.taken) {
                client::print_list(trick);
                std::cout << std::endl;
            }
            std::cout << std::endl;
        }
        return kierki::Card{};
    }
} // client