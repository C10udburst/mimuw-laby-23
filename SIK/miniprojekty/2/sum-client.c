#include <endian.h>
#include <inttypes.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sum-common.h"
#include "err.h"
#include "common.h"

int main(int argc, char *argv[]) {
    if (argc != 3 + 2) {
        fatal("usage: %s <host> <port> <n> <k>", argv[0]);
    }

    char const *host = argv[1];
    uint16_t port = read_port(argv[2]);
    struct sockaddr_in server_address = get_server_address(host, port);
    uint64_t n = read_uint64(argv[3]);
    uint64_t k = read_uint64(argv[4]);

    // Create a socket.
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        syserr("cannot create a socket");
    }

    void* data_to_send = malloc(k);

    clock_t start = clock();

    // Connect to the server.
    if (connect(socket_fd, (struct sockaddr *) &server_address,
                (socklen_t) sizeof(server_address)) < 0) {
        syserr("cannot connect to the server");
    }

    // CONN_SLEEP env var

    char* conn_sleep_env = getenv("CONN_SLEEP");
    if (conn_sleep_env == NULL) {
        conn_sleep_env = "0";
    }
    uint64_t conn_sleep = read_uint64(conn_sleep_env);

    if (conn_sleep > 0) {
        printf("sleeping for %" PRIu64 " seconds\n", conn_sleep);
        sleep(conn_sleep);
    }

    printf("sending %lu packets of size %lu to %s:%" PRIu16 " ", n, k, host, port);
    while(n--) {
        printf("\rsending packets of size %lu to %s:%" PRIu16 " %lu remaining    ", k, host, port, n);
        memset(data_to_send, 'a' + (n%26), k);

        ssize_t written_length = writen(socket_fd, data_to_send, k);
        if (written_length < 0) {
            syserr("writen");
        }
        // Here we can convert the type, because we know, that written_length >= 0.
        else if ((size_t) written_length != k) {
            fatal("incomplete writen");
        }
        
    }
    printf("\n");

    clock_t end = clock();

    double elapsed_time = (double) (end - start) / CLOCKS_PER_SEC;

    printf("sent packets of size %lu to %s:%" PRIu16 " in %.2f seconds\n", k, host, port, elapsed_time);

    close(socket_fd);
    return 0;
}
