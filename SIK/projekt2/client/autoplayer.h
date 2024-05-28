#ifndef PROJEKT2_AUTOPLAYER_H
#define PROJEKT2_AUTOPLAYER_H

#include "iplayer.h"

namespace client {

    class AutoPlayer : IPlayer {
        kierki::Card play(std::array<kierki::Card, 4> table) override;
    };

} // client

#endif //PROJEKT2_AUTOPLAYER_H
