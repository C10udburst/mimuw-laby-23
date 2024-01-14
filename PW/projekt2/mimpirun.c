/**
 * This file is for implementation of mimpirun program.
 * */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include "mimpi_common.h"
#include "channel.h"

#define CH_READ(p1, p2) (160 + p1 * (18*2) + p2 * 2)
#define CH_WRITE(p2, p1) (160 + p1 * (18*2) + p2 * 2 + 1)

atomic_bool deadlock_running = true;

#define SEND_DEADLOCK(to, cause)                                   \
    do {                                                           \
        int res = chsend(CH_WRITE(to, 17), &cause, sizeof(cause)); \
        if (!(res < 0 && (errno == EPIPE || errno == EBADF)))      \
            ASSERT_SYS_OK(res); \
    } while(0)

void* deadlock_thread(void* wtp) {
    int* wait_table = (int*) wtp;
    wait_packet_t wait_packet;
    while (atomic_load(&deadlock_running)) {
        int res = chrecv(60, &wait_packet, sizeof(wait_packet));
        if ((res < 0 && errno == EPIPE) || res == 0)
            continue;
        ASSERT_SYS_OK(res);
        //printf("deadlock: %d waits for %d\n", wait_packet.my_rank, wait_packet.sender_rank);
        //fflush(stdout);
        wait_table[wait_packet.my_rank] = wait_packet.sender_rank;
        if (wait_packet.sender_rank == DEADLOCK_FINISHED) {
            wait_table[wait_packet.my_rank] = DEADLOCK_NO_WAIT;
            SEND_DEADLOCK(wait_packet.my_rank, wait_packet.sender_rank);
            continue;
        }
        if (wait_packet.sender_rank != DEADLOCK_NO_WAIT && wait_table[wait_packet.sender_rank] == wait_packet.my_rank) {
            SEND_DEADLOCK(wait_packet.my_rank, wait_packet.sender_rank);
            SEND_DEADLOCK(wait_packet.sender_rank, wait_packet.my_rank);
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    int n = atoi(argv[1]);

    ASSERT_SYS_OK(setenv("MIMPI_N", argv[1], 1));

    // in child process:
    // write pipe to p_i is at fd: 20 + i
    // read pipe from p_i is at fd: 40 + i
    // 61 is write pipe to deadlock detector
    // 62 is read pipe from deadlock detector

    int fd[2];

    // pipes for inter-process communication
    for (int p1 = 0; p1 < n; p1++) {
        for (int p2 = 0; p2 < n; p2++) {
            if (p1 == p2) continue;
            ASSERT_SYS_OK(channel(fd));
            ASSERT_SYS_OK(dup2(fd[0], CH_READ(p1, p2))); // read from p2
            ASSERT_SYS_OK(dup2(fd[1], CH_WRITE(p2, p1))); // write to p1
            ASSERT_SYS_OK(close(fd[0]));
            ASSERT_SYS_OK(close(fd[1]));
        }
    }

    for (int p1 = 0; p1 < n; p1++) {
        ASSERT_SYS_OK(channel(fd));
        ASSERT_SYS_OK(dup2(fd[0], CH_READ(p1, 17))); // read from deadlock detector
        ASSERT_SYS_OK(dup2(fd[1], CH_WRITE(p1, 17))); // write to deadlock detector
        ASSERT_SYS_OK(close(fd[0]));
        ASSERT_SYS_OK(close(fd[1]));
    }

    ASSERT_SYS_OK(channel(fd));
    ASSERT_SYS_OK(dup2(fd[0], 60)); // read from deadlock detector
    ASSERT_SYS_OK(dup2(fd[1], WRITE_DEADLOCK)); // write to deadlock detector
    ASSERT_SYS_OK(close(fd[0]));
    ASSERT_SYS_OK(close(fd[1]));
    

    // start all children
    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        ASSERT_SYS_OK(pid);
        if (pid == 0) {
            for (int p1 = 0; p1 < n; p1++) {
                for (int p2 = 0; p2 < n; p2++) {
                    if (p1 == p2) continue;
                    if (p1 == i) {
                        ASSERT_SYS_OK(dup2(CH_READ(p1, p2), READ_PIPE(p2)));
                    } else if (p2 == i) {
                        ASSERT_SYS_OK(dup2(CH_WRITE(p2, p1), WRITE_PIPE(p1)));
                    }
                    ASSERT_SYS_OK(close(CH_READ(p1, p2)));
                    ASSERT_SYS_OK(close(CH_WRITE(p2, p1)));
                }
                if (p1 == i)
                    ASSERT_SYS_OK(dup2(CH_READ(p1, 17), READ_DEADLOCK));
                ASSERT_SYS_OK(close(CH_READ(p1, 17)));
                ASSERT_SYS_OK(close(CH_WRITE(p1, 17)));
            }
            ASSERT_SYS_OK(close(60));
            sprintf(argv[1], "%d", i);
            ASSERT_SYS_OK(setenv("MIMPI_RANK", argv[1], 1));
            ASSERT_SYS_OK(execvp(argv[2], argv + 2));
        }
    }

    for (int p1 = 0; p1 < n; p1++) {
        for (int p2 = 0; p2 < n; p2++) {
            if (p1 == p2) continue;
            ASSERT_SYS_OK(close(CH_READ(p1, p2)));
            ASSERT_SYS_OK(close(CH_WRITE(p2, p1)));
        }
        ASSERT_SYS_OK(close(CH_READ(p1, 17)));
    }

    ASSERT_SYS_OK(close(61));

    // start deadlock detector
    channels_init();
    int* wait_table = malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++)
        wait_table[i] = DEADLOCK_NO_WAIT;
    pthread_t deadlock_detector;
    ASSERT_SYS_OK(pthread_create(&deadlock_detector, NULL, deadlock_thread, wait_table));

    while (wait(NULL) > 0); // wait for all children to finish

    atomic_store(&deadlock_running, false);
    
    pthread_join(deadlock_detector, NULL);
    free(wait_table);
    channels_finalize();

    ASSERT_SYS_OK(close(60));
    for (int p1 = 0; p1 < n; p1++) {
        ASSERT_SYS_OK(close(CH_WRITE(p1, 17)));
    }

    return 0;
}