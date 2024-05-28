#include "utils.h"
#include <regex>

namespace client {
    std::string get_trick1(const std::string &line, const std::string &packet_type) {
        std::regex re("("+packet_type+"1[012]?)(?:(?:[23456789JQKA]|10)[CSDH]){0,3}");
        std::smatch match;
        if (std::regex_search(line, match, re)) {
            std::string deal = match[1];
            return deal.substr(packet_type.size());
        }
        throw std::runtime_error("Could not find trick in line.");
    }
} // client