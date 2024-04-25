#include <netdb.h>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <csignal>
#include <ranges>
#include <vector>
#include <random>
#include "packets.h"
#include "common.h"
#include "protconst.h"

class Cleaner {
public:
    int sockfd = -1;

    ~Cleaner() {
        if (sockfd >= 0)
            close(sockfd);
    }
};

const ssize_t UDP_DATA_SIZE = 512;
const ssize_t TCP_DATA_SIZE = 1024;

static_assert(UDP_DATA_SIZE + sizeof(PPCB::Packet) <= PPCB::MAX_PACKET_SIZE || !PPCB::IS_DEBUG, "UDP_DATA_SIZE too big");
static_assert(TCP_DATA_SIZE + sizeof(PPCB::Packet) <= PPCB::MAX_PACKET_SIZE || !PPCB::IS_DEBUG, "TCP_DATA_SIZE too big");

static struct sockaddr_in get_server_address(char const *host, uint16_t port);
namespace TcpClient { int tcpClient(struct sockaddr_in send_address, Cleaner &cleaner); }
namespace UdpClient { int udpClient(struct sockaddr_in send_address, Cleaner &cleaner); }
namespace UdprClient { int udprClient(struct sockaddr_in send_address, Cleaner &cleaner); }

int main(int argc, char *argv[]) {
    Cleaner cleaner;
    if (argc != 4) {
        std::cerr << "ERROR: Invalid number of arguments\n";
        return 1;
    }

    struct sockaddr_in send_address;
    try {
        send_address = get_server_address(argv[2], read_port(argv[3]));
    } catch (std::runtime_error &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }

    try {
        if (strcmp(argv[1], "tcp") == 0) {
            return TcpClient::tcpClient(send_address, cleaner);
        } else if (strcmp(argv[1], "udp") == 0) {
            return UdpClient::udpClient(send_address, cleaner);
        } else if (strcmp(argv[1], "udpr") == 0) {
            return UdprClient::udprClient(send_address, cleaner);
        } else {
            std::cerr << "ERROR: Invalid protocol. Must be either 'tcp' or 'udp' or 'udpr'\n";
            return 1;
        }
    } catch (std::runtime_error &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}

std::vector<char> _read_stdin() {
    // https://stackoverflow.com/questions/32954492/fastest-way-in-c-to-read-contents-of-stdin-into-a-string-or-vector
    std::vector<char> cin_str;
    // 64k buffer seems sufficient
    std::streamsize buffer_sz = 65536;
    std::vector<char> buffer(buffer_sz);
    cin_str.reserve(buffer_sz);

    auto rdbuf = std::cin.rdbuf();
    while (auto cnt_char = rdbuf->sgetn(buffer.data(), buffer_sz))
        cin_str.insert(cin_str.end(), buffer.data(), buffer.data() + cnt_char);

    return cin_str;
}

std::vector<char> read_stdin() {
    std::string fakeinput = "Hello, World! ABABABAB 1234567890\n";
    std::vector<char> data(fakeinput.begin(), fakeinput.end());
    return data;
}

uint64_t gen_session_id() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    return dis(gen);
}

static struct sockaddr_in get_server_address(char const *host, uint16_t port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    struct addrinfo *address_result;
    int errcode = getaddrinfo(host, NULL, &hints, &address_result);
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

namespace UdpClient {
    int udpClient(struct sockaddr_in send_address, Cleaner &cleaner) {
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            std::cerr << "ERROR: Could not create socket: " << strerror(errno) << "\n";
            return 1;
        }
        cleaner.sockfd = sockfd;

        set_sock_timeout(sockfd);

        std::vector<char> data = read_stdin();
        uint64_t session = gen_session_id();
        char buffer[PPCB::MAX_PACKET_SIZE];

        {
            PPCB::Packet packet{
                    .type = PPCB::PacketType::CONN,
                    .session_id = session,
                    .conn = {.conn_type = PPCB::ConnType::UDP, .length = htonll(data.size())}
            };

            ssize_t sent = sendto(sockfd, &packet, packet.size(), 0, (struct sockaddr *) &send_address,
                                  sizeof(send_address));
            if (sent != packet.size()) {
                std::cerr << "ERROR: Could not send CONN packet: " << strerror(errno) << "\n";
                return 1;
            }
        }

        do { // receive CONNACC or CONRJT
            socklen_t len = sizeof(send_address);
            ssize_t recv_len = recvfrom(sockfd, buffer, PPCB::MAX_PACKET_SIZE, 0, (struct sockaddr *) &send_address,
                                        &len);
            if (recv_len < 0) {
                std::cerr << "ERROR: Could not receive CONNACC/CONRJT packet: " << strerror(errno) << "\n";
                return 1;
            }

            auto *packet = reinterpret_cast<PPCB::Packet *>(buffer);
            try {
                packet->validate(recv_len);
            } catch (std::runtime_error &e) {
                std::cerr << "ERROR: Invalid packet received: " << e.what() << "\n";
                return 1;
            }
            packet->print();

            if (packet->session_id != session)
                continue;

            if (packet->type == PPCB::PacketType::CONRJT) {
                std::cerr << "ERROR: Connection refused by server\n";
                return 1;
            }

            if (packet->type != PPCB::PacketType::CONNACC) {
                std::cerr << "ERROR: Unexpected packet type: " << static_cast<int>(packet->type) << "\n";
                return 1;
            }

            break;
        } while (true);

        // send data in DATA packets
        uint64_t packet_id = 0;
        for (auto const chunk: data | std::views::chunk(UDP_DATA_SIZE)) {
            auto packet = reinterpret_cast<PPCB::Packet *>(buffer);

            packet->type = PPCB::PacketType::DATA;
            packet->session_id = session;
            packet->data.packet_id = packet_id++;
            packet->data.data_length = chunk.size();

            packet->data.packet_id = htonll(packet->data.packet_id);
            packet->data.data_length = htonl(packet->data.data_length);

            std::copy(chunk.begin(), chunk.end(), packet->data.data);

            ssize_t sent = sendto(sockfd, packet, packet->size(), 0, (struct sockaddr *) &send_address,
                                  sizeof(send_address));
            if (sent != packet->size()) {
                std::cerr << "ERROR: Could not send DATA packet: " << strerror(errno) << "\n";
                return 1;
            }
        }

        { // receive RCVD or RJT
            socklen_t len = sizeof(send_address);
            ssize_t recv_len = recvfrom(sockfd, buffer, PPCB::MAX_PACKET_SIZE, 0, (struct sockaddr *) &send_address,
                                        &len);
            if (recv_len < 0) {
                std::cerr << "ERROR: Could not receive RCVD packet: " << strerror(errno) << "\n";
                return 1;
            }

            auto *packet = reinterpret_cast<PPCB::Packet *>(buffer);
            try {
                packet->validate(recv_len);
            } catch (std::runtime_error &e) {
                std::cerr << "ERROR: Invalid packet received: " << e.what() << "\n";
                return 1;
            }
            packet->print();

            if (packet->session_id != session) {
                std::cerr << "ERROR: Invalid session ID in RCVD packet\n";
                return 1;
            }

            if (packet->type != PPCB::PacketType::RCVD) {
                std::cerr << "ERROR: Unexpected packet type: " << static_cast<int>(packet->type) << "\n";
                return 1;
            }
        }

        return 0;
    }
}

namespace TcpClient {
    int tcpClient(struct sockaddr_in send_address, Cleaner &cleaner) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            std::cerr << "ERROR: Could not create socket: " << strerror(errno) << "\n";
            return 1;
        }
        cleaner.sockfd = sockfd;

        set_sock_timeout(sockfd);

        if (connect(sockfd, (struct sockaddr *) &send_address, sizeof(send_address)) < 0) {
            std::cerr << "ERROR: Could not connect to server: " << strerror(errno) << "\n";
            return 1;
        }

        std::vector<char> data = read_stdin();
        uint64_t session = gen_session_id();
        char buffer[PPCB::MAX_PACKET_SIZE];

        {
            PPCB::Packet packet{
                    .type = PPCB::PacketType::CONN,
                    .session_id = session,
                    .conn = {.conn_type = PPCB::ConnType::TCP, .length = htonll(data.size())}
            };

            ssize_t sent = send(sockfd, &packet, packet.size(), 0);
            if (sent != packet.size()) {
                std::cerr << "ERROR: Could not send CONN packet: " << strerror(errno) << "\n";
                return 1;
            }
        }

        do { // receive CONNACC or CONRJT
            ssize_t recv_len = readn(sockfd, buffer, PPCB::MAX_PACKET_SIZE);

            auto *packet = reinterpret_cast<PPCB::Packet *>(buffer);
            try {
                packet->validate(recv_len);
            } catch (std::runtime_error &e) {
                std::cerr << "ERROR: Invalid packet received: " << e.what() << "\n";
                return 1;
            }
            packet->print();

            if (packet->session_id != session)
                continue;

            if (packet->type == PPCB::PacketType::CONRJT) {
                std::cerr << "ERROR: Connection refused by server\n";
                return 1;
            }

            if (packet->type != PPCB::PacketType::CONNACC) {
                std::cerr << "ERROR: Unexpected packet type: " << static_cast<int>(packet->type) << "\n";
                return 1;
            }

            break;
        } while (true);

        // send data in DATA packets
        uint64_t packet_id = 0;
        for (auto const chunk: data | std::views::chunk(TCP_DATA_SIZE)) {
            auto packet = reinterpret_cast<PPCB::Packet *>(buffer);

            packet->type = PPCB::PacketType::DATA;
            packet->session_id = session;
            packet->data.packet_id = packet_id++;
            packet->data.data_length = chunk.size();

            packet->data.packet_id = htonll(packet->data.packet_id);
            packet->data.data_length = htonl(packet->data.data_length);

            std::copy(chunk.begin(), chunk.end(), packet->data.data);

            if (writen(sockfd, packet, packet->size()) != packet->size()) {
                std::cerr << "ERROR: Could not send DATA packet: " << strerror(errno) << "\n";
                return 1;
            }
        }

        { // receive RCVD or RJT
            ssize_t recv_len = readn(sockfd, buffer, PPCB::MAX_PACKET_SIZE);

            auto *packet = reinterpret_cast<PPCB::Packet *>(buffer);
            try {
                packet->validate(recv_len);
            } catch (std::runtime_error &e) {
                std::cerr << "ERROR: Invalid packet received: " << e.what() << "\n";
                return 1;
            }
            packet->print();

            if (packet->session_id != session) {
                std::cerr << "ERROR: Invalid session ID in RCVD packet\n";
                return 1;
            }

            if (packet->type == PPCB::PacketType::RJT) {
                std::cerr << "ERROR: Server rejected packet\n";
                return 1;
            }

            if (packet->type != PPCB::PacketType::RCVD) {
                std::cerr << "ERROR: Unexpected packet type: " << static_cast<int>(packet->type) << "\n";
                return 1;
            }
        }

        return 0;
    }
}

namespace UdprClient {
    int udprClient(struct sockaddr_in send_address, Cleaner &cleaner) {
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            std::cerr << "ERROR: Could not create socket: " << strerror(errno) << "\n";
            return 1;
        }
        cleaner.sockfd = sockfd;

        set_sock_timeout(sockfd);

        std::vector<char> data = read_stdin();
        uint64_t session = gen_session_id();
        char buffer[PPCB::MAX_PACKET_SIZE];

        { // send CONN packet
            PPCB::Packet packet{
                    .type = PPCB::PacketType::CONN,
                    .session_id = session,
                    .conn = {.conn_type = PPCB::ConnType::UDPR, .length = htonll(data.size())}
            };

            ssize_t sent = sendto(sockfd, &packet, packet.size(), 0, (struct sockaddr *) &send_address,
                                  sizeof(send_address));
            if (sent != packet.size()) {
                std::cerr << "ERROR: Could not send CONN packet: " << strerror(errno) << "\n";
                return 1;
            }
        }

        do { // receive CONNACC or CONRJT
            socklen_t len = sizeof(send_address);
            ssize_t recv_len = recvfrom(sockfd, buffer, PPCB::MAX_PACKET_SIZE, 0, (struct sockaddr *) &send_address,
                                        &len);
            if (recv_len < 0) {
                std::cerr << "ERROR: Could not receive CONNACC/CONRJT packet: " << strerror(errno) << "\n";
                return 1;
            }

            auto *packet = reinterpret_cast<PPCB::Packet *>(buffer);
            try {
                packet->validate(recv_len);
            } catch (std::runtime_error &e) {
                std::cerr << "ERROR: Invalid packet received: " << e.what() << "\n";
                return 1;
            }
            packet->print();

            if (packet->session_id != session)
                continue;

            if (packet->type == PPCB::PacketType::CONRJT) {
                std::cerr << "ERROR: Connection refused by server\n";
                return 1;
            }

            if (packet->type != PPCB::PacketType::CONNACC) {
                std::cerr << "ERROR: Unexpected packet type: " << static_cast<int>(packet->type) << "\n";
                return 1;
            }

            break;
        } while (true);

        // send data in DATA packets and wait for ACC

        uint64_t packet_id = 0;
        for (auto const chunk: data | std::views::chunk(UDP_DATA_SIZE)) {
            for (int retries = 0; retries < MAX_RETRANSMITS; retries++) {
                auto packet = reinterpret_cast<PPCB::Packet *>(buffer);

                packet->type = PPCB::PacketType::DATA;
                packet->session_id = session;
                packet->data.packet_id = packet_id;
                packet->data.data_length = chunk.size();

                packet->data.packet_id = htonll(packet->data.packet_id);
                packet->data.data_length = htonl(packet->data.data_length);

                std::copy(chunk.begin(), chunk.end(), packet->data.data);

                ssize_t sent = sendto(sockfd, packet, packet->size(), 0, (struct sockaddr *) &send_address,
                                      sizeof(send_address));
                if (sent != packet->size()) {
                    auto err = errno;
                    if (err == EAGAIN)
                        goto retry;
                    std::cerr << "ERROR: Could not send DATA packet: " << strerror(err) << "\n";
                    return 1;
                }

                do { // receive ACCs
                    socklen_t len = sizeof(send_address);
                    ssize_t recv_len = recvfrom(sockfd, buffer, PPCB::MAX_PACKET_SIZE, 0,
                                                (struct sockaddr *) &send_address,
                                                &len);
                    if (recv_len < 0) {
                        auto err = errno;
                        if (err == EAGAIN)
                            goto retry;
                        std::cerr << "ERROR: Could not receive ACK packet: " << strerror(err) << "\n";
                        return 1;
                    }

                    auto *packet = reinterpret_cast<PPCB::Packet *>(buffer);
                    try {
                        packet->validate(recv_len);
                    } catch (std::runtime_error &e) {
                        std::cerr << "ERROR: Invalid packet received: " << e.what() << "\n";
                        return 1;
                    }
                    packet->print();

                    if (packet->session_id != session)
                        continue;

                    if (packet->type == PPCB::PacketType::RJT) {
                        std::cerr << "ERROR: Server rejected packet\n";
                        return 1;
                    }

                    if (packet->type != PPCB::PacketType::ACC) {
                        std::cerr << "ERROR: Unexpected packet type: " << static_cast<int>(packet->type) << "\n";
                        return 1;
                    }

                    if (packet->acc.packet_id != packet_id) {
                        std::cerr << "ERROR: Invalid packet_id in ACC packet\n";
                        return 1;
                    }

                    break;
                } while (true);
                goto transmited;
                retry:
                if (retries == MAX_RETRANSMITS - 1) {
                    std::cerr << "ERROR: Maximum number of retransmits reached\n";
                    return 1;
                }
            }
            transmited:
            packet_id++;
        }

        {  // receive RCVD or RJT
            socklen_t len = sizeof(send_address);
            ssize_t recv_len = recvfrom(sockfd, buffer, PPCB::MAX_PACKET_SIZE, 0, (struct sockaddr *) &send_address,
                                        &len);
            if (recv_len < 0) {
                std::cerr << "ERROR: Could not receive RCVD packet: " << strerror(errno) << "\n";
                return 1;
            }

            auto *packet = reinterpret_cast<PPCB::Packet *>(buffer);
            try {
                packet->validate(recv_len);
            } catch (std::runtime_error &e) {
                std::cerr << "ERROR: Invalid packet received: " << e.what() << "\n";
                return 1;
            }
            packet->print();

            if (packet->session_id != session) {
                std::cerr << "ERROR: Invalid session ID in RCVD packet\n";
                return 1;
            }

            if (packet->type != PPCB::PacketType::RCVD) {
                std::cerr << "ERROR: Unexpected packet type: " << static_cast<int>(packet->type) << "\n";
                return 1;
            }
        }

        return 0;
    }
}
