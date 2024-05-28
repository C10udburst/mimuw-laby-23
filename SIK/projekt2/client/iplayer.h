#ifndef PROJEKT2_IPLAYER_H
#define PROJEKT2_IPLAYER_H

#include "../common/card.h"
#include <vector>

namespace client {

    class IPlayer {
    public:
        std::array<kierki::Card, 13> hand;
        std::vector<std::array<kierki::Card, 4>> taken;
        int trick_id = 0;

        virtual kierki::Card play(std::array<kierki::Card, 4> table) = 0;
        std::string trick(std::array<kierki::Card, 4> table);
    };

} // client

#endif //PROJEKT2_IPLAYER_H
