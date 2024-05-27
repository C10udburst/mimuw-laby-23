#include <cstdint>
#include <cstdlib>
#include <climits>
#include <csignal>
#include <cstring>
#include <netinet/in.h>
#include <netdb.h>
#include <semaphore>
#include <sstream>
#include "errors.h"
#include "utils.h"

/*
 * Possible biggest lines:
 * SCOREN4294967295E4294967295S4294967295W4294967295\r\n
 * TOTALN4294967295E4294967295S4294967295W4294967295\r\n
 * DEAL1N10C10C10C10C10C10C10C10C10C10C10C10C10C\r\n
 *
 * Possible smallest lines:
 * IAMN\r\n
 * TRICK0N\r\n
 */
const auto max_line_length = 51;
const auto min_line_length = 6;

namespace utils {
    uint16_t read_port(const char *port) {
        char *endptr;
        unsigned long p = strtoul(port, &endptr, 10);
        if (*endptr != '\0' || p == 0 || p > USHRT_MAX) {
            throw errors::MainError("Invalid port number");
        }
        return static_cast<uint16_t>(p);
    }

    std::string readline(int fd) {
        char buf[max_line_length + 1];
        auto n = read(fd, buf, min_line_length - 1);
        if (n == -1) {
            switch (errno) {
                case EAGAIN:
                    throw errors::TimeoutError();
                case ECONNRESET:
                case ECONNABORTED:
                case EPIPE:
                    throw errors::ConnClosedError();
                default:
                    throw errors::MainError("readline failed: " + std::string(strerror(errno)));
            }
        }
        char c;
        while (true) {
            auto r = read(fd, &c, 1);
            if (r == -1) {
                switch (errno) {
                    case EINTR:
                    case EAGAIN:
                        throw errors::TimeoutError();
                    case ECONNRESET:
                    case ECONNABORTED:
                    case EPIPE:
                        throw errors::ConnClosedError();
                    default:
                        throw errors::MainError("readline failed: " + std::string(strerror(errno)));
                }
            }
            buf[n++] = c;
            if (c == '\n')
                break;
            if (r == 0) throw errors::ConnClosedError();
            if (n >= max_line_length) throw errors::LineTooLongError();
        }
        buf[n] = '\0';
        return std::string(buf);
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

    std::string addr2str(const struct sockaddr *addr, socklen_t addrlen) {
        char name[INET6_ADDRSTRLEN];
        char port[10];
        if (getnameinfo(addr, addrlen, name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV) != 0) {
            throw errors::MainError("getnameinfo failed: " + std::string(strerror(errno)));
        }
        return std::string(name) + ":" + std::string(port);
    }

    std::string now2str() {
        timeval curTime{};
        gettimeofday(&curTime, nullptr);

        int milli = curTime.tv_usec / 1000;
        char buf[sizeof "2011-10-08T07:07:09.000"];
        char *p = buf + strftime(buf, sizeof buf, "%FT%T", gmtime(&curTime.tv_sec));
        sprintf(p, ".%03d", milli);

        return buf;
    }
}


std::stringstream utils::parseline(const std::string &line, const std::string &type) {
    if (line.find(type, 0) != 0)
        return {};
    if (line.find("\r\n", type.size()) != line.size() - 2)
        return {};
    return std::stringstream(line.substr(type.size(), line.size() - 2));
}