#include <arpa/inet.h>
#include <sys/poll.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <regex>
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

    auto player = client::Player{};

    auto line = conn.readline();
    if (!automatic)
        std::cout << line;
    if (line.starts_with('B')) {
        if (!automatic) {
            auto ss = utils::parseline(line, "BUSY");
            std::string busylist;
            ss >> busylist;
            std::cout << "Place taken, list of taken places received: ";
            client::print_list(busylist);
            std::cout << "." << std::endl;
        }
        return 1;
    }

    pollfd pfd[2];
    pfd[0].fd = conn.fd;
    pfd[0].events = POLLIN;
    pfd[1].fd = STDIN_FILENO;
    pfd[1].events = automatic ? 0 : POLLIN;

    while (true) {
        try {
            auto ss = utils::parseline(line, "DEAL");
            client::Deal deal;
            ss >> deal;
            player.hand = deal.cards;
            if (!automatic) {
                std::cout << "New deal " << deal.rule << ": staring place " << deal.first_player << ", your cards: ";
                client::print_list(player.hand);
                std::cout << "." << std::endl;
            }
            while (true) {
                if (poll(pfd, 2, -1) == -1) {
                    std::cerr << "poll failed: " << strerror(errno) << std::endl;
                    return 1;
                }
                if (pfd[0].revents & POLLIN) {
                    line = conn.readline();
                    if (!automatic)
                        std::cout << line;
                    if (line.starts_with("WR") && !automatic) {
                        auto ss2 = utils::parseline(line, "WRONG");
                        int draw;
                        ss2 >> draw;
                        std::cout << "Wrong message received in trick " << draw << "." << std::endl;
                    }
                    if (line.starts_with("DE")) {
                        player.taken.clear();
                        player.taken.reserve(13);
                        break;
                    }
                    if (line.starts_with("TR")) {
                        client::Trick trick;
                        trick.from_string(line);
                        player.trick_id = trick.draw;
                        if (!automatic) {
                            std::cout << "Trick: (" << trick.draw << ") ";
                            client::print_list(trick.table);
                            std::cout << std::endl;
                            std::cout << "Available: ";
                            client::print_list(player.hand);
                            std::cout << std::endl;
                        } else {
                            conn.writeline("TRICK" + std::to_string(trick.draw) +
                                           client::AutoPlayer::play(trick.table, player).to_string() + "\r\n");
                        }
                    }
                    if (line.starts_with("TA")) {
                        client::Taken taken;
                        taken.from_string(line);
                        for (auto &table_card: taken.table) {
                            for (auto &hand_card: player.hand) {
                                if (table_card == hand_card) {
                                    hand_card.marknull();
                                    goto break_for_taken_table;
                                }
                            }
                        }
                        break_for_taken_table:
                        if (!automatic) {
                            std::cout << "A trick " << taken.draw << " is taken by " << taken.loser << ", cards: ";
                            client::print_list(taken.table);
                            std::cout << "." << std::endl;
                        }
                        if (taken.loser == seat) {
                            player.taken.push_back(taken.table);
                        }
                    }
                    if (line.starts_with("SC") && !automatic) {
                        client::Scoring sc;
                        sc.from_string(line, client::SCORE);
                        std::cout << "The scores are:" << std::endl;
                        for (auto &score: sc.scores) {
                            std::cout << score.place << " | " << score.score << std::endl;
                        }
                    }
                    if (line.starts_with("TO") && !automatic) {
                        client::Scoring sc;
                        sc.from_string(line, client::TOTAL);
                        std::cout << "The total scores are:" << std::endl;
                        for (auto &score: sc.scores) {
                            std::cout << score.place << " | " << score.score << std::endl;
                        }
                        return 0;
                    }
                    continue;
                }

                if (pfd[1].revents & POLLIN) {
                    auto line2 = utils::readline(STDIN_FILENO, sizeof "!3C\n" - 1);
                    auto card = client::HumanPlayer::parse_cmd(line2, player);
                    if (!card.isnull()) {
                        conn.writeline("TRICK" + std::to_string(player.trick_id) + card.to_string() + "\r\n");
                    }
                }
            }
        } catch (errors::ConnClosedError &e) {
            if (!line.starts_with("TOTAL")) {
                std::cerr << "Connection closed by the server" << std::endl;
                return 1;
            }
            return 0;
        }
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