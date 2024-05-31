#ifndef PROJEKT2_AUTOPLAYER_H
#define PROJEKT2_AUTOPLAYER_H

#include "player.h"
#include <array>

namespace client {

    namespace AutoPlayer {
        kierki::Card play(std::array<kierki::Card, 4> table, client::Player& player);
    };

} // client

#endif //PROJEKT2_AUTOPLAYER_H
