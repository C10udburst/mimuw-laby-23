#include <array>
#include "iplayer.h"

std::string client::IPlayer::trick(std::array<kierki::Card, 4> table) {
    return "TRICK" + std::to_string(trick_id) + play(table).to_string() + "\r\n";
}
