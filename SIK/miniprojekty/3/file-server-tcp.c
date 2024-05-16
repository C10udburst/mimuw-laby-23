#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <endian.h>
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>

#include "err.h"
#include "common.h"
#include "protocol.h"

#define MIN(a,b) (((a)<(b))?(a):(b))

#define NAME_MAX 255
#define SLEEP_TIME 1
#define BUF_SIZE 4096
#define QUEUE_LENGTH 5

struct conn_args {
    int client_fd;
    struct sockaddr_in client_address;
    atomic_uint_least64_t* total_size;
};

void* handle_connection(void *conn_args_ptr) {
    struct conn_args args;
    memcpy(&args, conn_args_ptr, sizeof(args));
    free(conn_args_ptr);

    data_header header;
    if (readn(args.client_fd, &header, sizeof(header)) < 0) {
        error("read header");
        close(args.client_fd);
        return NULL;
    }

    header.name_length = be16toh(header.name_length);
    header.file_length = be32toh(header.file_length);

    if (header.name_length > NAME_MAX || header.name_length == 0) {
        error("invalid name length");
        close(args.client_fd);
        return NULL;
    }

    char* fname = malloc(header.name_length + 1);
    fname[header.name_length] = '\0';

    if (readn(args.client_fd, fname, header.name_length) < 0) {
        error("read name");
        free(fname);
        close(args.client_fd);
        return NULL;
    }

    char const *client_ip = inet_ntoa(args.client_address.sin_addr);
    uint16_t client_port = ntohs(args.client_address.sin_port);
    printf("new client [%s:%" PRIu16 "] size=%" PRIu32 " file=%s\n", client_ip, client_port, header.file_length, fname);

    sleep(SLEEP_TIME);

    FILE* file = fopen(fname, "wb");
    if (file == NULL) {
        error("open file");
        free(fname);
        close(args.client_fd);
        return NULL;
    }

    char buf[BUF_SIZE];
    uint64_t remaining = header.file_length;
    ssize_t nread;
    while (remaining > 0) {
        if ((nread = read(args.client_fd, buf, MIN(remaining, BUF_SIZE))) < 0) {
            error("read file");
            fclose(file);
            free(fname);
            close(args.client_fd);
            return NULL;
        }
        else if (nread == 0)
            break;            // EOF
        else {
            if (fwrite(buf, 1, nread, file) < nread) {
                error("write file");
                fclose(file);
                free(fname);
                close(args.client_fd);
                return NULL;
            }
            remaining -= nread;
        }
    }

    fclose(file);

    printf("client [%s:%" PRIu16 "] has sent its file of size=%" PRIu32 "\n", client_ip, client_port, header.file_length);
    uint64_t total_size = atomic_fetch_add(args.total_size, header.file_length) + header.file_length;
    printf("total size of uploaded files %" PRIu64 "\n", total_size);

    free(fname);
    close(args.client_fd);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fatal("usage: %s <port>", argv[0]);
    }

    uint16_t port = read_port(argv[1]);

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

    if (bind(socket_fd, (struct sockaddr *) &server_address, (socklen_t) sizeof server_address) < 0) {
        syserr("bind");
    }

    // Switch the socket to listening.
    if (listen(socket_fd, QUEUE_LENGTH) < 0) {
        syserr("listen");
    }

    // Find out what port the server is actually listening on.
    socklen_t lenght = (socklen_t) sizeof server_address;
    if (getsockname(socket_fd, (struct sockaddr *) &server_address, &lenght) < 0) {
        syserr("getsockname");
    }

    //printf("parent is listening on port %" PRIu16 "\n", ntohs(server_address.sin_port));


    atomic_uint_least64_t total_size = 0;
    for (;;) {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        int client_fd = accept(socket_fd, (struct sockaddr *) &client_address, &client_address_len);
        if (client_fd < 0) {
            syserr("accept");
        }

        struct conn_args* args = malloc(sizeof(struct conn_args));
        if (args == NULL) {
            fatal("malloc");
        }
        args->client_fd = client_fd;
        args->client_address = client_address;
        args->total_size = &total_size;

        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_connection, args) != 0) {
            syserr("pthread_create");
        }
        pthread_detach(thread);
    }

    close(socket_fd);

    return 0;
}