#include <array>
#include <iostream>
#include <sstream>
#include "humanplayer.h"
#include "utils.h"
#include "../common/utils.h"

namespace client {
    kierki::Card HumanPlayer::play(std::array<kierki::Card, 4> __attribute__((unused)) table) {
        // TODO: allow server to send data while waiting for input
        while (true) {
            auto line = utils::readline(STDIN_FILENO, sizeof "!2H\n" - 1);
            auto card = parse_cmd(line);
            if (!card.isnull())
                return card;
        }
    }

    kierki::Card HumanPlayer::parse_cmd(const std::string &line) {
        if (line.starts_with("!")) {
            auto ss2 = std::stringstream(line.substr(1));
            kierki::Card card;
            ss2 >> card;
            return card;
        }
        if (line.starts_with("cards")) {
            std::cout << "Your cards: ";
            client::print_list(hand);
            std::cout << std::endl;
        }
        if (line.starts_with("tricks")) {
            std::cout << "Taken tricks: ";
            for (auto &trick: taken) {
                client::print_list(trick);
                std::cout << std::endl;
            }
            std::cout << std::endl;
        }
        return kierki::Card{};
    }
} // client