#include "packets.h"
#include <stdexcept>
#include <cstring>

namespace PPCB {

    void *Packet::serialize() {
        void *buffer = malloc(packet_size());
        serialize_inner(buffer);
        return buffer;
    }

    void Packet::serialize_inner(void *buffer) {}

    size_t Packet::packet_size() {
        return sizeof(PacketType) + sizeof(uint64_t);
    }

    // region ConnPacket
    ConnType from_string(const std::string &str) {
        if (str == "tcp") {
            return ConnType::TCP;
        } else if (str == "udp") {
            return ConnType::UDP;
        } else if (str == "udpr") {
            return ConnType::UDPR;
        } else {
            throw std::invalid_argument("Invalid connection type");
        }
    }

    ConnPacket from_string(const std::string &str, uint64_t session_id, uint64_t length) {
        return ConnPacket(PacketType::CONN, session_id, from_string(str), length);
    }

    void ConnPacket::serialize_inner(void *buffer) {
        memcpy(buffer, &type, sizeof(PacketType));
        memcpy(static_cast<char *>(buffer) + sizeof(PacketType), &session_id, sizeof(uint64_t));
        memcpy(static_cast<char *>(buffer) + sizeof(PacketType) + sizeof(uint64_t), &conn_type, sizeof(ConnType));
        uint64_t length = __builtin_bswap64(this->length);
        memcpy(static_cast<char *>(buffer) + sizeof(PacketType) + sizeof(uint64_t) + sizeof(ConnType), &length, sizeof(uint64_t));
    }

    size_t ConnPacket::packet_size() {
        return Packet::packet_size() + sizeof(ConnType) + sizeof(uint64_t);
    }
    // endregion

    // region Deserialization
    ConnPacket* deserializeConnPacket(void *buffer);
    DataPacket* deserializeDataPacket(void *buffer);

    Packet *deserialize(void *buffer) {
        PacketType type = *static_cast<PacketType *>(buffer);
        switch (type) {
            case PacketType::CONNACC:
            case PacketType::CONRJT:
            case PacketType::RCVD:
                // These packets don't have any additional data, so we don't need to do anything special
                uint64_t session_id;
                memcpy(&session_id, static_cast<char *>(buffer) + sizeof(PacketType), sizeof(uint64_t));
                return new Packet(type, session_id);
            case PacketType::CONN:
                return deserializeConnPacket(buffer);
            case PacketType::DATA:
                return deserializeDataPacket(buffer);
            default:
                throw std::invalid_argument("Invalid packet type");
        }
        return nullptr;
    }

    ConnPacket* deserializeConnPacket(void *buffer)  {
        uint64_t session_id;
        memcpy(&session_id, static_cast<char *>(buffer) + sizeof(PacketType), sizeof(uint64_t));
        ConnType conn_type;
        memcpy(&conn_type, static_cast<char *>(buffer) + sizeof(PacketType) + sizeof(uint64_t), sizeof(ConnType));
        uint64_t length;
        memcpy(&length, static_cast<char *>(buffer) + sizeof(PacketType) + sizeof(uint64_t) + sizeof(ConnType), sizeof(uint64_t));
        length = __builtin_bswap64(length);
        return new ConnPacket(PacketType::CONN, session_id, conn_type, length);
    }

    DataPacket* deserializeDataPacket(void *buffer) {
        uint64_t session_id;
        memcpy(&session_id, static_cast<char *>(buffer) + sizeof(PacketType), sizeof(uint64_t));
        uint64_t seq_num;
        memcpy(&seq_num, static_cast<char *>(buffer) + sizeof(PacketType) + sizeof(uint64_t), sizeof(uint64_t));
        uint32_t data_length;
        memcpy(&data_length, static_cast<char *>(buffer) + sizeof(PacketType) + sizeof(uint64_t) + sizeof(uint64_t), sizeof(uint32_t));
        data_length = __builtin_bswap32(data_length);
        char *data = static_cast<char *>(malloc(data_length));
        memcpy(data, static_cast<char *>(buffer) + sizeof(PacketType) + sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint32_t), data_length);
        return new DataPacket(PacketType::DATA, session_id, seq_num, data_length, data);
    }

    // endregion

} // namespace PPCB
