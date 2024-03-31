#include "common.h"
#include <climits>
#include <cerrno>
#include <iostream>

uint16_t read_port(char const *string) {
    char *endptr;
    unsigned long port = strtoul(string, &endptr, 10);
    if ((port == ULONG_MAX && errno == ERANGE) || *endptr != 0 || port == 0 || port > UINT16_MAX) {
        throw std::runtime_error("Invalid port number");
    }
    return (uint16_t) port;
}