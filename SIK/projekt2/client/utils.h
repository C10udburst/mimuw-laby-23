#ifndef PROJEKT2_CLIENT_UTILS_H
#define PROJEKT2_CLIENT_UTILS_H

#include <string>
#include <iostream>
#include <array>
#include "../common/card.h"

namespace client {
    // Function to get the trick from the line assuming its either 1, 10, 11, or 12
    std::string get_trick1(const std::string &line);
    std::string get_taken1(const std::string &line);

    template<typename T>
    void print_list(T list) {
        for (unsigned int i = 0; i < list.size(); i++) {
            std::cout << list[i];
            if (i + 1 < list.size()) {
                std::cout << ", ";
            }
        }
    }

    // TODO: broken for first digit
    template<size_t N>
    void print_list(std::array<kierki::Card, N> list) {
        for (unsigned int i = 0; i < list.size(); i++) {
            if (list[i].isnull()) continue;
            std::cout << list[i];
            if (i + 1 < list.size() && !list[i + 1].isnull()) {
                std::cout << ", ";
            }
        }
    }
} // client

#endif //PROJEKT2_CLIENT_UTILS_H
