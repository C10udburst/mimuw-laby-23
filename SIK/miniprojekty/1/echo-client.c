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

#define BUFFER_SIZE 20

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

static struct sockaddr_in get_server_address(char const *host, uint16_t port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    struct addrinfo *address_result;
    int errcode = getaddrinfo(host, NULL, &hints, &address_result);
    if (errcode != 0) {
        fatal("getaddrinfo: %s", gai_strerror(errcode));
    }

    struct sockaddr_in send_address;
    send_address.sin_family = AF_INET;   // IPv4
    send_address.sin_addr.s_addr =       // IP address
            ((struct sockaddr_in *) (address_result->ai_addr))->sin_addr.s_addr;
    send_address.sin_port = htons(port); // port from the command line

    freeaddrinfo(address_result);

    return send_address;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fatal("usage: %s <host> <port> <n> <k>\n", argv[0]);
    }

    char const *host = argv[1];
    uint16_t port = read_port(argv[2]);

    uint64_t n = read_uint64(argv[3]);
    uint64_t k = read_uint64(argv[4]);

    struct sockaddr_in server_address = get_server_address(host, port);
    //char const *server_ip = inet_ntoa(server_address.sin_addr);
    //uint16_t server_port = ntohs(server_address.sin_port);

    // Create a socket.
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        syserr("cannot create a socket");
    }

    void* msg = malloc(k);
    printf("sending %lu packets of size %lu to %s:%" PRIu16 " ", n, k, host, port);
    while(n--) {
        printf("\rsending packets of size %lu to %s:%" PRIu16 " %lu remaining    ", k, host, port, n);
        memset(msg, 'a' + (n%26), k);
        // Send a message.
        size_t message_length = k;
        int send_flags = 0;
        socklen_t address_length = (socklen_t) sizeof(server_address);
        ssize_t sent_length = sendto(socket_fd, msg, message_length, send_flags,
                                     (struct sockaddr *) &server_address, address_length);
        if (sent_length < 0) {
            syserr("sendto");
        }
        else if (sent_length != (ssize_t) message_length) {
            fatal("incomplete sending");
        }
    }
    printf("\n");

    close(socket_fd);
    return 0;
}
