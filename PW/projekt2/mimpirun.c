/**
 * This file is for implementation of mimpirun program.
 * */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include "mimpi_common.h"
#include "channel.h"

#define CH_READ(p1, p2) (160 + p1 * (n*2) + p2 * 2)
#define CH_WRITE(p2, p1) (160 + p1 * (n*2) + p2 * 2 + 1)
#define CH_M_WRITE(p) (82 + p * 2 + 1)

void master_process(int n) {
    channels_init();

    int* statuses = calloc(n, sizeof(int));

    // pipe 61 is reading from processes
    
    while(1) {
        mimpi_master_req req;
        ASSERT_SYS_OK(chrecv(61, &req, sizeof(req)));
        if (req.req_type == MIMPI_REQ_GET_STATE) {
            mimpi_master_res res;
            res.res_data.get_state.state = statuses[req.req_data.get_state.rank];
            int my_fd = CH_M_WRITE(req.req_data.get_state.my_rank);
            ASSERT_SYS_OK(chsend(my_fd, &res, sizeof(res)));
        } else if (req.req_type == MIMPI_REQ_SET_STATE) {
            int rank = req.req_data.set_state.rank;
            int state = req.req_data.set_state.state;
            statuses[rank] = state;
            if (state == MIMPI_PROC_AFTER) {
                bool all_after = true; 
                for (int rank = 0; rank < n; rank++) {
                    if (statuses[rank] != MIMPI_PROC_AFTER) {
                        all_after = false;
                        break;
                    }
                }
                if (all_after)
                    break;
            }
        }
    }

    free(statuses);

    channels_finalize();
}

int main(int argc, char **argv) {
    int n = atoi(argv[1]);

    ASSERT_SYS_OK(setenv("MIMPI_N", argv[1], 1));

    // in child process:
    // write pipe to p_i is at fd: 20 + i
    // read pipe from p_i is at fd: 40 + i
    // write pipe to master is at fd: 60
    // read pipe from master is at fd: 80

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

    // pipes for master process communication
    // 60 -> write to master

    ASSERT_SYS_OK(channel(fd));
    ASSERT_SYS_OK(dup2(fd[1], 60)); // write to master
    ASSERT_SYS_OK(dup2(fd[0], 61)); // read master
    ASSERT_SYS_OK(close(fd[0]));
    ASSERT_SYS_OK(close(fd[1]));
    for (int p = 1; p <= n; p++) {
        ASSERT_SYS_OK(channel(fd));
        ASSERT_SYS_OK(dup2(fd[0], 80 + p*2)); // read from master
        ASSERT_SYS_OK(dup2(fd[1], 80 + p*2 + 1)); // write to p
        ASSERT_SYS_OK(close(fd[0]));
        ASSERT_SYS_OK(close(fd[1]));
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
            ASSERT_SYS_OK(close(61));
            for (int p = 1; p <= n; p++) { 
                if (p - 1 == i) {
                    ASSERT_SYS_OK(dup2(80 + p*2, 80));
                }
                ASSERT_SYS_OK(close(80 + p*2));
                ASSERT_SYS_OK(close(80 + p*2 + 1));
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
        ASSERT_SYS_OK(close(CH_M_WRITE(p1) - 1));
    }
    ASSERT_SYS_OK(close(60));

    master_process(n);
    
    while (wait(NULL) > 0); // wait for all children to finish

    return 0;
}