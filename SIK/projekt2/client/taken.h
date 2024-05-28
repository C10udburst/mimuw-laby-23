#ifndef PROJEKT2_TAKEN_H
#define PROJEKT2_TAKEN_H

#include "../common/card.h"
#include <array>

namespace client {
    struct Taken {
        int draw;
        std::array<kierki::Card, 4> table;
        char loser;
    public:
        void from_string(const std::string &str);
    };
}

#endif //PROJEKT2_TAKEN_H
