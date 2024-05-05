#include <csignal>
#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include "server.h"

void server::handleConnection(const network::Connection &conn, char *buf) {
    if (ppcb::constants::debug)
        std::cout << "Handling connection from " << std::hex << conn.session_id << std::dec << std::endl;

    { // send CONNACC
        ppcb::types::Header connacc{
                .type = ppcb::types::PacketType::CONNACC,
                .session_id = conn.session_id,
        };
        ppcb::packet::hton_packet(&connacc);
        network::writen(conn, &connacc, sizeof(connacc));
    }
    if (conn.type != ppcb::types::ConnType::UDPR) {
        // no retransmission mode
        uint64_t packet_id = 0;
        auto bytes_left = conn.length;
        while (bytes_left > 0) {
            ppcb::types::Header* packet;
            try {
                packet = network::read_packet(conn, buf);
                ppcb::packet::ntoh_packet(packet);
                if (packet->session_id != conn.session_id)
                    throw network::exceptions::WrongPeerException();
                network::expect_packet(conn, ppcb::types::PacketType::DATA, packet);
            } catch (const network::exceptions::UnexpectedPacketException &e) {
                //std::cerr << "ERROR: " << e.what() << std::endl;
                send_rjt(conn, buf);
                continue;
            } catch (const network::exceptions::WrongPeerException &e) {
                //std::cerr << "ERROR: " << e.what() << std::endl;
                send_rjt(conn, buf);
                continue;
            }
            auto data_packet = reinterpret_cast<ppcb::types::DataPacket *>(packet);
            if (data_packet->packet_id != packet_id) {
                throw std::runtime_error("Invalid packet id");
            }
            packet_id++;
            bytes_left -= data_packet->data_length;
            if (!ppcb::constants::debug) {
                auto cbuf = reinterpret_cast<char *>(buf);
                write(STDOUT_FILENO, cbuf + sizeof(ppcb::types::DataPacket), data_packet->data_length);
            }
        }
    } else {
        uint64_t packet_id = 0;
        auto bytes_left = conn.length;
        while (bytes_left > 0) {
            ppcb::types::DataPacket* data_packet;
            if (packet_id == 0) {
                try {
                    auto packet = network::read_packet(conn, buf);
                    ppcb::packet::ntoh_packet(packet);
                    if (packet->session_id != conn.session_id)
                        throw network::exceptions::WrongPeerException();
                    network::expect_packet(conn, ppcb::types::PacketType::DATA, packet);
                    data_packet = reinterpret_cast<ppcb::types::DataPacket *>(packet);
                    if (data_packet->packet_id != packet_id)
                        throw std::runtime_error("Invalid packet id");
                } catch (const network::exceptions::WrongPeerException &e) {
                    send_rjt(conn, buf);
                    continue;
                } catch (const network::exceptions::UnexpectedPacketException &e) {
                    send_rjt(conn, buf);
                    continue;
                }
            } else {
                udprReceive(conn, buf, packet_id);
            }
            bytes_left -= data_packet->data_length;
            if (!ppcb::constants::debug) {
                auto cbuf = reinterpret_cast<char *>(buf);
                write(STDOUT_FILENO, cbuf + sizeof(ppcb::types::DataPacket), data_packet->data_length);
            }
            packet_id++;
        }

        // flush stdio
        std::cout << std::flush;

        { // send the last acc
            auto acc = ppcb::types::AccRjtPacket{
                    .type = ppcb::types::PacketType::ACC,
                    .session_id = conn.session_id,
                    .packet_id = packet_id - 1,
            };
            auto acc_packet = reinterpret_cast<ppcb::types::Header *>(&acc);
            ppcb::packet::hton_packet(acc_packet);
            network::writen(conn, acc_packet, sizeof(acc));
        }
    }

    { // send RCVD
        ppcb::types::Header rcvd{
                .type = ppcb::types::PacketType::RCVD,
                .session_id = conn.session_id,
        };
        ppcb::packet::hton_packet(&rcvd);
        network::writen(conn, &rcvd, sizeof(rcvd));
    }

    if (ppcb::constants::debug)
        std::cout << "Connection from " << std::hex << conn.session_id << std::dec << " handled" << std::endl;
}

void server::send_rjt(const network::Connection &conn, void* buf) {
    auto packet = reinterpret_cast<ppcb::types::Header *>(buf);
    if (packet->type == ppcb::types::PacketType::CONN && conn.session_id != packet->session_id) {
        auto conrjt = ppcb::types::Header{
                .type = ppcb::types::PacketType::CONRJT,
                .session_id = packet->session_id,
        };
        ppcb::packet::hton_packet(&conrjt);
        network::writen(conn, &conrjt, sizeof(conrjt));
    } else if (packet->type == ppcb::types::PacketType::DATA) {
        auto data_packet = reinterpret_cast<ppcb::types::DataPacket *>(packet);
        auto rjt = ppcb::types::AccRjtPacket{
                .type = ppcb::types::PacketType::RJT,
                .session_id = packet->session_id,
                .packet_id = data_packet->packet_id,
        };
        auto rjt_packet = reinterpret_cast<ppcb::types::Header *>(&rjt);
        ppcb::packet::hton_packet(rjt_packet);
        network::writen(conn, rjt_packet, sizeof(rjt));
    }
}

ppcb::types::Header *server::udprReceive(const network::Connection &conn, void *buf, uint64_t expected_pid) {
    // we send ACC until we receive the expected packet
    ppcb::types::AccRjtPacket acc{
            .type = ppcb::types::PacketType::ACC,
            .session_id = conn.session_id,
            .packet_id = expected_pid - 1,
    };
    auto packet = reinterpret_cast<ppcb::types::Header *>(&acc);
    ppcb::packet::hton_packet(packet);
    for (int i = 0; i < ppcb::constants::max_retransmits; i++) {
        ssize_t packet_len;
        try {
            packet_len = writen(conn, packet, ppcb::packet::sizeof_packetn(packet));
        } catch (const network::exceptions::TimeoutException &e) {
            continue; // if timeout, resend
        }
        if (packet_len == 0)
            throw std::runtime_error("Connection closed");
        try {
            auto ack = read_packet(conn, buf);
            ppcb::packet::ntoh_packet(ack);
            if (ack->session_id != packet->session_id)
                throw network::exceptions::WrongPeerException();
            network::expect_packet(conn, ppcb::types::PacketType::DATA, ack);
            auto data_packet = reinterpret_cast<ppcb::types::DataPacket *>(ack);
            if (data_packet->packet_id < expected_pid)
                continue; // ignore old packets
            if (data_packet->packet_id == expected_pid)
                return ack; // ack->type == ppcb::types::PacketType::DATA && data_packet->packet_id == expected_pid
        } catch (const network::exceptions::TimeoutException &e) {
            continue; // if timeout, resend
        }
        // if wrong peer, we need to catch, so we can send RJT/CONNRJT
        catch (const network::exceptions::WrongPeerException &e) { }
        catch (const network::exceptions::UnexpectedPacketException &e) { }
        // we know buf has valid packet because otherwise we would have thrown an exception,
        // but we also know that the packet is not the expected one, so we can send RJT
        send_rjt(conn, buf);
        i--;
    }
    throw std::runtime_error("Too many retransmits");
}

int server::tcpServer(uint16_t port) {
    utils::open_pcap(true);
    utils::Cleaner cleaner;
    char buffer[ppcb::constants::max_packet_size + sizeof(ppcb::types::DataPacket) + 1];

    // Ignore SIGPIPE signals, so they are delivered as normal errors.
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        std::cerr << "ERROR: Could not set signal handler: " << strerror(errno) << "\n";
        return 1;
    }

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        std::cerr << "ERROR: Could not open socket: " << strerror(errno) << "\n";
        return 1;
    }

    cleaner.add_fd(socket_fd);

    struct sockaddr_in serv_addr{}, cli_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    { // bind and listen
        if (bind(socket_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            std::cerr << "ERROR: Could not bind socket: " << strerror(errno) << "\n";
            return 1;
        }
        if (listen(socket_fd, ppcb::constants::queue_size) < 0) {
            std::cerr << "ERROR: Could not listen on socket: " << strerror(errno) << "\n";
            return 1;
        }
    }

    if (ppcb::constants::debug)
        std::cout << "TCP listening on port " << port << std::endl;

    socklen_t clilen = sizeof(cli_addr);
    int newsockfd = -1;
    do {
        try {
            memset(buffer, 0, sizeof(buffer)); // clear buffer

            newsockfd = accept(socket_fd, (struct sockaddr *) &cli_addr, &clilen);
            if (newsockfd < 0) {
                std::cerr << "ERROR: Could not accept connection: " << strerror(errno) << "\n";
                return 1;
            }
            cleaner.add_fd(newsockfd);
            ppcb::conf_sock(newsockfd);
            auto conn = network::Connection{
                    .type = ppcb::types::ConnType::TCP,
                    .length = 0,
                    .fd = newsockfd,
                    .session_id = 0xABAB, // dummy value so errors are easier to spot
                    .peer_addr = reinterpret_cast<struct sockaddr *>(&cli_addr),
                    .peer_addr_len = clilen,
                    .is_server = true,
            };
            auto packet = network::read_packet(conn, buffer);
            ppcb::packet::ntoh_packet(packet);
            network::expect_packet(conn, ppcb::types::PacketType::CONN, packet);
            // no need to send rjt, because only way this is wrong is if the client is wrong, so UB
            auto conn_packet = reinterpret_cast<ppcb::types::ConnPacket *>(packet);
            if (conn_packet->conn_type != ppcb::types::ConnType::TCP)
                throw std::runtime_error("Invalid connection type");
            conn.session_id = packet->session_id;
            conn.length = conn_packet->length;

            handleConnection(conn, buffer);
        } catch (const std::exception &e) {
            std::cerr << "ERROR: " << e.what() << std::endl;
        }
        cleaner.free_fd(newsockfd);
    } while (true);
}

int server::udpServer(uint16_t port) {
    utils::open_pcap(true);
    utils::Cleaner cleaner;
    char buffer[ppcb::constants::max_packet_size + sizeof(ppcb::types::DataPacket) + 1];

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "ERROR: Could not open socket: " << strerror(errno) << "\n";
        return 1;
    }
    cleaner.add_fd(sockfd);

    struct sockaddr_in serv_addr{}, cli_addr{};

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "ERROR: Could not bind socket: " << strerror(errno) << "\n";
        return 1;
    }

    if (ppcb::constants::debug)
        std::cout << "UDP listening on port " << port << std::endl;

    socklen_t clilen = sizeof(cli_addr);
    do {
        ppcb::reset_sock(sockfd); // revert to infinite timeout

        try {
            memset(buffer, 0, sizeof(buffer)); // clear buffer

            auto conn = network::Connection{
                    .type = ppcb::types::ConnType::UDP,
                    .length = 0,
                    .fd = sockfd,
                    .session_id = 0xABAB, // dummy value so errors are easier to spot
                    .peer_addr = reinterpret_cast<struct sockaddr *>(&cli_addr),
                    .peer_addr_len = clilen,
                    .is_server = true,
            };

            // read_packet_addr will also fill in the peer_addr and peer_addr_len fields
            auto packet = network::read_packet_addr(conn, buffer, conn.peer_addr, &conn.peer_addr_len);
            try {
                ppcb::packet::ntoh_packet(packet);
                network::expect_packet(conn, ppcb::types::PacketType::CONN, packet);
            } catch (const network::exceptions::UnexpectedPacketException &e) {
                //std::cerr << "ERROR: " << e.what() << std::endl;
                send_rjt(conn, buffer);
                continue;
            }

            auto conn_packet = reinterpret_cast<ppcb::types::ConnPacket *>(packet);
            if (conn_packet->conn_type != ppcb::types::ConnType::UDPR && conn_packet->conn_type != ppcb::types::ConnType::UDP)
                throw std::runtime_error("Invalid connection type");
            conn.type = conn_packet->conn_type;
            conn.session_id = packet->session_id;
            conn.length = conn_packet->length;

            ppcb::conf_sock(sockfd);  // set timeout

            handleConnection(conn, buffer);
        } catch (const std::exception &e) {
            std::cerr << "ERROR: " << e.what() << std::endl;
        }
    } while (true);
}