// this program closes pipes argv[1]...argv[2] and then opens argv[3..] as cmd

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <p1> <p2> <cmd> [args...]\n", argv[0]);
        return 1;
    }

    int p1 = atoi(argv[1]);
    int p2 = atoi(argv[2]);

    for (int i = p1; i <= p2; i++) {
        close(i);
    }

    char** args = &argv[3];

    execvp(args[0], args);
}