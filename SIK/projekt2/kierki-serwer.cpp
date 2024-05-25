#include "common.h"
#include <getopt.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <atomic>
#include <barrier>
#include <thread>
#include <sstream>
#include <vector>
#include <fstream>
#include <semaphore>
#include <string.h>
#include <poll.h>

struct Trick {
    kierki::Rule rule;
    int8_t first_seat;
    kierki::Card cards[4][13];

public:
    friend std::istream &operator>>(std::istream &is, Trick &trick);
};

enum Taken : char {
    False = 0,
    Busy = 1,
    Missing = 2 // the previous client disconnected
};

void handle_new_connection(kierki::Connection conn);
void handle_client(const kierki::Connection& conn, int seat, char taken);

std::atomic_char taken[4] = {Taken::False, Taken::False, Taken::False, Taken::False};
std::atomic_int currentTrick = 0;
std::barrier synchronizer(4);
int wake_pipes[4][2]; // wake_pipe[i][0] - read by i-th thread, wake_pipe[i][1] - write to wake i-th thread
kierki::Card table[4];

std::vector<Trick> tricks;

int main(int argc, char *argv[]) {

    u_int16_t port = 0;
    char* filepath = nullptr;
    __time_t timeout = 5;

    {  // read arguments
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

    { // read tricks from file
        std::ifstream file(filepath);
        if (file.is_open()) {
            while (!file.eof()) {
                Trick trick{};
                file >> trick;
                if (trick.first_seat == '!')
                    break;
                tricks.push_back(trick);
            }
            file.close();
        } else {
            std::cerr << "Failed to open the file" << std::endl;
            return 1;
        }
    }

    { // setup semaphores
        for (int i = 0; i < 4; i++) {
            if (pipe(wake_pipes[i]) < 0) {
                std::cerr << "Error creating pipe: " << strerror(errno) << std::endl;
                return 1;
            }
        }
    }
    
    // start tcp server
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port); // if port is 0, the system will assign a random port
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (sockaddr *) &server_address, sizeof(server_address)) == -1) {
        std::cerr << "Error binding socket" << std::endl;
        return 1;
    }

    if (listen(server_socket, 5) == -1) {
        std::cerr << "Error listening on socket" << std::endl;
        return 1;
    }

    auto my_addr = kierki::Addr{
        .addr = server_address,
        .addr_size = sizeof(server_address)
    };

    while (true) {
        sockaddr_in client_address{};
        socklen_t client_address_size = sizeof(client_address);
        int client_socket = accept(server_socket, (sockaddr *) &client_address, &client_address_size);
        if (client_socket == -1) {
            std::cerr << "Error accepting connection" << std::endl;
            return 1;
        }

        // create conn object
        auto conn = kierki::Connection{
            .fd = client_socket,
            .my_addr = my_addr,
            .their_addr = kierki::Addr{
                .addr = client_address,
                .addr_size = client_address_size
            },
            .log_cout = true
        };

        conn.init(timeout);

        // create new thread to handle connection
        auto conn_thread = std::thread(handle_new_connection, conn);
        conn_thread.detach();
    }
}

void handle_new_connection(kierki::Connection conn) {
    try {
        // IAM<miejsce przy stole>\r\n
        auto line = conn.readline();
        if (line.rfind("IAM", 0) != 0 || line.size() != (3+1+2)) {
            conn.kill();
            return;
        }

        auto seat = kierki::seat2int(line[3]);
        if (seat == -1) {
            conn.kill();
            return;
        }

        // check if seat is free, if not, send BUSY
        auto is_taken = taken[seat].exchange(Taken::Busy);
        if (is_taken == Taken::Busy) {
            // send BUSY<taken seats>\r\n
            std::stringstream ss;
            ss << "BUSY";
            for (int i = 0; i < 4; i++) {
                if (taken[i]) {
                    char c = kierki::int2seat[i];
                    ss << c;
                }
            }
            ss << "\r\n";
            conn.writeline(ss.str());
            conn.kill();
            return;
        } else {
            handle_client(conn, seat, is_taken);
        }
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    } 
}

void reset_table() {
    for (auto &card : table)
        card.marknull();
}

void handle_client(const kierki::Connection& conn, int seat, char is_taken) {
    try {
        for (int trick = currentTrick; trick < tricks.size(); trick++) {
            if (is_taken != Taken::Missing) {
                synchronizer.arrive_and_wait();
                currentTrick.store(trick);
            }
            std::stringstream ss;
            ss << "DEAL";
            ss << static_cast<int>(tricks[trick].rule);
            ss << kierki::int2seat[tricks[trick].first_seat];
            for (auto &card: tricks[trick].cards[seat])
                ss << card.to_string();
            ss << "\r\n";
            conn.writeline(ss.str());
            auto real_seat = (seat - tricks[trick].first_seat + 4) % 4;
            for (int card = 0; card < 13; card++) {
                if (real_seat > 0) {
                    // 0 - wake from other thread, 1 - wrong message from tcp
                    pollfd poll_fd[2] = {
                        {.fd = wake_pipes[real_seat][0], .events = POLLIN, .revents = 0},
                        {.fd = conn.fd, .events = POLLIN, .revents = 0}
                    };
                    while (true) {
                        for (auto& pf: poll_fd)
                            pf.revents = 0;
                        poll(poll_fd, 2, -1);
                        if (poll_fd[0].revents & POLLIN) {
                            char c;
                            read(wake_pipes[real_seat][0], &c, 1);
                            break;
                        }
                        if (poll_fd[1].revents & POLLIN) {
                            auto _ = conn.readline(); // ignore
                            ss.str("");
                            ss << "WRONG" << card << "\r\n";
                            conn.writeline(ss.str());
                        }
                    }
                }
                   
                else
                    reset_table();
                do {
                    try {
                        { // send table state
                            ss.str("");
                            ss << "TRICK" << card;
                            for (auto i: table)
                                ss << i.to_string();
                            ss << "\r\n";
                            conn.writeline(ss.str());
                        }
                        auto line = conn.readline();
                        kierki::Card played_card{};
                        ss.str("");
                        ss << "TRICK" << card;
                        if (line.rfind(ss.str(), 0) != 0)
                            goto invalid_input;
                        line = line.substr(ss.str().size());
                        ss.str(line);
                        ss >> played_card;
                        for (auto my_card: tricks[trick].cards[seat]) {
                            if (my_card == played_card) {
                                my_card.marknull();
                                goto found_card;
                            }
                        }
                        goto invalid_input;
                    found_card:
                        table[real_seat] = played_card;
                        break;
                    invalid_input:
                        ss.str("");
                        ss << "WRONG" << card << "\r\n";
                        conn.writeline(ss.str());
                    } catch (errors::TimeoutError &_) { }
                } while (true);

                if (real_seat < 3) {
                    write(wake_pipes[real_seat + 1][1], "a", 1);
                }
                synchronizer.arrive_and_wait();
            }
        }
    } catch (errors::ConnClosedError& _) {
        // we lost connection, allow new client to connect
        taken[seat].store(Taken::Missing);
    }

    conn.kill();
}

std::istream &operator>>(std::istream &is, Trick &trick) {
    char type;
    is.read(&type, 1);
    if (type < '1' || type > '7') {
        trick.first_seat = '!'; // indicate error
        return is;
    }
    trick.rule = static_cast<kierki::Rule>(type - '0');

    char first_seat;
    is.read(&first_seat, 1);
    trick.first_seat = kierki::seat2int(first_seat);

    is.ignore(1); // ignore \n

    for (auto & player : trick.cards) {
        for (auto & card : player)
            is >> card;
        is.ignore(1); // ignore \n
    }
    return is;
}
