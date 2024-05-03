#include <cstdlib>
#include <stdexcept>
#include <climits>
#include <cstring>
#include <csignal>
#include <asm-generic/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include "common.h"

#if PCAP

#include <ctime>
#include <fcntl.h>

#endif


namespace utils {
    uint64_t htonll(uint64_t host) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        return host;
#else
        return htobe64(host);
#endif
    }

    uint64_t ntohll(uint64_t network) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        return network;
#else
        return be64toh(network);
#endif
    }

    uint16_t read_port(const char *port) {
        char *endptr;
        unsigned long p = strtoul(port, &endptr, 10);
        if (*endptr != '\0' || p == 0 || ((p == ULONG_MAX) && (errno == ERANGE))) {
            throw std::invalid_argument("Invalid port number");
        }
        return static_cast<uint16_t>(p);
    }

    void open_pcap(__attribute_maybe_unused__ bool server) {
#if PCAP
        const auto filename = server ? "/tmp/server.ppcb" : "/tmp/client.ppcb";
        int filefd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (filefd != 5) {
            if (filefd < 0)
                throw std::runtime_error("open error: " + std::string(strerror(errno)));
            if (dup2(filefd, 5) != 5)
                throw std::runtime_error("dup2 error: " + std::string(strerror(errno)));
            if (close(filefd) < 0)
                throw std::runtime_error("close error: " + std::string(strerror(errno)));
        }
#endif
    }

}  // namespace utils

namespace ppcb {

    void conf_sock(int fd) {
        struct timeval tv{
                .tv_sec = constants::max_wait,
                .tv_usec = 0
        };
        if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
            throw std::runtime_error("setsockopt error: " + std::string(strerror(errno)));
    }

    void reset_sock(int fd) {
        struct timeval tv{
                .tv_sec = 0,
                .tv_usec = 0
        };
        if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
            throw std::runtime_error("setsockopt error: " + std::string(strerror(errno)));
    }

    namespace packet {
        void validate(const types::Header *header, ssize_t read_bytes) {
            // check if header is complete
            if (read_bytes < static_cast<ssize_t>(sizeof(types::Header)))
                throw std::runtime_error("Packet header is too short");

            // check if packet type is valid
            if (header->type < types::PacketType::CONN || header->type > types::PacketType::RCVD)
                throw std::runtime_error("Invalid packet type");

            // if data, validate if data header is complete
            if (header->type == types::PacketType::DATA) {
                if (read_bytes < static_cast<ssize_t>(sizeof(types::DataPacket)))
                    throw std::runtime_error("DATA packet header is too short");
                // if data too short, it will get caught in the next if
            }

            // validate packet size
            if (read_bytes != sizeof_packetn(header))
                throw std::runtime_error("Packet size mismatch");

            // validate conn type
            if (header->type == types::PacketType::CONN) {
                const auto conn = reinterpret_cast<const types::ConnPacket *>(header);
                if (conn->conn_type < types::ConnType::TCP || conn->conn_type > types::ConnType::UDPR)
                    throw std::runtime_error("Invalid connection type");
            }
        }

        ssize_t sizeof_packetn(const types::Header *header) {
            if (header->type != types::PacketType::DATA)
                return sizeof_packeth(header);

            const auto data = reinterpret_cast<const types::DataPacket *>(header);
            return static_cast<ssize_t>(sizeof(types::DataPacket)) + ntohl(data->data_length);
        }

        ssize_t sizeof_packeth(const types::Header *header) {
            switch (header->type) {
                case types::PacketType::CONN:
                    return sizeof(types::ConnPacket);

                case types::PacketType::CONNACC:
                case types::PacketType::CONRJT:
                case types::PacketType::RCVD:
                    return sizeof(types::Header);

                case types::PacketType::DATA: {
                    const auto data = reinterpret_cast<const types::DataPacket *>(header);
                    return static_cast<ssize_t>(sizeof(types::DataPacket)) + data->data_length;
                }

                case types::PacketType::RJT:
                case types::PacketType::ACC:
                    return sizeof(types::AccRjtPacket);

                default:
                    return sizeof(types::Header);
            }
        }

#if PPCB_DEBUG > 0

        std::string packet_type_to_string(types::PacketType type) {
            switch (type) {
                case types::PacketType::CONN:
                    return "CONN";
                case types::PacketType::CONNACC:
                    return "CONNACC";
                case types::PacketType::CONRJT:
                    return "CONRJT";
                case types::PacketType::DATA:
                    return "DATA";
                case types::PacketType::ACC:
                    return "ACC";
                case types::PacketType::RJT:
                    return "RJT";
                case types::PacketType::RCVD:
                    return "RCVD";
                default:
                    return "UNKNOWN(" + std::to_string(static_cast<uint8_t>(type)) + ")";
            }
        }

#endif

        void print_packet(__attribute_maybe_unused__ const types::Header *header, __attribute_maybe_unused__ char dir) {
#if PCAP == 1
            int filefd = 5;
            time_t seconds = time(NULL);
            write(filefd, &seconds, sizeof(seconds));
            write(filefd, &dir, sizeof(dir));
            long packet_size;
            switch (header->type) {
                case types::PacketType::CONN:
                    packet_size = sizeof(types::ConnPacket);
                    break;
                case types::PacketType::DATA:
                    packet_size = sizeof(types::DataPacket);
                    break;
                case types::PacketType::ACC:
                case types::PacketType::RJT:
                    packet_size = sizeof(types::AccRjtPacket);
                    break;
                default:
                    packet_size = sizeof(types::Header);
            }
            auto offset = 0;
            auto buf = reinterpret_cast<const char *>(header);
            while (packet_size > 0) {
                ssize_t written = write(filefd, buf + offset, packet_size);
                if (written < 0)
                    throw std::runtime_error("write error: " + std::string(strerror(errno)));
                packet_size -= written;
                offset += written;
            }
#endif

#if PPCB_DEBUG > 0
            std::cout << dir << " ";
            std::cout << packet_type_to_string(header->type) << "{";
            std::cout << "ssid=" << std::hex << header->session_id << std::dec;
            if (header->type == types::PacketType::CONN) {
                const auto conn = reinterpret_cast<const types::ConnPacket *>(header);
                std::cout << ", conn_type=";
                switch (conn->conn_type) {
                    case types::ConnType::TCP:
                        std::cout << "TCP";
                        break;
                    case types::ConnType::UDP:
                        std::cout << "UDP";
                        break;
                    case types::ConnType::UDPR:
                        std::cout << "UDPR";
                        break;
                    default:
                        std::cout << "UNKNOWN(" << static_cast<uint8_t>(conn->conn_type) << ")";
                }
                std::cout << ", length=" << utils::ntohll(conn->length);
            } else if (header->type == types::PacketType::DATA) {
                const auto data = reinterpret_cast<const types::DataPacket *>(header);
                auto data_len = ntohl(data->data_length);
                std::cout << ", pid=" << utils::ntohll(data->packet_id);
                std::cout << ", data_length=" << data_len;
                auto data_ptr = reinterpret_cast<const char *>(data) + sizeof(types::DataPacket);
                std::cout << ", data=" << std::string(data_ptr, data_len > 10 ? 10 : data_len);
            } else if (header->type == types::PacketType::ACC || header->type == types::PacketType::RJT) {
                const auto acc_rjt = reinterpret_cast<const types::AccRjtPacket *>(header);
                std::cout << ", pid=" << utils::ntohll(acc_rjt->packet_id);
            }
            std::cout << "}\n";
#endif
        }

        void ntoh_packet(types::Header *header) {
            switch (header->type) {
                case types::PacketType::CONN: {
                    auto conn = reinterpret_cast<types::ConnPacket *>(header);
                    conn->length = utils::ntohll(conn->length);
                    break;
                }
                case types::PacketType::DATA: {
                    auto data = reinterpret_cast<types::DataPacket *>(header);
                    data->packet_id = utils::ntohll(data->packet_id);
                    data->data_length = ntohl(data->data_length);
                    break;
                }
                case types::PacketType::ACC:
                case types::PacketType::RJT: {
                    auto acc_rjt = reinterpret_cast<types::AccRjtPacket *>(header);
                    acc_rjt->packet_id = utils::ntohll(acc_rjt->packet_id);
                    break;
                }
                default:
                    break;
            }
        }

        void hton_packet(types::Header *header) {
            switch (header->type) {
                case types::PacketType::CONN: {
                    auto conn = reinterpret_cast<types::ConnPacket *>(header);
                    conn->length = utils::htonll(conn->length);
                    break;
                }
                case types::PacketType::DATA: {
                    auto data = reinterpret_cast<types::DataPacket *>(header);
                    data->packet_id = utils::htonll(data->packet_id);
                    data->data_length = htonl(data->data_length);
                    break;
                }
                case types::PacketType::ACC:
                case types::PacketType::RJT: {
                    auto acc_rjt = reinterpret_cast<types::AccRjtPacket *>(header);
                    acc_rjt->packet_id = utils::htonll(acc_rjt->packet_id);
                    break;
                }
                default:
                    break;
            }
        }
    }

} // namespace ppcb

ssize_t tcp_readn(int fd, void *buf, ssize_t n) {
    if (n == 0) return 0;
    ssize_t nleft = n;
    ssize_t nread;
    char *ptr = static_cast<char *>(buf);

    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                throw network::exceptions::TimeoutException();
            throw std::runtime_error("read error: " + std::string(strerror(errno)));
        } else if (nread == 0)
            break;            // EOF

        nleft -= nread;
        ptr += nread;
    }

    return n - nleft;
}

ssize_t tcp_writen(int fd, const void *vptr, ssize_t n) {
    ssize_t nwritten;
    const char *ptr;

    ptr = static_cast<const char *>(vptr);    // Can't do pointer arithmetic on void*.
    auto nleft = n;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (errno == EINTR)
                throw network::exceptions::TimeoutException();
            throw std::runtime_error("write error: " + std::string(strerror(errno)));
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}

namespace network {

    ppcb::types::Header *read_packet(const Connection &conn, void *buf) {
        struct sockaddr addr{};
        socklen_t addr_len = sizeof(addr);
        return read_packet_addr(conn, buf, &addr, &addr_len);
    }

    ppcb::types::Header *
    read_packet_addr(const Connection &conn, void *buf, struct sockaddr *addr, socklen_t *addr_len) {
        if (conn.type == ppcb::types::ConnType::TCP) {
            auto cbuf = reinterpret_cast<char *>(buf); // cast to char* for pointer arithmetic
            auto recv_len = tcp_readn(conn.fd, buf, sizeof(ppcb::types::Header));
            if (recv_len == 0)
                throw std::runtime_error("Connection closed");
            auto header = reinterpret_cast<ppcb::types::Header *>(cbuf);
            if (header->type == ppcb::types::PacketType::DATA)
                recv_len += tcp_readn(conn.fd, cbuf + recv_len,
                                      sizeof(ppcb::types::DataPacket) - sizeof(ppcb::types::Header));
            recv_len += tcp_readn(conn.fd, cbuf + recv_len, ppcb::packet::sizeof_packetn(header) - recv_len);
            ppcb::packet::print_packet(reinterpret_cast<ppcb::types::Header *>(buf), 'R');
            ppcb::packet::validate(header, recv_len);
            return header;
        } else if (conn.type == ppcb::types::ConnType::UDP || conn.type == ppcb::types::ConnType::UDPR) {
            auto recv_len = recvfrom(conn.fd, buf, ppcb::constants::max_packet_size, 0,
                                     addr, addr_len);
            if (recv_len < 0 && errno == EAGAIN)
                throw exceptions::TimeoutException();
            else if (recv_len < 0)
                throw std::runtime_error("recvfrom error: " + std::string(strerror(errno)));
            //if (*addr_len != conn.peer_addr_len || memcmp(addr, conn.peer_addr, *addr_len) != 0)
            //    throw exceptions::WrongPeerException();
            auto header = reinterpret_cast<ppcb::types::Header *>(buf);
            ppcb::packet::print_packet(reinterpret_cast<ppcb::types::Header *>(buf), 'R');
            ppcb::packet::validate(header, recv_len);
            return header;
        } else
            throw std::runtime_error("Unsupported connection type");
    }

    ssize_t writen(const Connection &conn, const void *vptr, ssize_t n) {
        ppcb::packet::print_packet(reinterpret_cast<const ppcb::types::Header *>(vptr), 'W');
        if (conn.type == ppcb::types::ConnType::TCP) {
            auto sent_len = tcp_writen(conn.fd, vptr, n);
            return sent_len;
        } else if (conn.type == ppcb::types::ConnType::UDP || conn.type == ppcb::types::ConnType::UDPR) {
            auto sent_len = sendto(conn.fd, vptr, n, 0, conn.peer_addr, conn.peer_addr_len);
            if (sent_len < 0 && errno == EAGAIN)
                throw exceptions::TimeoutException();
            else if (sent_len < 0)
                throw std::runtime_error("sendto error: " + std::string(strerror(errno)));
            return sent_len;
        } else
            throw std::runtime_error("Unsupported connection type");
    }

    void
    expect_packet(__attribute_maybe_unused__ const Connection &conn, ppcb::types::PacketType expected_type,
                  const ppcb::types::Header *header) {
        if (header->type != expected_type) {
            auto expected_int = static_cast<uint8_t>(expected_type);
            auto got_int = static_cast<uint8_t>(header->type);
            throw exceptions::UnexpectedPacketException(expected_int, got_int);
        }
    }
} // namespace network