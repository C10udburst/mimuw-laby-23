#include <stdio.h>
#include <threads.h>

#include "BLQueue.h"
#include "HazardPointer.h"
#include "LLQueue.h"
#include "SimpleQueue.h"

// A structure holding function pointers to methods of some queue type.
struct QueueVTable {
    const char* name;
    void* (*new)(void);
    void (*push)(void* queue, Value item);
    Value (*pop)(void* queue);
    bool (*is_empty)(void* queue);
    void (*delete)(void* queue);
};
typedef struct QueueVTable QueueVTable;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

const QueueVTable queueVTables[] = {
    { "SimpleQueue", SimpleQueue_new, SimpleQueue_push, SimpleQueue_pop, SimpleQueue_is_empty, SimpleQueue_delete },
    { "LLQueue", LLQueue_new, LLQueue_push, LLQueue_pop, LLQueue_is_empty, LLQueue_delete },
    { "BLQueue", BLQueue_new, BLQueue_push, BLQueue_pop, BLQueue_is_empty, BLQueue_delete }
};

void basic_test(QueueVTable Q)
{
    HazardPointer_register(0, 1);
    void* queue = Q.new();

    Q.push(queue, 1);
    Q.push(queue, 2);
    Q.push(queue, 3);
    Value a = Q.pop(queue);
    Value b = Q.pop(queue);
    Value c = Q.pop(queue);
    printf("%lu %lu %lu\n", a, b, c);

    Q.delete(queue);
}

// region Threaded test 1

struct ThreadArgs {
    void* queue;
    QueueVTable Q;
};

void* producer1(void* args)
{
    HazardPointer_register(0, 2);
    struct ThreadArgs* targs = args;
    for (int i = 1; i < 1000000; ++i) {
        targs->Q.push(targs->queue, i);
    }
    return NULL;
}

void* consumer1(void* args)
{
    HazardPointer_register(1, 2);
    struct ThreadArgs* targs = args;
    for (int i = 1; i < 1000000; ++i) {
        Value v = targs->Q.pop(targs->queue);
        while (v == EMPTY_VALUE) {
            v = targs->Q.pop(targs->queue);
        }
        if (v != i) {
            printf("Error: %lu != %d\n", v, i);
        }
    }
    return NULL;
}

void threaded_test1(QueueVTable Q)
{
    void* queue = Q.new();

    struct ThreadArgs args = { queue, Q };

    thrd_t prod;
    thrd_t cons;

    thrd_create(&prod, producer1, &args);
    thrd_create(&cons, consumer1, &args);

    thrd_join(prod, NULL);
    thrd_join(cons, NULL);

    printf("Done threaded test 1\n");

    Q.delete(queue);
}
#pragma GCC diagnostic pop


// endregion


int main(void)
{
    printf("Hello, World!\n");

    for (int i = 0; i < sizeof(queueVTables) / sizeof(QueueVTable); ++i) {
        QueueVTable Q = queueVTables[i];
        printf("Queue type: %s\n", Q.name);
        basic_test(Q);
        threaded_test1(Q);
    }

    return 0;
}
