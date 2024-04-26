#define PPCB_DEBUG 1

#include <cstring>
#include <netinet/in.h>
#include <csignal>
#include "server.h"
#include "client.h"

namespace input {
    ppcb::types::PacketType pt_from_string(std::string &str) {
        for (auto &c: str)
            c = std::toupper(c);

        if (str == "CONN" || str == "C")
            return ppcb::types::PacketType::CONN;
        if (str == "CONNACC" || str == "CA")
            return ppcb::types::PacketType::CONNACC;
        if (str == "CONRJT" || str == "CR")
            return ppcb::types::PacketType::CONRJT;
        if (str == "DATA" || str == "D")
            return ppcb::types::PacketType::DATA;
        if (str == "ACC" || str == "A")
            return ppcb::types::PacketType::ACC;
        if (str == "RJT" || str == "R")
            return ppcb::types::PacketType::RJT;
        if (str == "RCVD" || str == "RC")
            return ppcb::types::PacketType::RCVD;

        int i = std::stoi(str);

        if (i < 0 || i > 256)
            throw std::invalid_argument("Invalid packet type");

        auto u = static_cast<uint8_t>(i);
        return static_cast<ppcb::types::PacketType>(u);
    }

    ppcb::types::ConnType ct_from_string(std::basic_string<char> str) {
        for (auto &c: str)
            c = std::toupper(c);

        if (str == "TCP")
            return ppcb::types::ConnType::TCP;
        if (str == "UDP")
            return ppcb::types::ConnType::UDP;
        if (str == "UDPR")
            return ppcb::types::ConnType::UDPR;

        int i = std::stoi(str);

        if (i < 0 || i > 256)
            throw std::invalid_argument("Invalid connection type");

        auto u = static_cast<uint8_t>(i);
        return static_cast<ppcb::types::ConnType>(u);
    }

    ppcb::types::Header *read_packet(std::istream &is) {
        std::string stype;
        is >> stype;
        auto type = pt_from_string(stype);
        if (type == ppcb::types::PacketType::CONN) {
            auto *conn = new ppcb::types::ConnPacket;
            conn->type = type;
            is >> std::hex >> conn->session_id >> std::dec;
            std::string ctype;
            is >> ctype;
            conn->conn_type = ct_from_string(ctype);
            is >> conn->length;
            return reinterpret_cast<ppcb::types::Header *>(conn);
        }
        if (type == ppcb::types::PacketType::ACC || type == ppcb::types::PacketType::RJT) {
            auto *acc = new ppcb::types::AccRjtPacket;
            acc->type = type;
            is >> std::hex >> acc->session_id >> std::dec >> acc->packet_id;
            return reinterpret_cast<ppcb::types::Header *>(acc);
        }
        if (type == ppcb::types::PacketType::DATA) {
            auto *data = new ppcb::types::DataPacket;
            data->type = type;
            is >> std::hex >> data->session_id >> std::dec >> data->packet_id >> data->data_length;
            auto *buf = new char[data->data_length + sizeof(ppcb::types::DataPacket)];
            std::memcpy(buf, data, sizeof(ppcb::types::DataPacket));
            is.readsome(buf + sizeof(ppcb::types::DataPacket), data->data_length);
            return reinterpret_cast<ppcb::types::Header *>(buf);
        }
        auto *header = new ppcb::types::Header;
        header->type = type;
        is >> std::hex >> header->session_id >> std::dec;
        return header;
    }
} // namespace input

void tcpServer(network::Connection *conn, utils::Cleaner *cleaner, uint16_t port) {
    signal(SIGPIPE, SIG_IGN);

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        throw std::runtime_error("Could not open socket: " + std::string(strerror(errno)));
    }

    cleaner->add_fd(socket_fd);

    struct sockaddr_in serv_addr{};
    auto *cli_addr = new struct sockaddr_in;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    { // bind and listen
        if (bind(socket_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            throw std::runtime_error("Could not bind socket: " + std::string(strerror(errno)));
        }
        if (listen(socket_fd, ppcb::constants::queue_size) < 0) {
            throw std::runtime_error("Could not listen on socket: " + std::string(strerror(errno)));
        }
    }

    socklen_t clilen = sizeof(*cli_addr);

    int newsockfd = accept(socket_fd, (struct sockaddr *) cli_addr, &clilen);
    if (newsockfd < 0) {
        throw std::runtime_error("Could not accept connection: " + std::string(strerror(errno)));
    }

    cleaner->add_fd(newsockfd);

    conn->fd = newsockfd;
    conn->peer_addr = reinterpret_cast<struct sockaddr *>(cli_addr);
    conn->peer_addr_len = clilen;
}

void udpServer(network::Connection *conn, utils::Cleaner *cleaner, uint16_t port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        throw std::runtime_error("Could not open socket: " + std::string(strerror(errno)));
    }

    cleaner->add_fd(sockfd);

    struct sockaddr_in serv_addr{};
    auto *cli_addr = new struct sockaddr_in;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        throw std::runtime_error("Could not bind socket: " + std::string(strerror(errno)));
    }

    conn->fd = sockfd;
    conn->peer_addr = reinterpret_cast<struct sockaddr *>(cli_addr);
    conn->peer_addr_len = sizeof(*cli_addr);
}

void tcpClient(network::Connection *conn, utils::Cleaner *cleaner) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        throw std::runtime_error("Could not open socket: " + std::string(strerror(errno)));
    }

    cleaner->add_fd(sockfd);

    if (connect(sockfd, (struct sockaddr *) conn->peer_addr, conn->peer_addr_len) < 0) {
        throw std::runtime_error("Could not connect to server: " + std::string(strerror(errno)));
    }

    conn->fd = sockfd;
}

void udpClient(network::Connection *conn, utils::Cleaner *cleaner) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        throw std::runtime_error("Could not open socket: " + std::string(strerror(errno)));
    }

    cleaner->add_fd(sockfd);

    conn->fd = sockfd;
}

void update_conn(network::Connection *conn, ppcb::types::Header *header) {
    if (header->type == ppcb::types::PacketType::CONN) {
        auto *conn_packet = reinterpret_cast<ppcb::types::ConnPacket *>(header);
        conn->session_id = conn_packet->session_id;
        conn->type = conn_packet->conn_type;
        conn->length = conn_packet->length;
    }
    conn->session_id = header->session_id;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "ERROR: Invalid args. Usage:" << argv[0] << " <protocol> <ip/-l> <port>" << std::endl;
        return 1;
    }

    bool is_server = (std::string(argv[2]) == "-l");
    uint16_t port = utils::read_port(argv[3]);
    auto conn = network::Connection{
            .type = input::ct_from_string(std::string(argv[1])),
            .length = 0,
            .fd = -1,
            .session_id = client::gen_session_id(),
            .peer_addr = nullptr,
            .peer_addr_len = 0,
            .is_server = is_server
    };

    utils::Cleaner cleaner;
    char buffer[ppcb::constants::max_packet_size + 8];
    if (is_server) {
        if (conn.type == ppcb::types::ConnType::TCP)
            tcpServer(&conn, &cleaner, port);
        else if (conn.type == ppcb::types::ConnType::UDP)
            udpServer(&conn, &cleaner, port);
        else
            throw std::invalid_argument("Invalid connection type");
    } else {

        struct sockaddr_in addr = client::get_server_address(argv[2], port);
        conn.peer_addr = reinterpret_cast<struct sockaddr *>(&addr);
        conn.peer_addr_len = sizeof(addr);

        if (conn.type == ppcb::types::ConnType::TCP)
            tcpClient(&conn, &cleaner);
        else if (conn.type == ppcb::types::ConnType::UDP || conn.type == ppcb::types::ConnType::UDPR)
            udpClient(&conn, &cleaner);
        else
            throw std::invalid_argument("Invalid connection type: " + std::to_string(reinterpret_cast<int &>(conn.type)));
    }

    // start reading/writing packets
    if (is_server) { // first read
        try {
            auto *header = network::read_packet_addr(conn, buffer, conn.peer_addr, &conn.peer_addr_len);
            update_conn(&conn, header);
        } catch (const std::exception &e) {
            std::cout << "ERROR: " << e.what() << std::endl;
            return 1;
        }
    }
    do { // write
        try {
            auto packet = input::read_packet(std::cin);
            update_conn(&conn, packet);
            ppcb::packet::hton_packet(packet);
            network::writen(conn, packet, ppcb::packet::sizeof_packetn(packet));

            auto *header = network::read_packet(conn, buffer);
            update_conn(&conn, header);
        } catch (const std::exception &e) {
            std::cout << "ERROR: " << e.what() << std::endl;
        }
    } while (true);
}