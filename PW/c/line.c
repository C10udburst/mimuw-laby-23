#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "err.h"

//#define ONLYFORK


int main(int argc, char* argv[])
{
    int n_children = 5;
    if (argc > 2)
        fatal("Expected zero or one arguments, got: %d.", argc - 1);
    if (argc == 2)
        n_children = atoi(argv[1]);

    printf("My pid is %d, my parent's pid is %d\n", getpid(), getppid());

    #ifdef ONLYFORK
        while (1) {
            __pid_t pid = fork();
            ASSERT_SYS_OK(pid);
            if (pid == 0) {
                // child
                n_children--;
                if (n_children == 0) {
                    break;
                }
            } else {
                break;
            }
        }
        if (n_children > 0) ASSERT_SYS_OK(wait(NULL));
    #endif
    
    
    #ifndef ONLYFORK
        if (n_children > 0) {
            __pid_t pid = fork();
            ASSERT_SYS_OK(pid);
            if (pid == 0) {
                // child
                n_children--;
                char* buf = malloc((n_children + 1) * sizeof(char));
                sprintf(buf, "%d", n_children);
                execl("./line", "line", buf, NULL);
            } else {
                // parent
                ASSERT_SYS_OK(wait(NULL));
            }
        }

    #endif

    printf("My pid is %d, exiting.\n", getpid());

    return 0;
}
