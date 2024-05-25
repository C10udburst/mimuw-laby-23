#include <arpa/inet.h>
#include <sstream>
#include <sys/poll.h>
#include <cstring>
#include "common.h"

struct sockaddr_in get_server_address(char const *host, uint16_t port);

int main(int argc, char *argv[]) {
    char* host = nullptr;
    u_int16_t port = 0;
    bool ipv4 = false;
    bool ipv6 = false;
    bool automatic = false;
    char seat = 0;

    {  // read arguments
        int c;
        while ((c = getopt(argc, argv, "h:p:46NESWa")) != -1) {
            switch (c) {
                case 'h':
                    host = optarg;
                    break;
                case 'p':
                    try {
                        port = utils::read_port(optarg);
                    } catch (const std::exception &e) {
                        std::cerr << e.what() << std::endl;
                        return 1;
                    }
                    break;
                case '4':
                    ipv4 = true;
                    break;
                case '6':
                    ipv6 = true;
                    break;
                case 'N':
                case 'E':
                case 'S':
                case 'W':
                    if (seat != 0) {
                        std::cerr << "-N, -E, -S, -W are mutually exclusive" << std::endl;
                        return 1;
                    }
                    seat = static_cast<char>(c);
                    break;
                case 'a':
                    automatic = true;
                    break;
                default:
                    break;
            }
        }

        if (host == nullptr) {
            std::cerr << "Host is required -h" << std::endl;
            return 1;
        }

        if (port == 0) {
            std::cerr << "Port is required -p" << std::endl;
            return 1;
        }

        if (ipv4 && ipv6) {
            std::cerr << "IPv4 and IPv6 are mutually exclusive" << std::endl;
            return 1;
        }

        if (seat == 0) {
            std::cerr << "-N, -E, -S, or -W is required" << std::endl;
            return 1;
        }

        auto server_address = get_server_address(host, port);

        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == -1) {
            std::cerr << "Error creating socket" << std::endl;
            return 1;
        }

        if (connect(server_socket, (sockaddr *) &server_address, sizeof(server_address)) == -1) {
            std::cerr << "Error connecting to the server" << std::endl;
            return 1;
        }

        sockaddr_in my_address{};
        socklen_t my_address_size = sizeof(my_address);
        if (getsockname(server_socket, (sockaddr *) &my_address, &my_address_size) == -1) {
            std::cerr << "Error getting my address" << std::endl;
            return 1;
        }

        auto conn = kierki::Connection{
            .fd = server_socket,
            .my_addr = kierki::Addr{
                .addr = my_address,
                .addr_size = my_address_size
            },
            .their_addr = kierki::Addr{
                .addr = server_address,
                .addr_size = sizeof(server_address)
            }
        };

        conn.init(0);

        std::stringstream ss;
        ss << "IAM" << seat << "\r\n";
        conn.writeline(ss.str());

        /*
         * pollfd:
         * 1 - stdin
         * 2 - server_socket
         */

        pollfd fds[2];
        fds[0].fd = 0;
        fds[0].events = POLLIN;
        fds[1].fd = server_socket;
        fds[1].events = POLLIN | POLLHUP | POLLERR | POLLNVAL;

        while (true) {
            for (auto &fd : fds) {
                fd.revents = 0;
            }

            if (poll(fds, 2, -1) == -1) {
                std::cerr << "Error polling" << std::endl;
                return 1;
            }

            if (fds[1].revents & (POLLHUP | POLLERR | POLLNVAL)) {
                std::cerr << "Server closed the connection" << std::endl;
                return 1;
            }

            if (fds[0].revents & POLLIN) {
                std::string line;
                std::getline(std::cin, line);
                if (line.empty()) {
                    continue;
                }
                if (!line.ends_with("\r\n"))
                    line += "\r\n";
                conn.writeline(line);
            }

            if (fds[1].revents & POLLIN) {
                std::string line = conn.readline();
                std::cout << line;
            }
        }
    }
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