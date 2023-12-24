/**
 * This file is for implementation of mimpirun program.
 * */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include "mimpi_common.h"

int main(int argc, char **argv) {
    int n = atoi(argv[1]);
    ASSERT_SYS_OK(setenv("MIMPI_N", argv[1], 1));
    char** args = (argc > 2) ? &argv[2] : NULL;
    // TODO: otwieranie kanałów?
    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        ASSERT_SYS_OK(pid);
        if (pid == 0) {
            sprintf(argv[1], "%d", i);
            ASSERT_SYS_OK(setenv("MIMPI_RANK", argv[1], 1));
            ASSERT_SYS_OK(execvp(args[0], args));
        }
    }
    ASSERT_SYS_OK(wait(NULL));
    return 0;
}