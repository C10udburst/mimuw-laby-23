#ifndef PROJEKT1_PACKETS_H
#define PROJEKT1_PACKETS_H

#include <cstdint>
#include <cstdlib>
#include <string>

namespace PPCB {

    const bool IS_DEBUG = true;

    enum class PacketType : char {
        CONN = 1,
        CONNACC = 2,
        CONRJT = 3,
        DATA = 4,
        ACC = 5,
        RJT = 6,
        RCVD = 7
    };
    static_assert(sizeof(PacketType) == 1 || !IS_DEBUG, "PacketType must be 1 byte long");

    enum class ConnType: char {
        TCP = 1,
        UDP = 2,
        UDPR = 3
    };
    static_assert(sizeof(ConnType) == 1 || !IS_DEBUG, "ConnType must be 1 byte long");

#pragma pack(push, 1)
    struct Packet {
        PacketType type;
        uint64_t session_id;
        union {
            struct {
                ConnType conn_type;
                uint64_t length;
            } conn;
            struct {
                uint64_t packet_id;
                uint32_t data_length;
                char data[0];
            } data;
            struct {
                uint64_t packet_id;
            } acc;
            struct {
                uint64_t packet_id;
            } rjt;
        };

        void validate(ssize_t len) const;
        [[nodiscard]] ssize_t size() const;
        void print() const;
    };
#pragma pack(pop)

    const ssize_t MAX_PACKET_SIZE = 64000;

    static_assert(sizeof(Packet) <= MAX_PACKET_SIZE || !IS_DEBUG, "Packet header must be smaller than MAX_PACKET_SIZE");
    static_assert(MAX_PACKET_SIZE <= ((1<<16) - 8 - 20) || !IS_DEBUG, "MAX_PACKET_SIZE must fit into UDP packet");
}


#endif //PROJEKT1_PACKETS_H
