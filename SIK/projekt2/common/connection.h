#ifndef PROJEKT2_CONNECTION_H
#define PROJEKT2_CONNECTION_H

#include <string>

namespace utils {
    struct Connection {
        int fd;
        bool should_log = false;
        std::string my_addr;
        std::string peer_addr;

    public:
        Connection(int fd, std::string my_addr, std::string peer_addr);
        ~Connection();

        void writeline(const std::string &line) const;
        [[nodiscard]] std::string readline() const;
    };
}

#endif //PROJEKT2_CONNECTION_H
