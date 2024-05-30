#include "utils.h"
#include <regex>

namespace client {
    std::string get_trick1(const std::string &line) {
        std::regex re("^(TRICK1[012]?)(?:(?:[23456789JQKA]|10)[CSDH]){0,3}\\r\\n$");
        std::smatch match;
        if (std::regex_search(line, match, re)) {
            std::string deal = match[1];
            return deal.substr(sizeof "TRICK" - 1);
        }
        throw std::runtime_error("Could not find trick in line: " + line);
    }
    std::string get_taken1(const std::string &line) {
        std::regex re("^(TAKEN1[012]?)(?:(?:[23456789JQKA]|10)[CSDH]){4}[NESW]\\r\\n$");
        std::smatch match;
        if (std::regex_search(line, match, re)) {
            std::string deal = match[1];
            return deal.substr(sizeof "TAKEN" - 1);
        }
        throw std::runtime_error("Could not find trick in line: " + line);
    }
} // client