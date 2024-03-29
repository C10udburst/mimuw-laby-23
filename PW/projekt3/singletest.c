//
// Created by cloudburst on 20.01.24.
//

#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <bits/types/siginfo_t.h>

#include "BLQueue.h"
#include "HazardPointer.h"
#include "LLQueue.h"
#include "SimpleQueue.h"
#include "RingsQueue.h"
#include "./tests/tester.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

const QueueVTable queueVTables[] = {
        MakeVTable(SimpleQueue),
        MakeVTable(RingsQueue),
        MakeVTable(LLQueue),
        MakeVTable(BLQueue)
};

#pragma GCC diagnostic pop

const char* queue_name = "none";
const char* test_name = "none";



void handler(int sig, siginfo_t *arg, void *ctx)
{

    printf("\033[1;34m%s\033[0m on queue type \033[1;34m%s\033[0m \033[1;31mSEGFAULT\033[0m\n", test_name, queue_name);
    exit(1);
}

int run_within_valgrind (void)
{
  char *p = getenv ("LD_PRELOAD");
  if (p == NULL)
    return 0;
  return (strstr (p, "/valgrind/") != NULL ||
          strstr (p, "/vgpreload") != NULL);
}

int main(int argc, char** argv)
{
    sigaction(SIGSEGV, &(struct sigaction){.sa_sigaction = handler, .sa_flags = SA_SIGINFO}, NULL);

    bool is_valgrind = run_within_valgrind();

    foreach_test(elem) {
        if (elem == NULL || elem->test == NULL) continue;
        if (strcmp(elem->name, argv[1]) != 0) continue;
        test_name = elem->name;
        for (int i = 0; i < sizeof(queueVTables) / sizeof(QueueVTable); ++i) {
            if (strcmp(queueVTables[i].name, argv[2]) != 0)
                continue;
            QueueVTable Q = queueVTables[i];
            queue_name = Q.name;
            clock_t start, end;
            start = clock();
            enum Result res = elem->test(Q);
            end = clock();
            switch (res) {
                case PASSED:
                    printf("\033[1;34m%s\033[0m on queue type \033[1;34m%s\033[0m \033[1;32mPASSED\033[0m in \033[1;33m%.3lf\033[0ms\n", elem->name, Q.name, (double) (end - start) / CLOCKS_PER_SEC);
                    break;
                case FAILED:
                    printf("\033[1;34m%s\033[0m on queue type \033[1;34m%s\033[0m \033[1;31mFAILED\033[0m\n", elem->name, Q.name);
                    break;
                case SKIPPED:
                    printf("\033[1;34m%s\033[0m on queue type \033[1;34m%s\033[0m \033[1;35mSKIPPED\033[0m\n", elem->name, Q.name);
                    break;
            }
            return res;
        }
    }

    return 0;
}
