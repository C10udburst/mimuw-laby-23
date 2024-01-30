#include <stdio.h>
#include <threads.h>
#include "tester.h"

#define NUM_THREADS 2
#define MWMW_WRITERS 7
#define MRMW_READERS 7
#define MRMW_ELEMENTS (MWMW_WRITERS - 1)

_Atomic(int) mrmw_counter = 0;

void* mrmw_queue;

QueueVTable mrmw_Q;

int mrmw_writer(void* arg)
{
    int id = (int)arg;
    
    HazardPointer_register(id, MWMW_WRITERS + MRMW_READERS);
    for (int i = 0; i < MRMW_ELEMENTS; ++i) {

        mrmw_Q.push(mrmw_queue, id*MWMW_WRITERS + i + 1);
        atomic_fetch_add(&mrmw_counter, 1);
    }


    return 0;
}

int mrmw_reader(void *arg)
{
    int id = (int)arg;
    
    HazardPointer_register(MWMW_WRITERS + id, MWMW_WRITERS + MRMW_READERS);
    int my_counter = 0;
    int anwsers[MWMW_WRITERS][MRMW_ELEMENTS];
    int indexes[MWMW_WRITERS];
    for (int i = 0; i < MWMW_WRITERS; ++i) {
        indexes[i] = 0;
    }
    while (my_counter < MRMW_ELEMENTS * MWMW_WRITERS || atomic_load(&mrmw_counter) < MRMW_ELEMENTS * MWMW_WRITERS) {
        Value item = mrmw_Q.pop(mrmw_queue);
        if (item != EMPTY_VALUE) {
            //printf("%lu\n", item);
            int writer_id = item / MWMW_WRITERS;
            item = item % MWMW_WRITERS;
            //printf("writer_id: %d, item: %d\n", writer_id, item);
            anwsers[writer_id][indexes[writer_id]] = item;
            indexes[writer_id]++;
            my_counter++;
        }
    }

    for (int i = 0; i < MWMW_WRITERS; ++i) {
        for (int j = 1; j < MRMW_ELEMENTS && anwsers[i][j]; ++j) {
            if (anwsers[i][j] < anwsers[i][j-1]) {
                printf("not sorted\n");
                return 1;
            }
        }
    }
    printf("sorted\n");

    return 0;
}

enum Result multiple_read_multiple_write(QueueVTable q)
{
    mrmw_Q = q;
    // run threads
    thrd_t write_threads[MWMW_WRITERS];
    thrd_t read_thread;
    mrmw_queue = mrmw_Q.new();
    thrd_create(&read_thread, mrmw_reader, NULL);
    for (int i = 0; i < MWMW_WRITERS; ++i) {
        thrd_create(&write_threads[i], mrmw_writer, (void*)i);
    }
     
    thrd_join(read_thread, NULL);
    for (int i = 0; i < MWMW_WRITERS; ++i) {
        thrd_join(write_threads[i], NULL);
    }
    mrmw_Q.delete(mrmw_queue);

    return PASSED;
}

ADD_TEST(multiple_read_multiple_write)
