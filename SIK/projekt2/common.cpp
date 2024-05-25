#include <climits>
#include "common.h"
#include <string>
#include <ctime>
#include <sstream>
#include <iostream>
#include <syncstream>
#include <cstring>
#include <semaphore>
#include <csignal>

const std::basic_string rank2str = "23456789TJQKA";


// https://stackoverflow.com/a/36590760
std::string getTime()
{
    timeval curTime;
    gettimeofday(&curTime, nullptr);

    int milli = curTime.tv_usec / 1000;
    char buf[sizeof "2011-10-08T07:07:09.000"];
    char *p = buf + strftime(buf, sizeof buf, "%FT%T", gmtime(&curTime.tv_sec));
    sprintf(p, ".%03d", milli);

    return buf;
}

namespace kierki
{

    int8_t seat2int(char dir)
    {
        switch (dir)
        {
        case 'N':
            return 0;
        case 'E':
            return 1;
        case 'S':
            return 2;
        case 'W':
            return 3;
        default:
            return -1;
        }
    }

    std::string Connection::log_header(bool them) const
    {
        std::stringstream ss;
        ss << "[";
        if (them) {
            ss << their_addr.to_string();
            ss << ",";
            ss << my_addr.to_string();
        }
        else {
            ss << my_addr.to_string();
            ss << ",";
            ss << their_addr.to_string();
        }
        ss << "," << getTime() << "]";
        return ss.str();
    }

    std::string Connection::readline() const
    {
        std::string line = utils::readline(fd);
        if (log_cout)
            std::osyncstream(std::cout) << log_header(true) << " " << line;
        return line;
    }

    void Connection::writeline(const std::string &line) const
    {
        utils::writeline(fd, line);
        if (log_cout)
            std::osyncstream(std::cout) << log_header(false) << " " << line;
    }

    std::string Addr::to_string() const {
        char name[INET6_ADDRSTRLEN];
        char port[10];
        if (getnameinfo(reinterpret_cast<const sockaddr *>(&addr), addr_size, name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV) != 0)
            throw std::runtime_error("Cannot convert addr to string");
        return std::string(name) + ":" + std::string(port);
    }

    void Connection::init(__time_t timeout) const {
        if (timeout > 0) {
            // add timeout
            timeval tv{};
            tv.tv_sec = timeout;
            setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        }

        // make sigpipe a no-op
        signal(SIGPIPE, SIG_IGN);
    }

    void Connection::kill() const {
        close(fd);
    }

    //[[nodiscard]] bool operator>(const Card &other) const;
    // friend std::istream &operator>>(std::istream &is, Card &card);

    bool Card::operator>(const Card &other) const {
        auto my_rank = rank2str.find(rank);
        auto other_rank = rank2str.find(other.rank);
        return my_rank > other_rank;
    }

    std::istream &operator>>(std::istream &is, Card &card) {
        is.read(&card.rank, 1);
        if (card.rank == '1') {
            // if 10, we skip 0
            is.ignore();
            card.rank = 'T';
        }
        is.read(&card.suit, 1);
        return is;
    }

    std::string Card::to_string() const {
        if (suit == nullcard) return "";
        std::string str = "aa";
        str[0] = rank;
        str[1] = suit;
        return str;
    }

    void Card::marknull() {
        rank = nullcard;
        suit = nullcard;
    }
} // namespace kierki

namespace utils {
    uint16_t read_port(const char *port) {
        char *endptr;
        unsigned long p = strtoul(port, &endptr, 10);
        if (*endptr != '\0' || p == 0 || p > USHRT_MAX) {
            throw std::invalid_argument("Invalid port number");
        }
        return static_cast<uint16_t>(p);
    }

    std::string readline(int fd) {
        // TODO: use amortization maybe?
        std::string line;
        char c;
        while (true) {
            auto n = read(fd, &c, 1);
            if (n == -1) {
                switch (errno) {
                    case EAGAIN:
                        throw errors::TimeoutError();
                    case ECONNRESET:
                    case ECONNABORTED:
                    case EPIPE:
                        throw errors::ConnClosedError();
                    default:
                        throw std::runtime_error("readline failed: " + std::string(strerror(errno)));

                }
            }
            else if (n == 0)
                break;
            line += c;
            if (c == '\n')
                break;
        }
        return line;
    }

    void writeline(int fd, const std::string &line) {
        auto n = line.size();
        while (n > 0) {
            auto m = write(fd, line.c_str(), n);
            if (m == -1) {
                switch (errno) {
                    case EAGAIN:
                        throw errors::TimeoutError();
                    case ECONNRESET:
                    case ECONNABORTED:
                    case EPIPE:
                        throw errors::ConnClosedError();
                    default:
                        throw std::runtime_error("writeline failed: " + std::string(strerror(errno)));
                }
            }
            n -= m;
        }
    }
}