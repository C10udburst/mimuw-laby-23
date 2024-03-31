#include "packets.h"
#include "common.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct sockaddr_in get_server_address(char const *host);
static PPCB::ConnType read_type(char const* type);

void recv();
void send();

struct sockaddr_in sockaddrIn;
bool server;
uint16_t port;
PPCB::ConnType connType;
char buffer[PPCB::MAX_PACKET_SIZE];
int sockfd;
uint64_t sessionId;

int main(int argc, char *argv[]) {
    static_assert(PPCB::IS_DEBUG, "must build in debug mode");
    if (argc != 4) {
        std::cerr << "usage: " << argv[0] << "<tcp/udp/udpr> [-l <port>] [<host> <port>]\n";
        return 1;
    }

    port = read_port(argv[3]);
    connType = read_type(argv[1]);

    sockfd = socket(AF_INET, connType == PPCB::ConnType::TCP ? SOCK_STREAM : SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "ERROR: Could not create socket\n";
        return 1;
    }

    server = strcmp(argv[2], "-l") == 0;

    if (server) {
        sockaddrIn.sin_family = AF_INET;
        sockaddrIn.sin_addr.s_addr = htonl(INADDR_ANY);
        sockaddrIn.sin_port = htons(port);
    } else {
        sockaddrIn = get_server_address(argv[2]);
    }

    if (server) {
        if (bind(sockfd, (struct sockaddr *) &sockaddrIn, sizeof(sockaddrIn)) < 0) {
            std::cerr << "ERROR: Could not bind socket\n";
            return 1;
        }
        std::cout << "Server started on port " << port << "\n";
    }

    if (server)
        recv();

    do {
        try {
            send();
            recv();
        } catch (std::runtime_error &e) {
            std::cerr << "ERROR: " << e.what() << "\n";
        }
    } while (true);
}

void recv() {
    if (connType == PPCB::ConnType::UDP) {
        socklen_t len = sizeof(sockaddrIn);
        ssize_t n = recvfrom(sockfd, buffer, PPCB::MAX_PACKET_SIZE, 0, (struct sockaddr *) &sockaddrIn, &len);
        if (n < 0) {
            std::cerr << "ERROR: Could not receive data\n";
            return;
        }
        PPCB::Packet* packet = reinterpret_cast<PPCB::Packet*>(buffer);
        if (server) {
            std::cout << inet_ntoa(sockaddrIn.sin_addr) << ":" << ntohs(sockaddrIn.sin_port);
        }
        std::cout << "->";
        packet->validate(n);
        packet->print();
        if (packet->type == PPCB::PacketType::CONN)
            sessionId = packet->session_id;
    } else {

    }
}

static PPCB::PacketType pt_from_str(std::string str) {
    for (auto& c: str)
        c = std::toupper(c);

    if (str == "CONN" || str == "1" || str == "C")
        return PPCB::PacketType::CONN;
    if (str == "CONNACC" || str == "2" || str == "CA")
        return PPCB::PacketType::CONNACC;
    if (str == "CONRJT" || str == "3" || str == "CR")
        return PPCB::PacketType::CONRJT;
    if (str == "DATA" || str == "4" || str == "D")
        return PPCB::PacketType::DATA;
    if (str == "ACC" || str == "5" || str == "A")
        return PPCB::PacketType::ACC;
    if (str == "RJT" || str == "6" || str == "R")
        return PPCB::PacketType::RJT;
    if (str == "RCVD" || str == "7" || str == "RC")
        return PPCB::PacketType::RCVD;
    throw std::runtime_error("Invalid packet type: " + str);
}

void send() {
    std::string packetTypeStr;
    std::cin >> packetTypeStr;

    if (packetTypeStr == "") {
        return;
    }

    PPCB::Packet* packet = reinterpret_cast<PPCB::Packet*>(buffer);

    packet->type = pt_from_str(packetTypeStr);

    if (packet->type == PPCB::PacketType::CONN) {
        std::cin >> std::hex >> sessionId;
        packet->conn.conn_type = connType;
        std::cin >> packet->conn.length;
        packet->conn.length = htonll(packet->conn.length);
    }

    packet->session_id = sessionId;

    if (packet->type == PPCB::PacketType::DATA) {
        std::cin >> packet->data.packet_id;
        std::cin >> packet->data.data_length;
        std::cin.readsome(packet->data.data, packet->data.data_length);
        packet->data.data_length = htonl(packet->data.data_length);
        packet->data.packet_id = htonll(packet->data.packet_id);
    }

    if (packet->type == PPCB::PacketType::ACC) {
        std::cin >> packet->acc.packet_id;
    }

    if (packet->type == PPCB::PacketType::RJT) {
        std::cin >> packet->rjt.packet_id;
    }

    std::cout << "<-";
    packet->print();

    if (connType == PPCB::ConnType::UDP || connType == PPCB::ConnType::UDPR) {
        ssize_t sent = sendto(sockfd, buffer, packet->size(), 0, (struct sockaddr *) &sockaddrIn, sizeof(sockaddrIn));
        if (sent != packet->size()) {
            std::cerr << "ERROR: Could not send packet: " << strerror(errno) << "\n";
            return;
        }
    } else {
        if (send(sockfd, buffer, packet->size(), 0) != packet->size()) {
            std::cerr << "ERROR: Could not send packet: " << strerror(errno) << "\n";
            return;
        }
    }
}

struct sockaddr_in get_server_address(char const *host) {
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

static PPCB::ConnType read_type(char const* type) {
    if (strcmp(type, "tcp") == 0) {
        return PPCB::ConnType::TCP;
    } else if (strcmp(type, "udp") == 0) {
        return PPCB::ConnType::UDP;
    } else if (strcmp(type, "udpr") == 0) {
        return PPCB::ConnType::UDPR;
    } else {
        throw std::runtime_error("Invalid connection type");
    }
}