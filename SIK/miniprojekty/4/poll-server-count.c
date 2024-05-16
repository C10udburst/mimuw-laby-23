#include <errno.h>
#include <endian.h>
#include <fcntl.h>
#include <inttypes.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>

#include "err.h"
#include "common.h"

#define BUFFER_SIZE   1024
#define QUEUE_LENGTH     5
#define TIMEOUT       5000
#define CONNECTIONS      3

// Added because VS Code does not recognize it.
#ifndef SA_RESTART
#define SA_RESTART	0x10000000
#endif

static bool finish = false;

/* Termination signal handling. */
static void catch_int(int sig) {
    finish = true;
    printf("signal %d catched so no new connections will be accepted\n", sig);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fatal("usage: %s <port> <control-port>", argv[0]);
    }

    install_signal_handler(SIGINT, catch_int, SA_RESTART);

    uint16_t port = read_port(argv[1]);
    uint16_t control_port = read_port(argv[2]);

    // Create main socket.
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        syserr("cannot create a socket");
    }

    // Create control socket.
    int control_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (control_socket_fd < 0) {
        syserr("cannot create a control socket");
    }

    // Bind the socket to a concrete address.
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // Listening on all interfaces.
    server_address.sin_port = htons(port);

    if (bind(socket_fd, (struct sockaddr *) &server_address, (socklen_t) sizeof server_address) < 0) {
        syserr("bind");
    }
    
    // Bind the control socket to a concrete address.
    struct sockaddr_in control_server_address;
    control_server_address.sin_family = AF_INET; // IPv4
    control_server_address.sin_addr.s_addr = htonl(INADDR_ANY); // Listening on all interfaces.
    control_server_address.sin_port = htons(control_port);

    if (bind(control_socket_fd, (struct sockaddr *) &control_server_address, (socklen_t) sizeof control_server_address) < 0) {
        syserr("bind");
    }

    // Switch the socket to listening.
    if (listen(socket_fd, QUEUE_LENGTH) < 0) {
        syserr("listen");
    }

    // Switch the control socket to listening.
    if (listen(control_socket_fd, QUEUE_LENGTH) < 0) {
        syserr("listen");
    }

    // Find out what port the server is actually listening on.
    socklen_t lenght = (socklen_t) sizeof server_address;
    if (getsockname(socket_fd, (struct sockaddr *) &server_address, &lenght) < 0) {
        syserr("getsockname");
    }

    printf("listening on port %" PRIu16 "\n", ntohs(server_address.sin_port));

    // Initialization of pollfd structures.
    struct pollfd poll_descriptors[CONNECTIONS + 2];

    // The main socket has index 0.
    // The control socket has index 1.
    // the control client has index 2.
    poll_descriptors[0].fd = socket_fd;
    poll_descriptors[0].events = POLLIN;
    poll_descriptors[1].fd = control_socket_fd;
    poll_descriptors[1].events = POLLIN;

    bool control_client = false;

    for (int i = 2; i < CONNECTIONS + 2; ++i) {
        poll_descriptors[i].fd = -1;
        poll_descriptors[i].events = POLLIN;
    }
    size_t active_clients = 0;
    static char buffer[BUFFER_SIZE];
    struct sockaddr_in client_address;

    do {
        for (int i = 0; i < CONNECTIONS + 2; ++i) {
            poll_descriptors[i].revents = 0;
        }

        // After Ctrl-C the main socket and control socket are closed.
        if (finish && poll_descriptors[0].fd >= 0) {
            close(poll_descriptors[0].fd);
            poll_descriptors[0].fd = -1;
        }
        if (finish && poll_descriptors[1].fd >= 0) {
            close(poll_descriptors[1].fd);
            poll_descriptors[1].fd = -1;
        }

        int poll_status = poll(poll_descriptors, CONNECTIONS, TIMEOUT);
        if (poll_status == -1 ) {
            if (errno == EINTR) {
                printf("interrupted system call\n");
            }
            else {
                syserr("poll");
            }
        }
        else if (poll_status > 0) {
            if (!finish && (poll_descriptors[0].revents & POLLIN)) {
                // New connection: new client is accepted.
                int client_fd = accept(poll_descriptors[0].fd,
                                       (struct sockaddr *) &client_address,
                                       &((socklen_t){sizeof client_address}));
                if (client_fd < 0) {
                    syserr("accept");
                }

                // Searching for a free slot.
                bool accepted = false;
                for (int i = 3; i < CONNECTIONS + 2; ++i) {
                    if (poll_descriptors[i].fd == -1) {
                        printf("received new connection (%d)\n", i);
                        poll_descriptors[i].fd = client_fd;
                        poll_descriptors[i].events = POLLIN;
                        active_clients++;
                        accepted = true;
                        break;
                    }
                }
                if (!accepted) {
                    close(client_fd);
                    printf("too many clients\n");
                }
                else {
                    char const *client_ip = inet_ntoa(client_address.sin_addr);
                    uint16_t client_port = ntohs(client_address.sin_port);
                    printf("accepted connection from %s:%" PRIu16 "\n",
                           client_ip, client_port);
                }
            }
            if (!finish && (poll_descriptors[1].revents & POLLIN)) {
                // New connection to control socket: new client is accepted.
                int client_fd = accept(poll_descriptors[1].fd,
                                       (struct sockaddr *) &client_address,
                                       &((socklen_t){sizeof client_address}));
                if (client_fd < 0) {
                    syserr("accept");
                }

                if (control_client) {
                    printf("control client already connected\n");
                    close(client_fd);
                } else {
                    control_client = true;
                    poll_descriptors[2].fd = client_fd;
                    poll_descriptors[2].events = POLLIN;
                    printf("accepted control connection\n");
                }
            }
            // Serve data connections.
            for (int i = 3; i < CONNECTIONS + 2; ++i) {
                if (poll_descriptors[i].fd != -1 && (poll_descriptors[i].revents & (POLLIN | POLLERR))) {

                    ssize_t received_bytes = read(poll_descriptors[i].fd, buffer, BUFFER_SIZE);

                    if (received_bytes < 0) {
                        error("error when reading message from connection %d", i);
                        close(poll_descriptors[i].fd);
                        poll_descriptors[i].fd = -1;
                        active_clients--;
                    } else if (received_bytes == 0) {
                        printf("ending connection (%d)\n", i);
                        close(poll_descriptors[i].fd);
                        poll_descriptors[i].fd = -1;
                        active_clients--;
                    } else {
                        printf("received %zd bytes within connection (%d): '%.*s'\n",
                        received_bytes, i, (int) received_bytes, buffer);
                    }
                }
            }
            // Serve control connection.
            if (control_client && (poll_descriptors[2].revents & (POLLIN | POLLERR))) {
                ssize_t received_bytes = read(poll_descriptors[2].fd, buffer, BUFFER_SIZE);

                if (received_bytes < 0) {
                    error("error when reading message from control connection");
                    close(poll_descriptors[2].fd);
                    poll_descriptors[2].fd = -1;
                    control_client = false;
                } else if (received_bytes == 0) {
                    printf("ending control connection\n");
                    close(poll_descriptors[2].fd);
                    poll_descriptors[2].fd = -1;
                    control_client = false;
                } else {
                    if (strncmp(buffer, "count", 5) == 0) {
                        printf("received 'count' command\n");
                        char response[64];
                        int response_length = snprintf(response, sizeof response, "number of active clients: %zu\ntotal number of clients: %d\n",
                            active_clients, CONNECTIONS);
                        if (write(poll_descriptors[2].fd, response, response_length) != response_length) {
                            error("error when writing response to control connection");
                        }
                    } else {
                        printf("received unknown command\n");
                    }
                    // Close the connection after receiving the command.
                    close(poll_descriptors[2].fd);
                    poll_descriptors[2].fd = -1;
                    control_client = false;
                }
            }
        } else {
            printf("%d milliseconds passed without any events\n", TIMEOUT);
        }
    } while (!finish || active_clients > 0 || control_client);

    if (poll_descriptors[0].fd >= 0) {
        close(poll_descriptors[0].fd);
    }
    if (poll_descriptors[1].fd >= 0) {
        close(poll_descriptors[1].fd);
    }
}
