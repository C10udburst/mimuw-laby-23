#include <iostream>
#include <syncstream>
#include "connection.h"
#include "utils.h"

utils::Connection::Connection(int fd, std::string my_addr, std::string peer_addr) {
    this->fd = fd;
    this->my_addr = std::move(my_addr);
    this->peer_addr = std::move(peer_addr);
}

utils::Connection::~Connection() {
    close(fd);
}

std::string utils::Connection::readline() const {
    auto line = utils::readline(fd);
    if (should_log)
        std::osyncstream(std::cout) << "[" << peer_addr << "," << my_addr << "," << now2str() << "] " << line;
    return line;
}

void utils::Connection::writeline(const std::string &line) const {
    if (should_log)
        std::osyncstream(std::cout) << "[" << my_addr << "," << peer_addr << "," << now2str() << "] " << line;
    utils::writeline(fd, line);
}

