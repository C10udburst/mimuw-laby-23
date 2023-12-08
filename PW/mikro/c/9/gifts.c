#include <assert.h>
#include <fcntl.h> // For O_* constants.
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h> // For mode constants.
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "err.h"

// Some constants.
#define MAX_REINDEERS 7
#define MAX_ELFS 6
#define MAX_GIFTS 5

#define N_REINDEERS 3
#define N_ELFS 4
#define N_GIFTS 5

#define BUFFSIZE 3
#define LOOPS 5

#define NAP_MICROSECS 100000

// Storage compartment.
struct SharedStorage {
    int buff[BUFFSIZE];
    int begin;
    int end;
    sem_t mutex;
    sem_t sem_free_slots;
    sem_t sem_filled_slots;
    int n_gifts_to_deliver;
};

// popular names
const char* elfs_names[MAX_ELFS] = { "Mirek", "Zuzia", "Gienia", "Macius", "Ela", "Stasia" };
const char* reindeers_names[MAX_REINDEERS] = { "Janek", "Zosia", "Franek", "Jozek", "Asia", "Olek", "Ruda" };
const char* gifts[MAX_GIFTS] = { "dolls", "legos", "a train", "a whip", "a bike" };

// Some toy-making procedure.
int produce()
{
    usleep((rand() % 3) * NAP_MICROSECS);
    return rand() % N_GIFTS;
}

// What to do on delivery.
void deliver(int i)
{
    usleep((rand() % 4) * NAP_MICROSECS);
}

/**************************************************************************************************/
// life of an elf
int elf_main(int id, struct SharedStorage* s)
{
    printf("Elf %s: starting!\n", elfs_names[id]);
    for (int i = 0; i < LOOPS; ++i) {
        int g = produce();
        printf("Elf %s: I produced %s\n", elfs_names[id], gifts[g]);

        ASSERT_SYS_OK(sem_wait(&s->sem_free_slots)); // jeśli nie ma miejsca w buforze, to czekamy

        ASSERT_SYS_OK(sem_wait(&s->mutex));

        s->buff[s->end] = g;
        s->end = (s->end + 1) % BUFFSIZE;

        ASSERT_SYS_OK(sem_post(&s->mutex));

        ASSERT_SYS_OK(sem_post(&s->sem_filled_slots));

        printf("Elf %s: I enqueued %s\n", elfs_names[id], gifts[g]);
    }
    printf("Elf %s: done!\n", elfs_names[id]);
    return 0;
}

/**************************************************************************************************/
// life of a reindeer
int reindeer_main(int id, struct SharedStorage* s)
{
    printf("Reindeer %s: starting!\n", reindeers_names[id]);
    while (true) {
        ASSERT_SYS_OK(sem_wait(&s->sem_filled_slots)); // jeśli nie ma prezentów w buforze, to czekamy
        printf("Left %d\n", s->n_gifts_to_deliver);
        ASSERT_SYS_OK(sem_wait(&s->mutex));

        if (s->n_gifts_to_deliver <= 0) {
            ASSERT_SYS_OK(sem_post(&s->mutex));
            ASSERT_SYS_OK(sem_post(&s->sem_filled_slots));
            break;
        } else {
            s->n_gifts_to_deliver--;

            int g = s->buff[s->begin];
            s->begin = (s->begin + 1) % BUFFSIZE;

            ASSERT_SYS_OK(sem_post(&s->mutex));
            ASSERT_SYS_OK(sem_post(&s->sem_free_slots));

            printf("Reindeer %s: I got %s\n", reindeers_names[id], gifts[g]);
            deliver(g);
            printf("Reindeer %s: I delivered %s\n", reindeers_names[id], gifts[g]);
        }
    }
    return 0;
}

/* ---- Test program ---- */
int main(void)
{
    srand(time(NULL));

    // Create shared storage.
    struct SharedStorage* shared_storage = mmap(
        NULL,
        sizeof(struct SharedStorage),
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0);
    if (shared_storage == MAP_FAILED)
        syserr("mmap");

    // TODO initialize shared_storage
    ASSERT_SYS_OK(sem_init(&shared_storage->mutex, 1, 1));
    ASSERT_SYS_OK(sem_init(&shared_storage->sem_free_slots, 1, BUFFSIZE));
    ASSERT_SYS_OK(sem_init(&shared_storage->sem_filled_slots, 1, 0));
    shared_storage->begin = 0;
    shared_storage->end = 0;
    shared_storage->n_gifts_to_deliver = LOOPS * N_ELFS - 1;

    printf("Creating workers.\nElfs: %d, reindeers: %d\n", N_ELFS, N_REINDEERS);

    for (int i = 0; i < N_ELFS + N_REINDEERS; i++) {
        rand(); // some randomness
        pid_t pid = fork();
        ASSERT_SYS_OK(pid);
        if (!pid) {
            srand(rand());
            if (i < N_ELFS)
                return elf_main(i, shared_storage);
            else
                return reindeer_main(i - N_ELFS, shared_storage);
        }
        usleep(1 * NAP_MICROSECS);
        printf("Next worker!\n");
    }

    for (int i = 0; i < N_ELFS + N_REINDEERS; ++i)
        ASSERT_SYS_OK(wait(NULL));

    assert(shared_storage->n_gifts_to_deliver == 0);

    // TODO clean up shared_storage
    ASSERT_SYS_OK(sem_destroy(&shared_storage->mutex));
    ASSERT_SYS_OK(sem_destroy(&shared_storage->sem_free_slots));
    ASSERT_SYS_OK(sem_destroy(&shared_storage->sem_filled_slots));

    return 0;
}
