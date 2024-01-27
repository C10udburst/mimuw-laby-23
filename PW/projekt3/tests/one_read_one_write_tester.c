#include <stdio.h>
#include <threads.h>
#include "tester.h"


#define NUM_THREADS 2
#define ELEMENTS 5

_Atomic(int) orow_counter = 0;
void* orow_queue;

QueueVTable orow_Q;

int orow_writer(void* arg)
{
    
    HazardPointer_register(0, 2);
    for (int i = 0; i < ELEMENTS; ++i) {
        
        orow_Q.push(orow_queue, i+1);
        atomic_fetch_add(&orow_counter, 1);
    }


    return 0;
}

int orow_reader(void *arg)
{
    
    HazardPointer_register(1, 2);
    int my_counter = 0;
    while (my_counter < ELEMENTS || atomic_load(&orow_counter) < ELEMENTS) {
        Value item = orow_Q.pop(orow_queue);
        if (item != EMPTY_VALUE) {
            printf("%lu\n", item);
            my_counter++;
        } 
    }

    return 0;
}

enum Result one_read_one_write(QueueVTable Q)
{
    orow_Q = Q;
    // run threads
    thrd_t write_thread;
    thrd_t read_thread;
    orow_queue = orow_Q.new();
    thrd_create(&read_thread, orow_reader, NULL);
    thrd_create(&write_thread, orow_writer, NULL);
     
    thrd_join(read_thread, NULL);
    thrd_join(write_thread, NULL);
    orow_Q.delete(orow_queue);

    return PASSED;
}

ADD_TEST(one_read_one_write)
