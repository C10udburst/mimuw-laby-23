/**
 * This file is for implementation of mimpirun program.
 * */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include "mimpi_common.h"
#include "channel.h"


#define CH_READ(p1, p2) (160 + p1 * (n*2) + p2 * 2)
#define CH_WRITE(p2, p1) (160 + p1 * (n*2) + p2 * 2 + 1)

int main(int argc, char **argv) {
    int n = atoi(argv[1]);

    ASSERT_SYS_OK(setenv("MIMPI_N", argv[1], 1));

    // in child process:
    // write pipe to p_i is at fd: 20 + i
    // read pipe from p_i is at fd: 40 + i

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
    

    // start all children
    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        ASSERT_SYS_OK(pid);
        if (pid == 0) {
            for (int p1 = 0; p1 < n; p1++) {
                for (int p2 = 0; p2 < n; p2++) {
                    if (p1 == p2) continue;
                    if (p1 == i) {
                        ASSERT_SYS_OK(dup2(CH_READ(p1, p2), 40 + p2));
                    } else if (p2 == i) {
                        ASSERT_SYS_OK(dup2(CH_WRITE(p2, p1), 20 + p1));
                    }
                    ASSERT_SYS_OK(close(CH_READ(p1, p2)));
                    ASSERT_SYS_OK(close(CH_WRITE(p2, p1)));
                }
            }
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
    }

    while (wait(NULL) > 0); // wait for all children to finish

    return 0;
}