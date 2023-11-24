#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "err.h"
#include "pipeline-utils.h"


int main(int argc, char* argv[]) {
    // Read descriptor of previous pipe.
    int prev_read_dsc = -1;

    fprintf(stderr, "Pid %d starts %s\n", getpid(), argv[0]);
    print_open_descriptors();

    // Spawn children calling argv[1], ..., argv[argc - 1].
    for (int i = 1; i < argc; ++i) {
        // FIXME: Create pipe to next program (if there is one).
        int next_pipe[2];
        if (i < argc - 1) {
            ASSERT_SYS_OK(pipe(next_pipe));
        }

        pid_t pid = fork();
        ASSERT_SYS_OK(pid);
        if (!pid) {
            // For all programs but the first, replace stdin with the previous pipe's reading end.
            if (i > 1) {
                fprintf(stderr, "Child %d: replacing stdin descriptor %d with a copy of %d\n", i, STDIN_FILENO, prev_read_dsc);
                ASSERT_SYS_OK(dup2(prev_read_dsc, STDIN_FILENO));
                ASSERT_SYS_OK(close(prev_read_dsc));
            }
            // For all programs but the last, replace stdout with the current pipe's writing end.
            if (i < argc - 1) {
                // FIXME: But first, close reading end of pipe, only the next program will use it.
                ASSERT_SYS_OK(close(next_pipe[0]));

                int next_write_dsc = next_pipe[1];

                fprintf(stderr, "Child %d: replacing stdout descriptor %d with a copy of %d\n", i, STDOUT_FILENO, next_write_dsc);
                ASSERT_SYS_OK(dup2(next_write_dsc, STDOUT_FILENO));
                ASSERT_SYS_OK(close(next_write_dsc));
            }

            fprintf(stderr, "Pid %d execs %s\n", getpid(), argv[i]);
            print_open_descriptors();
            ASSERT_SYS_OK(execlp(argv[i], argv[i], NULL));
        }

        // we are in parent

        sleep(1);

        fprintf(stderr, "Pid %d spawned child %d, next_pipe = %d\n", getpid(), pid, next_pipe[0]);

        // Close the read descriptor we gave to our child.
        if (i > 1) {
            ASSERT_SYS_OK(close(prev_read_dsc));
        }
        // Close writing end of pipe, keep the reading end.
        if (i < argc - 1) {
            ASSERT_SYS_OK(close(next_pipe[1]));
        }

        prev_read_dsc = next_pipe[0];
    }

    for (int i = 1; i < argc; ++i) {
        ASSERT_SYS_OK(wait(NULL));
    }

    fprintf(stderr, "Pid %d finished %s\n", getpid(), argv[0]);
    print_open_descriptors();

    return 0;
}
