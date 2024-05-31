#include <iostream>
#include <sstream>
#include "server.h"
#include "const.h"
#include "../common/errors.h"
#include "../common/utils.h"

#define TOTAL_SCORE 0
#define GAME_SCORE 1

void server::Server::handle_new(utils::Connection *conn) {
    try {
        auto line = conn->readline();
        if (line.rfind("IAM", 0) != 0 || line.size() != (3+1+2)) {
            delete conn;
            return;
        }

        auto seat = seat2int.find(line[3]);
        if (seat == std::string::npos) {
            delete conn;
            return;
        }

        if (!clients[seat].offer(conn)) {
            // send busy
            std::stringstream ss;
            ss << "BUSY";
            for (int i = 0; i < 4; i++) {
                if (clients[i].taken())
                    ss << seat2int[i];
            }
            ss << "\r\n";
            conn->writeline(ss.str());
            delete conn;
            return;
        }
        try {
            handle_game(clients[seat]);
        }
        catch (const errors::ConnClosedError &e) { clients[seat].release(); }
        catch (const errors::TimeoutError &e) { clients[seat].release(); }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

inline std::string make_deal(const server::Game *game, int seat);
inline std::string make_trick(const server::Game *game, int draw);
inline std::string make_score(const server::Server &server, const std::string& type_name, int type);
inline std::string make_wrong(int draw);

void server::Server::handle_game(server::Client &client) {
    for (int game_id = sync.current_game ;; game_id++) {
        if (!client.dirty) {
            sync.barrier(client);
        }
        auto game = gamefile.get_game(game_id);
        if (game->rule == kierki::Rule::NIL)
            break;

        try {
            client.connection->writeline(make_deal(game, client.seat));
            if (client.dirty) {
                for (auto &msg: taken)
                    client.connection->writeline(msg);
            }
        } catch (const errors::ConnClosedError &e) {
            client.dirty = true;
            throw e;
        } catch (const errors::TimeoutError &e) {
            client.dirty = true;
            throw e;
        }
        client.dirty = true;

        // TODO: draw resets after reconnect
        // TODO: first seat is wrong, cuz player should be calculated each draw, but that breaks shit
        auto player = (4 + client.seat - game->first_seat) % 4;
        for (int draw = sync.current_draw; draw < 13; draw++) {
            client.current_draw = draw;
            if (player >= sync.current_player) {
                if (player > sync.current_player) {
                    sync.sem_sleep(client);
                    sync.current_player = player;
                }
                else if (player == 0)
                    game->reset_table();

                do {
                    client.connection->writeline(make_trick(game, draw));
                    kierki::Card played_card{};
                    try {
                        auto ss = utils::parseline(client.connection->readline(), "TRICK" + std::to_string(draw));
                        ss >> played_card;
                    } catch (const errors::TimeoutError &e) { continue; }
                    if (!game->play_card(played_card, client.seat)) {
                        client.connection->writeline(make_wrong(draw));
                        continue;
                    }
                    break;
                } while (true);

                if (player < 3) {
                    auto next_client = ((player + 1) + game->first_seat) % 4;
                    sync.current_player = player + 1;
                    sync.sem_wake(next_client);
                } else  {
                    sync.current_player = 0;
                }

                sync.barrier(client);
                int loser = game->get_loser();
                if (loser == client.seat) {
                    auto score = game->calculate_score(draw);
                    client.scores[GAME_SCORE] += score;
                    client.scores[TOTAL_SCORE] += score;
                    game->first_seat = client.seat;
                }
                if (player == 0) {
                    std::stringstream ss;
                    ss << "TAKEN";
                    ss << draw;
                    for (auto &card: game->table)
                        ss << card;
                    ss << seat2int[loser] << "\r\n";
                    taken.push_back(ss.str());
                } else {
                    sync.sem_sleep(client);
                }
                auto conn_broken = false;
                try {
                    client.connection->writeline(taken.back());
                }
                catch (errors::ConnClosedError& e) { conn_broken = true; }
                catch (errors::TimeoutError& e) { conn_broken = true; }
                if (player < 3) {
                    auto next_client = ((player + 1) + game->first_seat) % 4;
                    sync.sem_wake(next_client);
                }
                if (conn_broken) {
                    client.release();
                    return;
                }

                sync.barrier(client);
            }
        }

        client.dirty = false;
        sync.current_draw = 0;
        sync.current_game = game_id + 1;
        client.connection->writeline(make_score(*this, "SCORE", GAME_SCORE));
        client.connection->writeline(make_score(*this, "TOTAL", TOTAL_SCORE));
        if (client.seat == 0)
            taken.clear();
    }
    sync.game_running = false;
}


inline std::string make_deal(const server::Game *game, int seat) {
    std::string msg = "DEAL00";
    msg[4] += game->rule;
    msg[5] = seat2int[game->first_seat];
    std::stringstream ss;
    for (auto &card: game->cards_initial[seat])
        ss << card;
    ss << "\r\n";
    return msg + ss.str();
}

inline std::string make_trick(const server::Game *game, int draw) {
    std::stringstream ss;
    ss << draw;
    for (auto &card: game->table)
        ss << card;
    ss << "\r\n";
    return "TRICK" + ss.str();
}

inline std::string make_wrong(int draw) {
    return "WRONG" + std::to_string(draw) + "\r\n";
}

inline std::string make_score(const server::Server &server, const std::string& type_name, int type) {
    std::stringstream ss;
    ss << type_name;
    for (int i = 0; i < 4; i++) {
        ss << seat2int[i] << server.clients[i].scores[type];
    }
    ss << "\r\n";
    return ss.str();
}