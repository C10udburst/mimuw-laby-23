#ifndef PROJEKT2_HUMANPLAYER_H
#define PROJEKT2_HUMANPLAYER_H

#include "player.h"

namespace client {

    namespace HumanPlayer {
        kierki::Card parse_cmd(const std::string& line, const client::Player& player);
    };

} // client

#endif //PROJEKT2_HUMANPLAYER_H
