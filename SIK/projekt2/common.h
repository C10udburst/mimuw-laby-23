#ifndef KIERKI_COMMON_H
#define KIERKI_COMMON_H

#include <string>
#include <stdexcept>
#include <cstdint>
#include <cstdio>
#include <string>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>

namespace kierki {

    enum Rule {
        // nie brać lew, za każdą wziętą lewę dostaje się 1 punkt;
        ANY = 1,
        // nie brać kierów, za każdego wziętego kiera dostaje się 1 punkt;
        NO_HEARTS = 2,
        // nie brać dam, za każdą wziętą damę dostaje się 5 punktów;
        NO_QUEENS = 3,
        // nie brać panów (waletów i króli), za każdego wziętego pana dostaje się 2 punkty;
        NO_MEN = 4,
        // nie brać króla kier, za jego wzięcie dostaje się 18 punktów;
        NO_KING_OF_HEARTS = 5,
        // nie brać siódmej i ostatniej lewy, za wzięcie każdej z tych lew dostaje się po 10 punktów;
        NO_LAST_TWO = 6,
        // rozbójnik, punkty dostaje się za wszystko wymienione powyżej.
        ALL = 7
    };

    constexpr auto nullcard = '!';
    constexpr auto int2seat = "NESW";

    struct Card {
        char suit;
        char rank;

        public:
            [[nodiscard]] bool operator>(const Card &other) const;
            friend std::istream &operator>>(std::istream &is, Card &card);
            bool operator==(const Card& other) const {
                if (suit == nullcard) return false;
                return suit == other.suit && rank == other.rank;
            }
            [[nodiscard]] std::string to_string() const;
            void marknull();
    };

    struct Addr {
        sockaddr_in addr;
        socklen_t addr_size;

        public:
            [[nodiscard]] std::string to_string() const;
    };

    struct Connection {
        int fd;
        struct Addr my_addr;
        struct Addr their_addr;
        bool log_cout = false;
    
    public:
        // returns a string with formatted log header
        [[nodiscard]] std::string log_header(bool them) const;

        [[nodiscard]] std::string readline() const;
        void writeline(const std::string &line) const;

        void init(__time_t timeout) const;
        void kill() const;
    };

    int8_t seat2int(char dir);
} // namespace kierki

namespace utils {
    u_int16_t read_port(const char *port);
    std::string readline(int fd);
    void writeline(int fd, const std::string &line);
}

namespace errors {
    class TimeoutError : public std::runtime_error {
        public:
            TimeoutError() : std::runtime_error("Timeout reached") {}
    };

    class ConnClosedError : public std::runtime_error {
        public:
            ConnClosedError() : std::runtime_error("Connection closed") {}
    };

}

#endif // KIERKI_COMMON_H