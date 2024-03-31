#include "packets.h"
#include "common.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <csignal>
#include <netinet/in.h>

class Cleaner {
public:
    int sockfd = -1;

    ~Cleaner() {
        if (sockfd >= 0)
            close(sockfd);
    }
};

int tcpServer(uint16_t port, Cleaner &cleaner);
int udpServer(uint16_t port, Cleaner &cleaner);


int main(int argc, char *argv[]) {
    Cleaner cleaner;
    if (argc != 3) {
        std::cerr << "ERROR: Invalid number of arguments\n";
        return 1;
    }

    uint16_t port;
    try {
        port = read_port(argv[2]);
    } catch (std::runtime_error &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }

    if (strcmp(argv[1], "tcp") == 0) {
        return tcpServer(port, cleaner);
    } else if (strcmp(argv[1], "udp") == 0) {
        return udpServer(port, cleaner);
    } else {
        std::cerr << "ERROR: Invalid protocol. Must be either 'tcp' or 'udp'\n";
        return 1;
    }
}


int tcpServer(uint16_t port, Cleaner &cleaner) {
    return 0;
}

void handleUdpConnection(int sockfd, PPCB::Packet connection, char* buffer, struct sockaddr_in conn_addr) {
    struct sockaddr_in cli_addr{};

    connection.print();

    connection.conn.length = ntohll(connection.conn.length);

    // send CONNACC

    {
        PPCB::Packet packet{};
        packet.type = PPCB::PacketType::CONNACC;
        packet.session_id = connection.session_id;

        if (sendto(sockfd, &packet, packet.size(), 0, (struct sockaddr *) &conn_addr, sizeof(conn_addr)) < 0) {
            throw std::runtime_error("Could not send CONNACC packet: " + std::string(strerror(errno)));
        }
    }

    socklen_t len = sizeof(cli_addr);
    while (connection.conn.length > 0) {
        ssize_t recv_len = recvfrom(sockfd, buffer, PPCB::MAX_PACKET_SIZE, 0, (struct sockaddr *) &cli_addr, &len);
        if (recv_len < 0) {
            throw std::runtime_error("Could not receive data: " + std::string(strerror(errno)));
        }

        auto* packet = reinterpret_cast<PPCB::Packet*>(buffer);
        try {
            packet->validate(recv_len);
        } catch (std::runtime_error &e) {
            throw std::runtime_error("Invalid packet received: " + std::string(e.what()));
        }

        packet->print();

        if (packet->session_id != connection.session_id || cli_addr.sin_port != conn_addr.sin_port || cli_addr.sin_addr.s_addr != conn_addr.sin_addr.s_addr) {
            // refuse connection from different session
            if (packet->type == PPCB::PacketType::CONN) {
                PPCB::Packet rjtpacket{};
                rjtpacket.type = PPCB::PacketType::CONRJT;
                rjtpacket.session_id = packet->session_id;

                if (sendto(sockfd, &rjtpacket, rjtpacket.size(), 0, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0) {
                    throw std::runtime_error("Could not send CONRJT packet: " + std::string(strerror(errno)));
                }
            }
            continue;
        }

        if (packet->type != PPCB::PacketType::DATA) {
            throw std::runtime_error("Expected DATA packet, got " + std::to_string(static_cast<int>(packet->type)));
        }

        connection.conn.length -= ntohl(packet->data.data_length);
    }

    // send RCVD
    {
        PPCB::Packet packet{};
        packet.type = PPCB::PacketType::RCVD;
        packet.session_id = connection.session_id;

        if (sendto(sockfd, &packet, packet.size(), 0, (struct sockaddr *) &conn_addr, sizeof(conn_addr)) < 0) {
            throw std::runtime_error("Could not send RCVD packet: " + std::string(strerror(errno)));
        }
    }
}

void handleUdprConnection(int sockfd, PPCB::Packet connection, char* buffer, struct sockaddr_in conn_addr) {

}

int udpServer(uint16_t port, Cleaner &cleaner) {
    char buffer[PPCB::MAX_PACKET_SIZE];

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "ERROR: Could not open socket: " << strerror(errno) << "\n";
        return 1;
    }
    cleaner.sockfd = sockfd;

    struct sockaddr_in serv_addr{}, cli_addr{};

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "ERROR: Could not bind socket: " << strerror(errno) << "\n";
        return 1;
    }

    ssize_t recv_len;
    do {
        memset(buffer, 0, sizeof(buffer)); // clear buffer

        socklen_t len = sizeof(cli_addr);
        recv_len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &cli_addr, &len);

        if (recv_len < 0) {
            std::cerr << "ERROR: Could not receive data: " << strerror(errno) << "\n";
            return 1;
        }

        auto* packet = reinterpret_cast<PPCB::Packet*>(buffer);
        try {
            packet->validate(recv_len);
        } catch (std::runtime_error &e) {
            std::cerr << "ERROR: Invalid packet received: " << e.what() << "\n";
            continue;
        }
        if (packet->type != PPCB::PacketType::CONN) {
            std::cerr << "ERROR: Expected CONN packet, got " << static_cast<int>(packet->type) << "\n";
            continue;
        }
        try {
            PPCB::Packet conn{};
            memcpy(&conn, packet, sizeof(conn));
            if (conn.conn.conn_type == PPCB::ConnType::UDP) {
                handleUdpConnection(sockfd, conn, buffer, cli_addr);
            } else if (conn.conn.conn_type == PPCB::ConnType::UDPR) {
                handleUdprConnection(sockfd, conn, buffer, cli_addr);
            } else {
                std::cerr << "ERROR: Invalid connection type: " << static_cast<int>(conn.conn.conn_type) << "\n";
                continue;
            }
        } catch (std::runtime_error &e) {
            std::cerr << "ERROR: " << e.what() << "\n";
            continue;
        }
    } while (recv_len > 0);

    return 0;
}