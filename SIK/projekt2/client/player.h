#ifndef PROJEKT2_PLAYER_H
#define PROJEKT2_PLAYER_H

#include "../common/card.h"
#include <vector>
#include <array>

namespace client {

    class Player {
    public:
        std::array<kierki::Card, 13> hand{};
        std::vector<std::array<kierki::Card, 4>> taken;
        int trick_id = 0;
    };

} // client

#endif //PROJEKT2_PLAYER_H
