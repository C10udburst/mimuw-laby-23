#include <stdio.h>
#include <threads.h>
#include "tester.h"


#define NUM_THREADS 2
#define ORMW_WRITERS 7
#define ORMW_ELEMENTS 5

_Atomic(int) ormw_counter = 0;

void* ormw_queue;

QueueVTable ormw_Q;

int ormw_writer(void* arg)
{
    int id = (int)arg;
    
    HazardPointer_register(id, ORMW_WRITERS + 1);
    for (int i = 0; i < ORMW_ELEMENTS; ++i) {

        ormw_Q.push(ormw_queue, id*ORMW_WRITERS + i + 1);
        atomic_fetch_add(&ormw_counter, 1);
    }


    return 0;
}

int ormw_reader(void *arg)
{
    
    HazardPointer_register(ORMW_WRITERS, ORMW_WRITERS + 1);
    int my_counter = 0;
    int anwsers[ORMW_WRITERS][ORMW_ELEMENTS];
    int indexes[ORMW_WRITERS];
    for (int i = 0; i < ORMW_WRITERS; ++i) {
        indexes[i] = 0;
    }
    while (my_counter < ORMW_ELEMENTS * ORMW_WRITERS || atomic_load(&ormw_counter) < ORMW_ELEMENTS * ORMW_WRITERS) {
        Value item = ormw_Q.pop(ormw_queue);
        if (item != EMPTY_VALUE) {
            //printf("%lu\n", item);
            int writer_id = item / ORMW_WRITERS;
            item = item % ORMW_WRITERS;
            //printf("writer_id: %d, item: %d\n", writer_id, item);
            anwsers[writer_id][indexes[writer_id]] = item;
            indexes[writer_id]++;
            my_counter++;
        }
    }

    for (int i = 0; i < ORMW_WRITERS; ++i) {
        for (int j = 0; j < ORMW_ELEMENTS; ++j) {
            printf("%d ", anwsers[i][j]);
        }
        printf("\n");
    }

    return 0;
}

enum Result one_read_multiple_write(QueueVTable Q)
{
    ormw_Q = Q;
    // run threads
    thrd_t write_threads[ORMW_WRITERS];
    thrd_t read_thread;
    ormw_queue = ormw_Q.new();
    thrd_create(&read_thread, ormw_reader, NULL);
    for (int i = 0; i < ORMW_WRITERS; ++i) {
        thrd_create(&write_threads[i], ormw_writer, (void*)i);
    }
     
    thrd_join(read_thread, NULL);
    for (int i = 0; i < ORMW_WRITERS; ++i) {
        thrd_join(write_threads[i], NULL);
    }
    ormw_Q.delete(ormw_queue);

    return PASSED;
}
ADD_TEST(one_read_multiple_write)
