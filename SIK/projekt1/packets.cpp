#include "packets.h"
#include "common.h"
#include <stdexcept>
#include <iostream>
#include <netinet/in.h>

ssize_t PPCB::Packet::size() const {
    if (type == PacketType::DATA)
        return static_cast<ssize_t>(sizeof(PPCB::Packet) + ntohl(data.data_length));
    return sizeof(PPCB::Packet);
}

void PPCB::Packet::validate(ssize_t len) const {
    if (len < static_cast<ssize_t>(sizeof(PacketType) + sizeof(session_id))) {
        throw std::runtime_error("Packet too short");
    }

    if (type < PacketType::CONN || type > PacketType::RCVD) {
        throw std::runtime_error("Invalid packet type");
    }
    if (type == PacketType::DATA) {
        if (len < static_cast<ssize_t>(sizeof(PacketType) + sizeof(session_id) + sizeof(data.packet_id) +
                                       sizeof(data.data_length))) {
            throw std::runtime_error("DATA packet too short");
        }
        if (ntohl(data.data_length) > MAX_PACKET_SIZE - sizeof(PacketType) - sizeof(session_id) - sizeof(data.packet_id) -
                               sizeof(data.data_length)) {
            throw std::runtime_error("DATA packet too long (data_length too big?)");
        }
        if (len < static_cast<ssize_t>(sizeof(PacketType) + sizeof(session_id) + sizeof(data.packet_id) +
                                       sizeof(data.data_length) + ntohl(data.data_length))) {
            throw std::runtime_error("DATA packet too short (data too short?)");
        }
    }

    if (size() != len) {
        throw std::runtime_error("Invalid packet size");
    }

    if (type == PacketType::CONN) {
        if (conn.conn_type < ConnType::TCP || conn.conn_type > ConnType::UDPR) {
            throw std::runtime_error("Invalid connection type");
        }
    }
}

void PPCB::Packet::print() const {
    if (IS_DEBUG) {
        std::cout << "Packet{";

        switch (type) {
            case PPCB::PacketType::CONN:
                std::cout << "CONN"; break;
            case PPCB::PacketType::CONNACC:
                std::cout << "CONNACC"; break;
            case PPCB::PacketType::CONRJT:
                std::cout << "CONRJT"; break;
            case PPCB::PacketType::DATA:
                std::cout << "DATA"; break;
            case PPCB::PacketType::ACC:
                std::cout << "ACC"; break;
            case PPCB::PacketType::RJT:
                std::cout << "RJT"; break;
            case PPCB::PacketType::RCVD:
                std::cout << "RCVD"; break;
            default:
                std::cout << "UNKNOWN:" << static_cast<int>(type); break;
        }

        std::cout << ", session_id=" << std::hex << session_id << std::dec;
        if (type == PacketType::CONN) {
            std::cout << ", conn_type=";
            switch (conn.conn_type) {
                case ConnType::TCP:
                    std::cout << "TCP"; break;
                case ConnType::UDP:
                    std::cout << "UDP"; break;
                case ConnType::UDPR:
                    std::cout << "UDPR"; break;
                default:
                    std::cout << "UNKNOWN:" << static_cast<int>(conn.conn_type);
            }
            std::cout << ", length=" << ntohll(conn.length);
        } else if (type == PacketType::DATA) {
            std::cout << ", packet_id=" << ntohll(data.packet_id) << ", data_length=" << ntohl(data.data_length) << ", data=\"";
            for (uint32_t i = 0; i < ntohl(data.data_length) && i < 10; i++) {
                if (data.data[i] == '\n')
                    std::cout << "\\n";
                else if (data.data[i] == '\r')
                    std::cout << "\\r";
                else
                    std::cout << data.data[i];
            }
            if (ntohl(data.data_length) > 10) {
                std::cout << "...";
            }
            std::cout << "\"";
        } else if (type == PacketType::ACC) {
            std::cout << ", packet_id=" << ntohll(acc.packet_id);
        } else if (type == PacketType::RJT) {
            std::cout << ", packet_id=" << ntohll(rjt.packet_id);
        }
        std::cout << "}\n";
    } else {
        if (type != PacketType::DATA) return;
        printf("%.*s", ntohl(data.data_length), data.data);
    }
}

