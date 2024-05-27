#include <arpa/inet.h>
#include <sstream>
#include <sys/poll.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include "common.h"
#include "client.h"

struct sockaddr_in get_server_address(char const *host, uint16_t port, bool ipv4);

int main(int argc, char *argv[]) {
    char *host = nullptr;
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

        if (!ipv4 && !ipv6) {
            std::cerr << "IPv4 or IPv6 is required" << std::endl;
            return 1;
        }

        if (seat == 0) {
            std::cerr << "-N, -E, -S, or -W is required" << std::endl;
            return 1;
        }
    }
    auto server_address = get_server_address(host, port, ipv4);

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

    auto conn = utils::Connection(server_socket,
                                  utils::addr2str(reinterpret_cast<sockaddr *>(&my_address), my_address_size),
                                  utils::addr2str(reinterpret_cast<sockaddr *>(&server_address),
                                                  sizeof(server_address)));
    conn.should_log = automatic;

    { // send IAM
        std::string iam = "IAMN\r\n";
        iam[3] = seat;
        conn.writeline(iam);
    }

    auto line = conn.readline();
    if (line.starts_with('B')) {
        auto ss = utils::parseline(line, "BUSY");
        std::string busylist;
        ss >> busylist;
        std::cout << "Place taken, list of taken places received: ";
        for (unsigned int i = 0; i < busylist.size(); i++) {
            std::cout << busylist[i];
            if (i + 1 < busylist.size()) {
                std::cout << ", ";
            }
        }
        std::cout << "." << std::endl;
        return 1;
    }
}

struct sockaddr_in get_server_address(char const *host, uint16_t port, bool ipv4) {
    struct addrinfo hints{};
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = ipv4 ? AF_INET : AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    struct addrinfo *address_result;
    int errcode = getaddrinfo(host, nullptr, &hints, &address_result);
    if (errcode != 0) {
        throw std::runtime_error("getaddrinfo: " + std::string(gai_strerror(errcode)));
    }

    struct sockaddr_in send_address{};
    send_address.sin_family = ipv4 ? AF_INET : AF_INET6;
    send_address.sin_addr.s_addr =       // IP address
            ((struct sockaddr_in *) (address_result->ai_addr))->sin_addr.s_addr;
    send_address.sin_port = htons(port); // port from the command line

    freeaddrinfo(address_result);

    return send_address;
}