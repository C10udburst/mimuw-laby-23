#ifndef PROJEKT2_TRICK_H
#define PROJEKT2_TRICK_H

#include "../common/card.h"
#include <array>

namespace client {
    struct Trick {
        int draw;
        std::array<kierki::Card, 4> table;
    public:
        void from_string(const std::string &str);
    };
}

#endif //PROJEKT2_TRICK_H
