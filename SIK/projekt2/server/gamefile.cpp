#include "gamefile.h"
#include "const.h"
#include <iostream>
#include <cstring>

std::istream &operator>>(std::istream &is, server::Game &game) {
    is >> game.rule;
    if (game.rule == kierki::Rule::NIL)
        return is;
    char first_seat;
    is.read(&first_seat, 1);
    auto seat = seat2int.find(first_seat);
    if (seat == std::string::npos) {
        game.rule = kierki::Rule::NIL;
        return is;
    }
    game.first_seat = static_cast<int8_t>(seat);
    is.ignore(1); // ignore \n
    for (auto & player : game.cards) {
        for (auto & card : player)
            is >> card;
        is.ignore(1); // ignore \n
    }
    memcpy(game.cards_initial, game.cards, sizeof(game.cards));
    return is;
}


server::Game* server::Gamefile::get_game(int game_number) {
    std::lock_guard<std::mutex> lock(mutex);
    if (game_number <= current_game)
        return &loaded_game;
    if (!file.is_open() || file.eof()) {
        current_game = game_number;
        loaded_game.rule = kierki::Rule::NIL;
        return &loaded_game;
    }
    file >> loaded_game;
    current_game = game_number;
    loaded_game.reset_table();
    return &loaded_game;
}

server::Gamefile::~Gamefile() {
    if (file.is_open())
        file.close();
}

server::Gamefile::Gamefile(const std::string &filename) {
    file = std::ifstream(filename);
    if (!file.is_open())
        throw std::runtime_error("Cannot open file " + filename);
}

bool server::Game::play_card(kierki::Card card, int seat) {
    if (card.isnull())
        return false;
    for (auto &c: cards[seat]) {
        if (c == card) {
            if (table[0].isnull()) {
                table[0] = card;
                c.marknull();
                return true;
            } else {
                auto player = (4 + seat - first_seat) % 4;
                if (card.suit == table[0].suit) {
                    table[player] = card;
                    c.marknull();
                    return true;
                }
                for (auto &c2: cards[seat]) {
                    if (c2.suit == table[0].suit)
                        return false;
                }
                table[player] = card;
                c.marknull();
                return true;

            }
        }
    }
    return false;
}

const auto KING_OF_HEARTS = kierki::Card{kierki::Rank::KING, kierki::Suit::HEARTS};

int server::Game::calculate_score(int deal) {
    int score = 0;
    if (rule == kierki::Rule::ANY || rule == kierki::Rule::ALL)
        score = 1;
    if (rule == kierki::Rule::NO_HEARTS || rule == kierki::Rule::ALL) {
        for (auto &c: table) {
            if (c.suit == kierki::Suit::HEARTS)
                score++;
        }
    }
    if (rule == kierki::Rule::NO_QUEENS || rule == kierki::Rule::ALL) {
        for (auto &c: table) {
            if (c.rank == kierki::Rank::QUEEN)
                score += 5;
        }
    }
    if (rule == kierki::Rule::NO_MEN || rule == kierki::Rule::ALL) {
        for (auto &c: table) {
            if (c.rank == kierki::Rank::JACK || c.rank == kierki::Rank::KING)
                score += 2;
        }
    }
    if (rule == kierki::Rule::NO_KING_OF_HEARTS || rule == kierki::Rule::ALL) {
        for (auto &c: table) {
            if (c == KING_OF_HEARTS)
                score += 18;
        }
    }
    if (rule == kierki::Rule::NO_7_13 || rule == kierki::Rule::ALL) {
        if (deal == 6 || deal == 12)
            score += 10;
    }
    return score;
}

int server::Game::get_loser() {
    int8_t max = 0;
    for (int8_t i = 0; i < 4; i++) {
        if (table[i] > table[max])
            max = i;
    }
    return (max + first_seat) % 4;
}

void server::Game::reset_table() {
    for (auto &c: table)
        c.marknull();
}
