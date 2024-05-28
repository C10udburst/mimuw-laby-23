#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <csignal>
#include <thread>
#include "common.h"
#include "server.h"

int main(int argc, char *argv[]) {
    u_int16_t port = 0;
    char* filepath = nullptr;
    __time_t timeout = 5;

    { // read arguments
        int c;
        while ((c = getopt(argc, argv, "p:f:t:h")) != -1) {
            switch (c) {
                case 'p':
                    try {
                        port = utils::read_port(optarg);
                    } catch (const std::exception &e) {
                        std::cerr << e.what() << std::endl;
                        return 1;

                    }
                    break;
                case 'f':
                    filepath = optarg;
                    break;
                case 't':
                    timeout = strtol(optarg, nullptr, 10);
                    if (timeout <= 0) {
                        std::cerr << "Timeout must be a positive integer" << std::endl;
                        return 1;
                    }
                    break;
                case 'h':
                    std::cout <<  argv[0] << " [-p port] -f file [-t timeout]" << std::endl;
                    return 0;
                default:
                    break;
            }
        }

        if (filepath == nullptr) {
            std::cerr << "File is required -f" << std::endl;
            return 1;
        }
    }

    // start tcp server
    auto server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Cannot create socket" << std::endl;
        return 1;
    }

    auto server = server::Server{
        .clients = {},
        .gamefile = server::Gamefile(filepath),
        .taken = std::vector<std::string>(),
        .sync = server::Sync()
    };
    server.taken.reserve(13);
    for (auto i = 0; i < 4; i++)
        server.clients[i].seat = i;

    auto server_address = sockaddr_in{
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = in_addr{.s_addr = INADDR_ANY},
        .sin_zero = {}
    };

    if (bind(server_socket, (sockaddr *) &server_address, sizeof(server_address)) == -1) {
        std::cerr << "Cannot bind socket" << std::endl;
        return 1;
    }

    socklen_t len = sizeof server_address;

    if (getsockname(server_socket, (sockaddr *) &server_address, &len) == -1) {
        std::cerr << "Failed to get hostname" << std::endl;
        return 1;
    }

    if (listen(server_socket, 4) == -1) {
        std::cerr << "Cannot listen on socket" << std::endl;
        return 1;
    }

    auto my_addr = utils::addr2str(reinterpret_cast<sockaddr *>(&server_address), sizeof(server_address));
    while (server.sync.game_running) {
        sockaddr_in client_address{};
        socklen_t client_address_size = sizeof(client_address);
        // TODO: allow to interrupt when game is finished
        int client_socket = accept(server_socket, (sockaddr *) &client_address, &client_address_size);
        if (client_socket == -1) {
            std::cerr << "Error accepting connection" << std::endl;
            return 1;
        }

        timeval tv = {.tv_sec = timeout, .tv_usec = 0};
        if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
            std::cerr << "Error setting timeout" << std::endl;
            return 1;
        }
        if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
            std::cerr << "Error setting SIGPIPE handler" << std::endl;
            return 1;
        }

        auto conn = new utils::Connection(client_socket, my_addr, utils::addr2str(reinterpret_cast<sockaddr *>(&client_address), client_address_size));
        conn->should_log = true;

        auto thread = std::thread([&server, conn] {
            server.handle_new(conn);
        });
        thread.detach();
    }
    return 0;
}