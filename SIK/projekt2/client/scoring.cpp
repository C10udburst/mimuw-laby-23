#include "scoring.h"
#include "../common/utils.h"
#include <sstream>

namespace client {
    void Scoring::from_string(const std::string &str, ScoreType type) {
        auto ss = utils::parseline(str, type == TOTAL ? "TOTAL" : "SCORE");
        for (auto &score: scores)
            ss >> score;
    }
} // client

std::istream& operator>>(std::istream& is, client::ScoreEntry& entry) {
    is.read(&entry.place, 1);
    int score = 0;
    while (is.peek() >= '0' && is.peek() <= '9') {
        char c;
        is.read(&c, 1);
        score = score * 10 + (c - '0');
    }
    entry.score = score;
    return is;
}