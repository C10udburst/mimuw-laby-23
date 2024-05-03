#include <random>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <ranges>
#include <iostream>
#include "client.h"

constexpr ssize_t UDP_DATA_SIZE = 512;
constexpr ssize_t TCP_DATA_SIZE = 512;

static_assert((UDP_DATA_SIZE + sizeof(ppcb::types::DataPacket) <= ppcb::constants::max_packet_size) || !ppcb::constants::debug,
              "UDP_DATA_SIZE too big");
static_assert((TCP_DATA_SIZE + sizeof(ppcb::types::DataPacket) <= ppcb::constants::max_packet_size) || !ppcb::constants::debug,
                "TCP_DATA_SIZE too big");

namespace client {

    int tcpClient(struct sockaddr_in addr, std::vector<char> &data) {
        utils::open_pcap(false);
        utils::Cleaner cleaner;
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            std::cerr << "ERROR: Could not open socket: " << strerror(errno) << "\n";
            return 1;
        }
        cleaner.add_fd(sockfd);

        ppcb::conf_sock(sockfd);

        if (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
            std::cerr << "ERROR: Could not connect to server: " << strerror(errno) << "\n";
            return 1;
        }

        network::Connection conn{
                .type = ppcb::types::ConnType::TCP,
                .length = data.size(),
                .fd = sockfd,
                .session_id = gen_session_id(),
                .peer_addr = (struct sockaddr *) &addr,
                .peer_addr_len = sizeof(addr),
                .is_server = false
        };

        char buffer[ppcb::constants::max_packet_size + 8];
        try {
            sendData(conn, data, buffer);
        } catch (const std::exception &e) {
            std::cerr << "ERROR: " << e.what() << std::endl;
            return 1;
        }

        return 0;
    }

    int udpClient(struct sockaddr_in addr, std::vector<char> &data, bool retransmit) {
        utils::open_pcap(false);
        utils::Cleaner cleaner;
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            std::cerr << "ERROR: Could not open socket: " << strerror(errno) << "\n";
            return 1;
        }
        cleaner.add_fd(sockfd);

        ppcb::conf_sock(sockfd);

        network::Connection conn{
                .type = retransmit ? ppcb::types::ConnType::UDPR : ppcb::types::ConnType::UDP,
                .length = data.size(),
                .fd = sockfd,
                .session_id = gen_session_id(),
                .peer_addr = (struct sockaddr *) &addr,
                .peer_addr_len = sizeof(addr),
                .is_server = false
        };

        char buffer[ppcb::constants::max_packet_size + 8];
        try {
            sendData(conn, data, buffer);
        } catch (const std::exception &e) {
            std::cerr << "ERROR: " << e.what() << std::endl;
            return 1;
        }

        return 0;
    }

    uint64_t gen_session_id() {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dis;
        return dis(gen);
    }

    void sendData(const network::Connection &conn, const std::vector<char> &data, void *buf) {
        { // send CONN
            auto connPacket = ppcb::types::ConnPacket{
                    .type = ppcb::types::PacketType::CONN,
                    .session_id = conn.session_id,
                    .conn_type = conn.type,
                    .length = data.size()
            };
            ppcb::packet::hton_packet(reinterpret_cast<ppcb::types::Header *>(&connPacket));
            network::writen(conn, &connPacket, sizeof(connPacket));
        }

        do { // receive CONNACC or CONRJT
            auto header = network::read_packet(conn, buf);
            ppcb::packet::ntoh_packet(header);
            if (header->session_id != conn.session_id)
                continue;
            if (header->type == ppcb::types::PacketType::CONRJT)
                throw std::runtime_error("Connection rejected");
            network::expect_packet(conn, ppcb::types::PacketType::CONNACC, header);
            break;
        } while (true);

        // send DATA
        auto chunk_size = conn.type == ppcb::types::ConnType::TCP ? TCP_DATA_SIZE : UDP_DATA_SIZE;
        uint64_t packet_id = 0;
        char writebuf[ppcb::constants::max_packet_size + 8];
        for (auto const chunk: data | std::views::chunk(chunk_size)) {
            auto dataPacket = reinterpret_cast<ppcb::types::DataPacket *>(writebuf);
            auto packet = reinterpret_cast<ppcb::types::Header *>(dataPacket);
            dataPacket->type = ppcb::types::PacketType::DATA;
            dataPacket->session_id = conn.session_id;
            dataPacket->packet_id = packet_id;
            dataPacket->data_length = chunk.size();

            std::copy(chunk.begin(), chunk.end(), writebuf + sizeof(ppcb::types::DataPacket));
            ppcb::packet::hton_packet(packet);

            if (conn.type == ppcb::types::ConnType::UDPR) {
                udprSend(conn, dataPacket, buf);
            } else {
                network::writen(conn, dataPacket,
                                ppcb::packet::sizeof_packetn(packet));
            }

            packet_id++;
        }

        { // receive RCVD or RJT
            auto header = network::read_packet(conn, buf);
            ppcb::packet::ntoh_packet(header);

            if (header->session_id != conn.session_id) {
                throw std::runtime_error("Invalid session id");
            }

            if (header->type == ppcb::types::PacketType::RJT) {
                throw std::runtime_error("Connection rejected");
            }

            network::expect_packet(conn, ppcb::types::PacketType::RCVD, header);
        }
    }

    void udprSend(const network::Connection &conn, const ppcb::types::DataPacket *dataPacket, void *buf) {
        ssize_t packet_len;
        uint64_t packet_id = dataPacket->packet_id;
        auto packet = reinterpret_cast<const ppcb::types::Header *>(dataPacket);
        for (int i = 0; i < ppcb::constants::max_retransmits; i++) {
            try {
                packet_len = writen(conn, dataPacket, ppcb::packet::sizeof_packetn(packet));
            } catch (const network::exceptions::TimeoutException &e) {
                continue; // if timeout, resend
            }
            if (packet_len == 0)
                throw std::runtime_error("Connection closed");
            try {
                auto ack = read_packet(conn, buf);
                if (ack->session_id != packet->session_id)
                    continue;
                if (ack->type != ppcb::types::PacketType::RJT && ack->type != ppcb::types::PacketType::ACC)
                    throw std::runtime_error("Invalid packet type");
                auto rjtacc = reinterpret_cast<ppcb::types::AccRjtPacket *>(ack);
                if (rjtacc->packet_id < packet_id)
                    continue; // ignore old acks
                if (rjtacc->packet_id > packet_id)
                    throw std::runtime_error("Invalid packet id");
                if (rjtacc->type == ppcb::types::PacketType::RJT)
                    throw std::runtime_error("Connection rejected");
                return; // success, ack->type == ACC && ack->packet_id == packet_id
            } catch (const network::exceptions::TimeoutException &e) {
                continue; // if timeout, resend
            }
        }
        throw std::runtime_error("Max retransmits reached");
    }

    struct sockaddr_in get_server_address(char const *host, uint16_t port) {
        struct addrinfo hints;
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET; // IPv4
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

        struct addrinfo *address_result;
        int errcode = getaddrinfo(host, nullptr, &hints, &address_result);
        if (errcode != 0) {
            throw std::runtime_error("getaddrinfo: " + std::string(gai_strerror(errcode)));
        }

        struct sockaddr_in send_address;
        send_address.sin_family = AF_INET;   // IPv4
        send_address.sin_addr.s_addr =       // IP address
                ((struct sockaddr_in *) (address_result->ai_addr))->sin_addr.s_addr;
        send_address.sin_port = htons(port); // port from the command line

        freeaddrinfo(address_result);

        return send_address;
    }
} // namespace client