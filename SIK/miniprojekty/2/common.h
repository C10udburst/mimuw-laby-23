#ifndef MIM_COMMON_H
#define MIM_COMMON_H

#include <limits.h>

uint16_t read_port(char const *string);
size_t   read_size(char const *string);
uint64_t read_uint64(char const *string);

struct sockaddr_in get_server_address(char const *host, uint16_t port);

#endif
