#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include "cb.h"
#include "err.h"

#define MAX_HOSTNAME_LEN  255
#define MAX_PORT_LEN      5

char *progName = NULL;

#define MAX_MESSAGE_SIZE 2048
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static void helpAndExit()
{
    fatal("invocation: %s UDPaddress UDPport TCPport", progName);
}

struct sockaddr_and_len {
    socklen_t addr_len;
    struct sockaddr_storage addr;
};

static struct sockaddr_and_len _get_first_matching_addr(const char *hostStr,
                                                                                                                const char *portStr,
                                                                                                                int family, int flags)
{
    struct addrinfo *ai;
    struct addrinfo hints = {
            .ai_family = family, .ai_socktype = 0, .ai_protocol = 0, .ai_flags = flags};

    int gai_error;
    if ((gai_error = getaddrinfo(hostStr, portStr, &hints, &ai)) == -1) {
        fprintf(stderr, "getaddrinfo failed: %s", gai_strerror(gai_error));
        helpAndExit();
    }

    struct sockaddr_and_len ret;
    ret.addr_len = ai->ai_addrlen;
    *(struct sockaddr_in *)&(ret.addr) = *(struct sockaddr_in *)(ai->ai_addr);
    freeaddrinfo(ai);

    return ret;
}

static struct sockaddr_and_len parseHostAndPort(const char *hostStr, const char *portStr)
{
    return _get_first_matching_addr(hostStr, portStr, AF_INET, 0);
}

static struct sockaddr_and_len getListeningAddress(const char *portStr)
{
    return _get_first_matching_addr(NULL, portStr, AF_INET, AI_PASSIVE);
}

// Parametry: adres i port UDP są do rozgłaszania i słuchania,
// a port tcp do nasłuchiwania.
int main(int argc, char *argv[])
{
    progName = argv[0];
    if (argc != 4) {
        helpAndExit();
    }

    struct sockaddr_and_len udp_sending_address = parseHostAndPort(argv[1], argv[2]);
    struct sockaddr_and_len udp_listening_address = getListeningAddress(argv[2]);
    struct sockaddr_and_len tcp_address = getListeningAddress(argv[3]);
    const char *remote_dotted_address = argv[1];

    struct ip_mreq ip_mreq;
    ip_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (inet_aton(remote_dotted_address, &ip_mreq.imr_multiaddr) == 0) {
        fatal("inet_aton - invalid multicast address\n");
    }

    if (!IN_MULTICAST(ntohl(ip_mreq.imr_multiaddr.s_addr))) {
        fatal("not a multicast address\n");
    }


    int tcp_listen_sock;
    if ((tcp_listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        syserr("socket");
    }

    if (bind(tcp_listen_sock, (struct sockaddr *)&(tcp_address.addr), tcp_address.addr_len) < 0) {
        syserr("bind");
    }

    if (listen(tcp_listen_sock, 1) < 0) {
        syserr("bind");
    }

    struct sockaddr_storage peer;
    socklen_t peer_len = sizeof peer;
    int tcp_sock;
    if ((tcp_sock = accept(tcp_listen_sock, (struct sockaddr *)&peer, &peer_len)) < 0) {
        syserr("accept");
    }

    char peer_addr_str[MAX_HOSTNAME_LEN + 1];
    char peer_port_str[MAX_PORT_LEN + 1];
    getnameinfo((struct sockaddr *)&peer, peer_len, peer_addr_str, (socklen_t)(sizeof peer_addr_str),
                            peer_port_str, (socklen_t)(sizeof peer_port_str), NI_NUMERICHOST | NI_NUMERICSERV);
    printf("TCP connection to %s:%s established.\n", peer_addr_str, peer_port_str);

    close(tcp_listen_sock);

    /*
        0 - tcp read
        1 - udp read
        2 - tcp write
        3 - udp write
    */
    struct pollfd poll_descriptors[4];
    poll_descriptors[0].fd = tcp_sock;
    poll_descriptors[0].events = POLLIN;
    poll_descriptors[1].fd = -1;
    poll_descriptors[1].events = POLLIN;
    poll_descriptors[2].fd = -1;
    poll_descriptors[2].events = 0;
    poll_descriptors[3].fd = -1;
    poll_descriptors[3].events = 0;

    // Create a socket.
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        syserr("cannot create a socket");
    }

    // Join the multicast group.
    if (setsockopt(socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *) &ip_mreq, sizeof ip_mreq) < 0) {
        syserr("cannot join the multicast group");
    }

    // prevent sending to ourselves
    int optval = 0;
    if (setsockopt(socket_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &optval, sizeof(optval)) < 0) {
        syserr("setsockopt");
    }

    // Bind the socket to a concrete address.
    if (bind(socket_fd, (struct sockaddr *) &udp_listening_address.addr, udp_listening_address.addr_len) < 0) {
        syserr("bind");
    }
    poll_descriptors[1].fd = socket_fd;
    poll_descriptors[1].events = POLLIN;

    CircularBuffer* udp2tcp = malloc(sizeof(CircularBuffer));
    CircularBuffer* tcp2udp = malloc(sizeof(CircularBuffer));
    cbInit(udp2tcp);
    cbInit(tcp2udp);

    char buffer[MAX_MESSAGE_SIZE + 2];

    do {
        for (int i = 0; i < 4; i++) {
            poll_descriptors[i].revents = 0;
        }

        if (poll(poll_descriptors, 4, -1) < 0) {
            break;
        }

        if (poll_descriptors[0].revents & POLLIN) {
            ssize_t len = read(poll_descriptors[0].fd, buffer, sizeof(buffer));
            if (len < 0) {
                printf("read error\n");
                break;
            } else if (len == 0) {
                break;
            } else {
                cbPushBack(tcp2udp, buffer, len);
                poll_descriptors[3].fd = socket_fd;
                poll_descriptors[3].events = POLLOUT;
            }
        }

        if (poll_descriptors[1].revents & POLLIN) {
            ssize_t len = read(poll_descriptors[1].fd, buffer, sizeof(buffer));
            if (len < 0) {
                printf("read error\n");
                break;
            } else {
                cbPushBack(udp2tcp, buffer, len);
                poll_descriptors[2].fd = tcp_sock;
                poll_descriptors[2].events = POLLOUT;
            }
        }

        if ((poll_descriptors[2].revents & POLLOUT) && cbGetContinuousCount(udp2tcp) > 0) {
            char* data = cbGetData(udp2tcp);
            size_t len = cbGetContinuousCount(udp2tcp);
            ssize_t sent = write(poll_descriptors[2].fd, data, MIN(len, MAX_MESSAGE_SIZE));
            if (sent < 0) {
                printf("write error\n");
                break;
            } else {
                cbDropFront(udp2tcp, sent);
                if (cbGetContinuousCount(udp2tcp) == 0) {
                    poll_descriptors[2].fd = -1;
                    poll_descriptors[2].events = 0;
                }
            }
        }

        if ((poll_descriptors[3].revents & POLLOUT) && cbGetContinuousCount(tcp2udp) > 0) {
            char* data = cbGetData(tcp2udp);
            size_t len = cbGetContinuousCount(tcp2udp);
            ssize_t sent = sendto(poll_descriptors[3].fd, data, MIN(len, MAX_MESSAGE_SIZE), 0, (struct sockaddr *) &udp_sending_address.addr, udp_sending_address.addr_len);
            if (sent < 0) {
                printf("sendto error\n");
                break;
            } else {
                cbDropFront(tcp2udp, sent);
                if (cbGetContinuousCount(tcp2udp) == 0) {
                    poll_descriptors[3].fd = -1;
                    poll_descriptors[3].events = 0;
                }
            }
        }

    } while(true);

    // leave the multicast group
    if (setsockopt(socket_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void *) &ip_mreq, sizeof ip_mreq) < 0) {
        syserr("cannot leave the multicast group");
    }

    close(socket_fd);
    close(tcp_sock);
    cbDestroy(udp2tcp);
    cbDestroy(tcp2udp);
    free(udp2tcp);
    free(tcp2udp);

    return 0;
}
