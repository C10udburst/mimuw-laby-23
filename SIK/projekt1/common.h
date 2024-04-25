#ifndef PROJEKT1_COMMON_H
#define PROJEKT1_COMMON_H

#include <cstdint>
#include <cstdio>
#include <string>
#include <unistd.h>
#include <vector>
#include <set>
#include "protconst.h"
#include "exceptions.h"

#ifndef PPCB_DEBUG
# define PPCB_DEBUG 1
#endif

namespace ppcb {
    namespace constants {
        constexpr int max_wait = MAX_WAIT;
        constexpr int max_retransmits = MAX_RETRANSMITS;
        constexpr ssize_t max_packet_size = 64000;
        constexpr int queue_size = 5;
        constexpr bool debug = PPCB_DEBUG;
    } // namespace ppcb::constants

    namespace types {
        // PPCB protocol types.
        // TODO: task does not specify whether to use unsigned or signed integers.

        enum class PacketType : uint8_t {
            CONN = 1,
            CONNACC = 2,
            CONRJT = 3,
            DATA = 4,
            ACC = 5,
            RJT = 6,
            RCVD = 7
        };
        static_assert(sizeof(PacketType) == 1 || !constants::debug, "PacketType must be 1 byte long");

        enum class ConnType : uint8_t {
            TCP = 1,
            UDP = 2,
            UDPR = 3
        };
        static_assert(sizeof(ConnType) == 1 || !constants::debug, "ConnType must be 1 byte long");

#pragma pack(push, 1)
        struct Header {
            PacketType type;
            uint64_t session_id;
        };
#pragma pack(pop)

#pragma pack(push, 1)
        struct ConnPacket {
            PacketType type;
            uint64_t session_id;
            ConnType conn_type;
            uint64_t length;
        };
#pragma pack(pop)

#pragma pack(push, 1)
        struct DataPacket {
            PacketType type;
            uint64_t session_id;
            uint64_t packet_id;
            uint32_t data_length;
        };
#pragma pack(pop)

#pragma pack(push, 1)
        struct AccRjtPacket {
            PacketType type;
            uint64_t session_id;
            uint64_t packet_id;
        };
#pragma pack(pop)
    } // namespace ppcb::types

    // Configures tcp/udp socket (sets timeout etc.).
    void conf_sock(int fd);

    void reset_sock(int fd);

    namespace packet {
        // Validates packet, including stuff after header. Assumes header is in network byte order.
        void validate(const types::Header *header, ssize_t read_bytes);

        // Calculates size of the whole packet. Assumes header is in network byte order.
        ssize_t sizeof_packetn(const types::Header *header);

        // Calculates size of the whole packet. Assumes header is in host byte order.
        ssize_t sizeof_packeth(const types::Header *header);

        // prints debug packet info.
        void print_packet(const types::Header *header, char dir);

        // Converts packet to host byte order.
        void ntoh_packet(types::Header *header);

        // Converts packet to network byte order.
        void hton_packet(types::Header *header);
    } // namespace ppcb::packet
} // namespace ppcb

#if __BYTE_ORDER__ == __LITTLE_ENDIAN__
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
#endif

namespace utils {
    // Convert string to port number.
    uint16_t read_port(const char *port);

    // Convert 64-bit integer between host and network byte order.
    uint64_t htonll(uint64_t host);

    uint64_t ntohll(uint64_t network);

    class Cleaner {
    public:
        std::set<int> fds;

        Cleaner() = default;

        ~Cleaner() {
            for (int fd : fds) {
                close(fd);
            }
        }

        void add_fd(int fd) {
            fds.insert(fd);
        }

        void free_fd(int fd) {
            if (fds.find(fd) != fds.end()) {
                fds.erase(fd);
                close(fd);
            }
        }

    };
}

namespace network {
    struct Connection {
        ppcb::types::ConnType type;
        uint64_t length;
        int fd;
        uint64_t session_id;
        struct sockaddr *peer_addr;
        socklen_t peer_addr_len;
        bool is_server;
    };


    // protocol-agnostic read/write functions.
    ppcb::types::Header *read_packet(const Connection &conn, void *buf);

    ppcb::types::Header *
    read_packet_addr(const Connection &conn, void *buf, struct sockaddr *addr, socklen_t *addr_len);


    ssize_t writen(const Connection &conn, const void *vptr, ssize_t n);

    void
    expect_packet(__attribute_maybe_unused__ const Connection &conn, ppcb::types::PacketType expected_type, const ppcb::types::Header *header);
}

#endif //PROJEKT1_COMMON_H