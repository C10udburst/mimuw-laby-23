#include "common.h"
#include <climits>
#include <cerrno>
#include <iostream>
#include <csignal>
#include <cstring>
#include <asm-generic/socket.h>
#include <sys/socket.h>

uint16_t read_port(char const *string) {
    char *endptr;
    unsigned long port = strtoul(string, &endptr, 10);
    if ((port == ULONG_MAX && errno == ERANGE) || *endptr != 0 || port == 0 || port > UINT16_MAX) {
        throw std::runtime_error("Invalid port number");
    }
    return (uint16_t) port;
}

// Following two functions come from Stevens' "UNIX Network Programming" book.

// Read n bytes from a descriptor. Use in place of read() when fd is a stream socket.
ssize_t readn(int fd, void *vptr, size_t n) {
    ssize_t nleft, nread;
    char *ptr;

    ptr = static_cast<char *>(vptr);
    nleft = n;
    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0)
            throw std::runtime_error("read error: " + std::string(strerror(errno)));
        else if (nread == 0)
            break;            // EOF

        nleft -= nread;
        ptr += nread;
    }
    return n - nleft;         // return >= 0
}

// Write n bytes to a descriptor.
ssize_t writen(int fd, const void *vptr, size_t n){
    ssize_t nwritten;
    const char *ptr;

    ptr = static_cast<const char *>(vptr);    // Can't do pointer arithmetic on void*.
    auto nleft = n;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0)
            throw std::runtime_error("write error: " + std::string(strerror(errno)));

        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}

void set_sock_timeout(int fd) {
    struct timeval tv{};
    tv.tv_sec = MAX_WAIT;
    tv.tv_usec = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        throw std::runtime_error("setsockopt error: " + std::string(strerror(errno)));
    }
}