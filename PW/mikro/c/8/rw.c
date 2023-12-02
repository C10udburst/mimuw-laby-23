#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "err.h"

#define DEBUG

#ifdef DEBUG
    #define MUTEX_ATTR (pthread_mutexattr_t[]) { PTHREAD_MUTEX_ERRORCHECK }
#else
    #define MUTEX_ATTR NULL
#endif

struct ReadWriteLock {
    pthread_mutex_t mutex;
    pthread_cond_t readers;
    pthread_cond_t writers;
    int n_reading, n_writing;
    int n_readers_waiting, n_writers_waiting;
    bool change;
};

void read_write_lock_init(struct ReadWriteLock* rw) {
    rw->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    rw->readers = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    rw->writers = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    rw->n_reading = 0;
    rw->n_writing = 0;
    rw->n_readers_waiting = 0;
    rw->n_writers_waiting = 0;
    rw->change = false;
    ASSERT_SYS_OK(pthread_mutex_init(&rw->mutex, NULL));
    ASSERT_SYS_OK(pthread_cond_init(&rw->readers, NULL));
    ASSERT_SYS_OK(pthread_cond_init(&rw->writers, NULL));
}

void read_write_lock_destroy(struct ReadWriteLock* rw) {
    ASSERT_SYS_OK(pthread_mutex_destroy(&rw->mutex));
    ASSERT_SYS_OK(pthread_cond_destroy(&rw->readers));
    ASSERT_SYS_OK(pthread_cond_destroy(&rw->writers));
}

void before_write(struct ReadWriteLock* rw) {
    ASSERT_ZERO(pthread_mutex_lock(&rw->mutex));

    while (rw->n_reading > 0 || rw->n_writing > 0) {
        rw->n_writers_waiting++;
        ASSERT_ZERO(pthread_cond_wait(&rw->writers, &rw->mutex));
        rw->n_writers_waiting--;
    }

    rw->n_writing = 1;

    ASSERT_ZERO(pthread_mutex_unlock(&rw->mutex));
}

void after_write(struct ReadWriteLock* rw) {
    ASSERT_ZERO(pthread_mutex_lock(&rw->mutex));

    rw->n_writing = 0;

    if (rw->n_readers_waiting > 0) {
        rw->change = true;
        ASSERT_ZERO(pthread_cond_broadcast(&rw->readers));
    }
    else if (rw->n_writers_waiting > 0)
        ASSERT_ZERO(pthread_cond_signal(&rw->writers));

    ASSERT_ZERO(pthread_mutex_unlock(&rw->mutex));
}

void before_read(struct ReadWriteLock* rw) {
    ASSERT_ZERO(pthread_mutex_lock(&rw->mutex));

    while (rw->n_writing > 0 || (rw->n_writers_waiting > 0 && !rw->change)) {
        rw->n_readers_waiting++;
        ASSERT_ZERO(pthread_cond_wait(&rw->readers, &rw->mutex));
        rw->n_readers_waiting--;
    }

    rw->n_reading++;

    ASSERT_ZERO(pthread_mutex_unlock(&rw->mutex));
}

void after_read(struct ReadWriteLock* rw) {
    ASSERT_ZERO(pthread_mutex_lock(&rw->mutex));

    rw->change = false;
    rw->n_reading--;
    if (rw->n_reading == 0 && rw->n_writers_waiting > 0)
        ASSERT_ZERO(pthread_cond_signal(&rw->writers));

    ASSERT_ZERO(pthread_mutex_unlock(&rw->mutex));
}

/* ---------------------- Example program ----------------------
 * In a correct solution, the six numbers in each line should be equal
 * (otherwise that means a writer was writing while a reader was reading).
 *
 * Take a look at whether the numbers stay unchanged for many lines.
 * If so, this means writers may be (somewhat) starved.
 */

#define N_READERS 3
#define N_WRITERS 2
#define NAP_TIME_SECONDS 2

struct Library {
    char book[1024]; // A buffer which will be protected by a ReadWriteLock.
    struct ReadWriteLock rw_lock;
    atomic_bool working;
};

void* reader_main(void* data)
{
    int count = 0;
    struct Library* library = data;
    while (library->working) {
        before_read(&library->rw_lock);
        printf("Reader read: %s\n", library->book);
        after_read(&library->rw_lock);
        count++;
    }
    printf("Reader read %d times\n", count);
    return 0;
}

void* writer_main(void* data)
{   
    int count = 0;
    struct Library* library = data;
    while (library->working) {
        int n = rand() % 10;
        before_write(&library->rw_lock);
        fprintf(stderr, "Writer writing: %d\n", n);
        snprintf(library->book, sizeof(library->book), "six times a number %d %d %d %d %d %d", n, n, n, n, n, n);
        after_write(&library->rw_lock);
        count++;
    }
    printf("Writer wrote %d times\n", count);
    return 0;
}

int main()
{
    srand((unsigned)time(0));

    struct Library library;
    snprintf(library.book, sizeof(library.book), "Empty.");
    read_write_lock_init(&library.rw_lock);
    library.working = true;

    pthread_t threads[N_READERS + N_WRITERS];
    for (int i = 0; i < N_READERS + N_WRITERS; i++) {
        if (i < N_READERS)
            ASSERT_ZERO(pthread_create(&threads[i], NULL, reader_main, &library));
        else
            ASSERT_ZERO(pthread_create(&threads[i], NULL, writer_main, &library));
    }

    sleep(NAP_TIME_SECONDS);
    library.working = false;

    for (int i = 0; i < N_READERS + N_WRITERS; i++)
        ASSERT_ZERO(pthread_join(threads[i], NULL));

    read_write_lock_destroy(&library.rw_lock);

    return 0;
}
