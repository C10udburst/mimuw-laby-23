//
// Created by cloudburst on 31.03.24.
//

#ifndef PROJEKT1_COMMON_H
#define PROJEKT1_COMMON_H

#include <cstdint>
#include <cstdio>
#include "protconst.h"

uint16_t read_port(char const *string);
ssize_t readn(int fd, void *buf, size_t count);
ssize_t writen(int fd, const void *buf, size_t count);
void set_sock_timeout(int fd);

#if __BIG_ENDIAN__
# define htonll(x) (x)
# define ntohll(x) (x)
#else
#if defined(__linux__)
#  include <endian.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#  include <sys/endian.h>
#elif defined(__OpenBSD__)
#  include <sys/types.h>
#  define be16toh(x) betoh16(x)
#  define be32toh(x) betoh32(x)
#  define be64toh(x) betoh64(x)
#endif
# define htonll(x) htobe64(x)
# define ntohll(x) be64toh(x)
#endif

#endif //PROJEKT1_COMMON_H
