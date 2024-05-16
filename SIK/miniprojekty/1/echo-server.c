#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "err.h"

static uint16_t read_port(char const *string) {
    char *endptr;
    unsigned long port = strtoul(string, &endptr, 10);
    if ((port == ULONG_MAX && errno == ERANGE) || *endptr != 0 || port == 0 || port > UINT16_MAX) {
        fatal("%s is not a valid port number", string);
    }
    return (uint16_t) port;
}

static uint64_t read_uint64(char const *string) {
    char *endptr;
    unsigned long long value = strtoull(string, &endptr, 10);
    if ((value == ULLONG_MAX && errno == ERANGE) || *endptr != 0) {
        fatal("%s is not a valid uint64 number", string);
    }
    return (uint64_t) value;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fatal("usage: %s <port> <buffer_size>", argv[0]);
    }

    uint16_t port = read_port(argv[1]);
    uint64_t buffer_size = read_uint64(argv[2]);

    // Create a socket. Buffer should not be allocated on the stack.
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
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

    printf("listening on port %" PRIu16 "\n", port);

    ssize_t received_length;
    char *buffer = malloc(buffer_size);
    do {
        // Receive a message. Buffer should not be allocated on the stack.
        memset(buffer, 0, buffer_size); // Clean the buffer.

        int flags = 0;
        struct sockaddr_in client_address;
        socklen_t address_length = (socklen_t) sizeof(client_address);

        received_length = recvfrom(socket_fd, buffer, buffer_size, flags,
                                   (struct sockaddr *) &client_address, &address_length);
        if (received_length < 0) {
            syserr("recvfrom");
        }

        char const *client_ip = inet_ntoa(client_address.sin_addr);
        uint16_t client_port = ntohs(client_address.sin_port);
        printf("received %zd bytes from %s:%" PRIu16 "\n",
               received_length, client_ip, client_port);
    } while (received_length > 0);

    printf("exchange finished\n");

    close(socket_fd);
    return 0;
}
