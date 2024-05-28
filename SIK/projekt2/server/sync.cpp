#include <sys/poll.h>
#include <cstring>
#include "sync.h"
#include "../common/errors.h"

#define Barrier 0
#define Semaphore 1

server::Sync::Sync() {
    for (auto &type: pipefd) {
        for (auto &i : type) {
            if (pipe(i))
                throw errors::MainError("pipe failed: " + std::string(strerror(errno)));
        }
        }
}

server::Sync::~Sync() {
    for (auto &type: pipefd) {
        for (auto &i: type) {
            close(i[0]);
            close(i[1]);
        }
    }
}

void server::Sync::wait(const server::Client& client, int type) const {
    // TODO: check if this is fucked or fine
    pollfd pfd[2];
    /*
     * pfd[0] is the pipe for the client
     * pfd[1] is the tcp connection
     * if tcp sends data before the pipe, we send wrong to the client
     */
    pfd[0].fd = pipefd[type][client.seat][0];
    pfd[0].events = POLLIN;
    pfd[1].fd = client.connection->fd;
    pfd[1].events = POLLIN;
    while (true) {
        for (auto &i : pfd)
            i.revents = 0;
        if (poll(pfd, 2, -1) == -1)
            throw errors::MainError("poll failed: " + std::string(strerror(errno)));
        if (pfd[0].revents & POLLIN) {
            char c;
            if (read(pfd[0].fd, &c, 1) == -1)
                throw errors::MainError("read failed: " + std::string(strerror(errno)));
            break;
        }
        if (pfd[1].revents & POLLIN) {
            auto line = client.connection->readline();
            if (line.starts_with("TRICK"))
                client.connection->writeline("WRONG" + std::to_string(client.current_draw) + "\r\n");
        }
    }
}

void server::Sync::barrier(const server::Client& client) const {
    for (auto i : pipefd[Barrier]) {
        if (write(i[1], "B", 1) == -1)
            throw errors::MainError("write failed: " + std::string(strerror(errno)));
    }
    for (int i = 0; i < 4; i++)
        wait(client, Barrier);
}

void server::Sync::sem_wake(int seat) const {
    if (write(pipefd[Semaphore][seat][1], "S", 1) == -1)
        throw errors::MainError("write failed: " + std::string(strerror(errno)));
}

void server::Sync::sem_sleep(const server::Client& client) const {
    wait(client, Semaphore);
}
