#include <stdio.h>
#include <threads.h>
#include "tester.h"

#define NUM_THREADS 2
#define MROW_READERS 7
#define MROW_ELEMENTS 100

_Atomic(int) mrow_counter = 0;

void* mrow_queue;

_Atomic(int) mrow_my_counter = 0;

// declare counter mutexes

QueueVTable mrow_Q;

int mrow_writer(void* arg)
{
    
    HazardPointer_register(MROW_READERS, MROW_READERS + 1);
    for (int i = 0; i < MROW_ELEMENTS; ++i) {

        mrow_Q.push(mrow_queue, i+1);
        atomic_fetch_add(&mrow_counter, 1);
    }


    return 0;
}

int mrow_reader(void *arg)
{
    int id = (int)arg;
    
    HazardPointer_register(id, MROW_READERS + 1);
    int popped[MROW_ELEMENTS];
    for (int i = 0; i < MROW_ELEMENTS; ++i) {
        popped[i] = 0;
    }
    while (atomic_load(&mrow_my_counter) < MROW_ELEMENTS || atomic_load(&mrow_counter) < MROW_ELEMENTS) {
        Value item = mrow_Q.pop(mrow_queue);
        if (item != EMPTY_VALUE) {
            //printf("%lu\n", item);
            //my_counter++;
            int idx = atomic_fetch_add(&mrow_my_counter, 1);
            popped[idx] = item;
        }
    }
    for (int i = 1; i < MROW_ELEMENTS && popped[i]; ++i) {
        // check if popped is sorted
        if (popped[i-1] > popped[i]) {
            printf("not sorted\n");
            return 1;
        }
    }
    printf("sorted\n");

    return 0;
}

enum Result multiple_read_one_write(QueueVTable Q)
{
    mrow_Q = Q;
    // run threads
    thrd_t write_thread;
    thrd_t read_thread;
    mrow_queue = mrow_Q.new();
    for (int i = 0; i < MROW_READERS; ++i) {
        thrd_create(&read_thread, mrow_reader, (void*)i);
    }
    thrd_create(&write_thread, mrow_writer, NULL);
     
    for (int i = 0; i < MROW_READERS; ++i) {
        thrd_join(read_thread, NULL);
    }
    thrd_join(write_thread, NULL);
    mrow_Q.delete(mrow_queue);

    return PASSED;
}
ADD_TEST(multiple_read_one_write)
