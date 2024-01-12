#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>

const long counter = 5000000;

volatile long x = 0;
atomic_int wants[2] = {0, 0};
atomic_int priority = 0;

void critical_section(void) {
    long y;
    y = x;
    y = y + 1;
    x = y;
}

void local_section(void) {
}

void entry_protocol(int nr) {
    int other = 1 - nr;
    atomic_store(&wants[nr], 1);
    while (atomic_load(&wants[other]) == 1) {
        if (atomic_load(&priority) == other) {
            atomic_store(&wants[nr], 0);
            while (atomic_load(&priority) == other) {
                // busy wait
            }
            atomic_store(&wants[nr], 1);
        }
    }
}

void exit_protocol(int nr) {
    atomic_store(&wants[nr], 0);
    atomic_store(&priority, 1 - nr);
}

void* th(void* arg) {
    int nr = *(int*)arg;
    for (long i = 0; i < counter; i++) {
        local_section();
        entry_protocol(nr);
        critical_section();
        exit_protocol(nr);
    }
    return NULL;
}

void* monitor(void* arg) {
    (void)arg; // Unused argument
    long prev = 0;
    while (1) {
        sleep(2);
        if (prev == x) {
            printf("Deadlock! wants = %d/%d priority %d\n", 
                atomic_load(&wants[0]), 
                atomic_load(&wants[1]), 
                atomic_load(&priority)
            );
        }
        else {
            printf("monitor: %ld\n", x);
        }
    }
    return NULL;
}

int main() {
    printf("main() starts\n");

    pthread_t monitor_th;
    pthread_t t1, t2;

    pthread_create(&monitor_th, NULL, monitor, NULL);
    int t1_arg = 0;
    int t2_arg = 1;
    pthread_create(&t1, NULL, th, (void*)&t1_arg);
    pthread_create(&t2, NULL, th, (void*)&t2_arg);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("main() completes: %ld\n", x);

    return 0;
}
