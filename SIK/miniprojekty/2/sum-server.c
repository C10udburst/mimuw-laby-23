#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <endian.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sum-common.h"
#include "err.h"
#include "common.h"

#define QUEUE_LENGTH  5
#define SOCK_TIMEOUT  4

int main(int argc, char *argv[]) {
    if (argc != 2 + 1) {
        fatal("usage: %s <port> <k>", argv[0]);
    }

    uint16_t port = read_port(argv[1]);
    uint64_t k = read_uint64(argv[2]);

    // Ignore SIGPIPE signals, so they are delivered as normal errors.
    signal(SIGPIPE, SIG_IGN);

    // Create a socket.
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        syserr("cannot create a socket");
    }

    // Bind the socket to a concrete address.
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // Listening on all interfaces.
    server_address.sin_port = htons(port);

    if (bind(socket_fd, (struct sockaddr *) &server_address, (socklen_t) sizeof(server_address)) < 0) {
        syserr("bind");
    }

    // Switch the socket to listening.
    if (listen(socket_fd, QUEUE_LENGTH) < 0) {
        syserr("listen");
    }

    printf("listening on port %" PRIu16 "\n", port);

    void* buffer = malloc(k);

    for (;;) {
        struct sockaddr_in client_address;

        int client_fd = accept(socket_fd, (struct sockaddr *) &client_address,
                               &((socklen_t){sizeof(client_address)}));
        if (client_fd < 0) {
            syserr("accept");
        }

        char const *client_ip = inet_ntoa(client_address.sin_addr);
        uint16_t client_port = ntohs(client_address.sin_port);
        printf("accepted connection from %s:%" PRIu16 "\n", client_ip, client_port);

        // Set timeouts for the client socket.
        struct timeval to = {.tv_sec = SOCK_TIMEOUT, .tv_usec = 0};
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof to);
        setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &to, sizeof to);

        uint64_t received_packets = 0;
        uint64_t received_bytes = 0;
        printf("receiving packets from %s:%" PRIu16 " ", client_ip, client_port);

        clock_t start = clock();
        
        ssize_t read_length;
        do {
            read_length = readn(client_fd, buffer, k);
            if (read_length < 0) {
                if (errno == EAGAIN) {
                    printf("\rtimeout\n");
                }
                else {
                    error("readn");
                    continue;
                }
                
            }
            
            if (read_length > 0)
                received_packets++;
            received_bytes += read_length;
        } while (read_length != 0);

        clock_t end = clock();

        double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

        printf("finished serving %s:%" PRIu16 "\n", client_ip, client_port);
        printf("received %lu packets, %lu bytes in %.2f seconds\n", received_packets, received_bytes, time_spent);
        close(client_fd);
    }
    close(socket_fd);
    return 0;
}
