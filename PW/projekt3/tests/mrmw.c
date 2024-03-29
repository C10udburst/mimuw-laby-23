#include <stdio.h>
#include <threads.h>
#include <stdlib.h>
#include <pthread.h>
#include "tester.h"
#include "../HazardPointer.h"

const int mrmw_iter = 100;

struct MRMWArgs {
    QueueVTable Q;
    void* queue;
    int tid;
    int threads;
    int producers;
    atomic_int* count;
    _Atomic(int*) seen;
};

volatile bool barrier;

int producer_mrmw(void* args) {
    struct MRMWArgs* a = args;

    HazardPointer_register(a->tid, a->threads);
    while (!barrier);

    for (int i = 1; i < mrmw_iter; ++i) {
        a->Q.push(a->queue, i);
    }

    free(args);
    return 0;
}

int consumer_mrmw(void* args) {
    struct MRMWArgs* a = args;
    HazardPointer_register(a->tid, a->threads);
    while (!barrier);

    while(atomic_load(a->count) < a->producers * (mrmw_iter-1)) {
        Value v = a->Q.pop(a->queue);
        if (v != EMPTY_VALUE) {
            atomic_fetch_add(a->count, 1);
            atomic_fetch_add(a->seen + v, 1);
        }
    }

    free(args);
    return 0;
}

static enum Result mrmw(QueueVTable Q, int producers, int consumers) {
    if (producers + consumers > MAX_THREADS) {
        printf("too many threads\n");
        return SKIPPED;
    }
    void* queue = Q.new();

    struct MRMWArgs baseargs = {
            Q, queue, 0, producers + consumers, producers, NULL, NULL
    };

    baseargs.seen = calloc(mrmw_iter, sizeof(atomic_int));
    baseargs.count = calloc(1, sizeof(atomic_int));
    thrd_t threads[producers + consumers];

     for (int i = 0; i < producers; ++i) {

        struct MRMWArgs* args = malloc(sizeof(struct MRMWArgs));
        memcpy(args, &baseargs, sizeof(struct MRMWArgs));
        args->tid = i + 1;
        thrd_create(&threads[i], producer_mrmw, args);
    }

    for (int i = 0; i < consumers; ++i) {
        struct MRMWArgs* args = malloc(sizeof(struct MRMWArgs));
        memcpy(args, &baseargs, sizeof(struct MRMWArgs));
        args->tid = producers + i + 1;
        thrd_create(&threads[i + producers], consumer_mrmw, args);
    }

    barrier = true;

    bool ok = true;

    for (int i = 0; i < producers + consumers; ++i) {
        int status;
        thrd_join(threads[i], &status);
        if (status != 0) {
            ok = false;
        }
    }

    if (!ok) {
        free(baseargs.seen);
        free(baseargs.count);
        return FAILED;
    }

    for (int i = 1; i < mrmw_iter; i++) {
        if (baseargs.seen[i] != baseargs.producers) {
            printf("\033[1;31mERROR: checker saw %d %d times instead of %d\033[0m\n", i, baseargs.seen[i], baseargs.producers);
            free(baseargs.seen);
            free(baseargs.count);
            return FAILED;
        }
    }

    free(baseargs.seen);
    free(baseargs.count);
    return PASSED;
}

#define mrmw_nm(n, m) \
    static enum Result mrmw_##n##_##m(QueueVTable Q) { \
        return mrmw(Q, n, m); \
    } \
    ADD_TEST(mrmw_##n##_##m)

mrmw_nm(1, 1)
mrmw_nm(2, 2)
mrmw_nm(4, 2)
mrmw_nm(2, 4)
mrmw_nm(3, 5)
mrmw_nm(16, 16)
mrmw_nm(32, 15)
mrmw_nm(72, 72)
mrmw_nm(100, 124)
