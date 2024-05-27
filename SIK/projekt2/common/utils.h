#ifndef PROJEKT2_UTILS_H
#define PROJEKT2_UTILS_H

#include <cstdint>
#include <unistd.h>

namespace utils {
    uint16_t read_port(const char *port);
    std::string addr2str(const struct sockaddr *addr, socklen_t addrlen);
    std::string now2str();

    std::string readline(int fd);
    void writeline(int fd, const std::string &line);
    std::stringstream parseline(const std::string &line, const std::string &type);
}

#endif //PROJEKT2_UTILS_H
