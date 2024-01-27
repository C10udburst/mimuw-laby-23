//
// Created by cloudburst on 20.01.24.
//

#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <time.h>
#include <string.h>

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


int main(int argc, char** argv)
{
    foreach_test(elem) {
        if (elem == NULL || elem->test == NULL) continue;
        if (argc > 1 && strcmp(argv[1], "all") != 0)
            if (strcmp(elem->name, argv[1]) != 0) continue;
        for (int i = 0; i < sizeof(queueVTables) / sizeof(QueueVTable); ++i) {
            if (argc > 2)
                if (strcmp(queueVTables[i].name, argv[2]) != 0) continue;
            if (fork() == 0) {
                QueueVTable Q = queueVTables[i];
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
    }

    int status;
    int passed = 0;
    int failed = 0;
    int skipped = 0;
    int total = 0;

    while (wait(&status) != -1) {
        if (WIFEXITED(status)) {
            switch (WEXITSTATUS(status)) {
                case PASSED:
                    ++passed;
                    break;
                case FAILED:
                    ++failed;
                    break;
                case SKIPPED:
                    ++skipped;
                    break;
            }
        } else {
            printf("Child terminated abnormally: \033[1;31m%s (%d)\033[0m\n", strerror(WTERMSIG(status)), WTERMSIG(status));
            ++failed;
        }
        ++total;
    }

    printf("Passed: \033[1;32m%d\033[0m (%0.3lf%%), Failed: \033[1;31m%d\033[0m, Skipped: \033[1;35m%d\033[0m, Total: \033[1;33m%d\033[0m", passed, (double) passed*100 / (passed + failed), failed, skipped, total);

    return 0;
}
