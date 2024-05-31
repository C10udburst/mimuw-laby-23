#ifndef PROJEKT2_SCORING_H
#define PROJEKT2_SCORING_H

#include <string>
#include <array>

namespace client {

    struct ScoreEntry {
        char place;
        int score;
    };

    enum ScoreType {
        SCORE,
        TOTAL
    };

    struct Scoring {
        std::array<ScoreEntry, 4> scores{};
        public:
            void from_string(const std::string &str, ScoreType type);
    };
} // client

std::istream& operator>>(std::istream& is, client::ScoreEntry& entry);

#endif //PROJEKT2_SCORING_H
