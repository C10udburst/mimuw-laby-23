#ifndef PROJEKT2_HUMANPLAYER_H
#define PROJEKT2_HUMANPLAYER_H

#include "iplayer.h"

namespace client {

    class HumanPlayer: IPlayer {
        kierki::Card play(std::array<kierki::Card, 4> table) override;
    };

} // client

#endif //PROJEKT2_HUMANPLAYER_H
