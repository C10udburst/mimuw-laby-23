/**
 * This file is for implementation of MIMPI library.
 * */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>
#include "channel.h"
#include "mimpi.h"
#include "mimpi_common.h"

#define BARRIER_TAG -0xBA221E2 // tag used for barrier messages
#define NOWAIT_TAG -0x1D7E  // tag used for specifying that our process is not waiting for a message

#define DIE_TAG -0xDEAD // tag used for notifying other processes that we are finishing

typedef struct {
    int tag;
    int count;
} packet_header;

typedef struct buffer_entry {
    int tag;
    int count;
    void* data;
    struct buffer_entry* next;
} buffer_entry;

typedef struct {
    int tag;
    int count_target;
    volatile bool found;
    pthread_cond_t cond;
} buffer_update_t;

struct global_t {
    int rank;
    int size;
    int states;
    volatile bool finishing;
    struct {
        pthread_mutex_t state;
        pthread_mutex_t* buffers;
    } mutex;
    pthread_t* threads;
    buffer_update_t* buffer_update;
    buffer_entry* buffers;
};

struct global_t global;

// internal functions

int min(int a, int b) {
    return a < b ? a : b;
}

bool is_proc_done(int rank) {
    ASSERT_SYS_OK(pthread_mutex_lock(&global.mutex.state));
    bool result = global.states & (1 << rank);
    ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.state));
    return result;
}

void set_done(int rank) {
    ASSERT_SYS_OK(pthread_mutex_lock(&global.mutex.state));
    global.states |= (1 << rank);
    ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.state));
}

void* buffer_thread(void* source_rank) {
    int rank = *(int*)source_rank;
    free(source_rank);
    int fd = 40 + rank;
    packet_header header;
    int res;
    while (global.finishing == false)
    {
        // receive header
        res = chrecv(fd, &header, sizeof(header));
        if ((res < 0 && errno == EPIPE) || (res >= 0 && res < sizeof(header))) {
            if (global.finishing) {
                // Our process is finishing
                break;
            } else {
                // The other process has finished
                set_done(rank);
                ASSERT_SYS_OK(pthread_mutex_lock(&global.mutex.buffers[rank]));
                ASSERT_SYS_OK(pthread_cond_signal(&global.buffer_update[rank].cond));
                ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.buffers[rank]));
            }
            break;
        }
        ASSERT_SYS_OK(res);

        if (header.tag == DIE_TAG) {
            // The other process is finishing, resend the header to it, to kill the chrecv
            int res = chsend(20+rank, &header, sizeof(header));
            if (res < 0 && errno != EPIPE)
                ASSERT_SYS_OK(res);
            set_done(rank);
            ASSERT_SYS_OK(pthread_mutex_lock(&global.mutex.buffers[rank]));
            ASSERT_SYS_OK(pthread_cond_signal(&global.buffer_update[rank].cond));
            ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.buffers[rank]));
            break;
        }
    
        // receive data
        void* data = malloc(header.count);
        int offset = 0;
        while (offset < header.count) {
            int count = min(header.count - offset, 512);
            res = chrecv(fd, data + offset, count);
            if ((res < 0 && errno == EPIPE) || (res >= 0 && res < count)) {
                free(data);
                if (global.finishing) {
                    // Our process is finishing
                    goto kill_thread;
                } else {
                    // The other process has finished
                    set_done(rank);
                    ASSERT_SYS_OK(pthread_mutex_lock(&global.mutex.buffers[rank]));
                    ASSERT_SYS_OK(pthread_cond_signal(&global.buffer_update[rank].cond));
                    ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.buffers[rank]));
                }
                goto kill_thread;
            }
            ASSERT_SYS_OK(res);
            offset += res;
        }
        
        // make buffer entry
        buffer_entry* entry = malloc(sizeof(buffer_entry));
        entry->tag = header.tag;
        entry->count = header.count;
        entry->data = data;
        entry->next = NULL;

        // add buffer entry
        ASSERT_SYS_OK(pthread_mutex_lock(&global.mutex.buffers[rank]));
        buffer_entry* last = &global.buffers[rank];
        while (last->next != NULL)
            last = last->next;
        last->next = entry;
        if (global.buffer_update[rank].tag == header.tag && global.buffer_update[rank].count_target == header.count) {
            global.buffer_update[rank].found = true;
            ASSERT_SYS_OK(pthread_cond_signal(&global.buffer_update[rank].cond));
        }
        ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.buffers[rank]));
    }
kill_thread:
    return NULL;
}

bool ok_tag(int tag, int filter) {
    return (filter == MIMPI_ANY_TAG && tag > 0) || tag == filter;
}

int buf_size(int rank, int tag) {
    int size = 0;
    buffer_entry* entry = &global.buffers[rank];
    while (entry != NULL) {
        if (ok_tag(entry->tag, tag)) {
            size += entry->count;
        }
        entry = entry->next;
    }
    return size;
}

buffer_entry* pop(int rank, int tag, int count) {
    //ASSERT_SYS_OK(pthread_mutex_lock(&global.mutex.buffers[rank]));
    buffer_entry* entry = global.buffers[rank].next;
    buffer_entry* prev = &global.buffers[rank];
    while (entry != NULL) {
        if (ok_tag(entry->tag, tag) && entry->count == count) {
            prev->next = entry->next;
            ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.buffers[rank]));
            return entry;
        }
        prev = entry;
        entry = entry->next;
    }
    //ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.buffers[rank]));
    return NULL;
}

// interface functions

void MIMPI_Init(bool enable_deadlock_detection) {
    channels_init();

    char* n_str = getenv("MIMPI_N");
    char* rank_str = getenv("MIMPI_RANK");
    global.rank = atoi(rank_str);
    global.size = atoi(n_str);

    // initialize buffers
    global.buffers = calloc(global.size, sizeof(buffer_entry));

    // initialize mutexes
    ASSERT_SYS_OK(pthread_mutex_init(&global.mutex.state, NULL));
    global.mutex.buffers = calloc(global.size, sizeof(pthread_mutex_t));
    for (int p = 0; p < global.size; p++) {
        ASSERT_SYS_OK(pthread_mutex_init(&global.mutex.buffers[p], NULL));
    }

    // initialize buffer update
    global.buffer_update = calloc(global.size, sizeof(buffer_update_t));
    for (int p = 0; p < global.size; p++) {
        ASSERT_SYS_OK(pthread_cond_init(&global.buffer_update[p].cond, NULL));
        global.buffer_update[p].tag = NOWAIT_TAG;
    }

    // start buffer threads
    global.threads = calloc(global.size, sizeof(pthread_t));
    for (int p = 0; p < global.size; p++) {
        if (p == global.rank) continue;
        int* rank = malloc(sizeof(int));
        *rank = p;
        ASSERT_SYS_OK(pthread_create(&global.threads[p], NULL, buffer_thread, rank));
    }
}

void MIMPI_Finalize() {
    global.finishing = true;
    
    // send DIE_TAG to all processes
    packet_header die_packet = {.tag = DIE_TAG, .count = 0};
    for (int p = 0; p < global.size; p++) {
        if (p == global.rank) continue;
        int fd = 20 + p;
        int res = chsend(fd, &die_packet, sizeof(packet_header));
        if (res < 0 && errno != EPIPE)
            ASSERT_SYS_OK(res);
    }

    // wait for buffer threads
    for (int p = 0; p < global.size; p++) {
        if (p == global.rank) continue;
        ASSERT_SYS_OK(pthread_join(global.threads[p], NULL));
    }
    free(global.threads);

    // close all pipes
    for (int p = 0; p < global.size; p++) {
        if (p == global.rank) continue;
        ASSERT_SYS_OK(close(20 + p));
        ASSERT_SYS_OK(close(40 + p));
    }

    // destroy mutexes & conditions
    ASSERT_SYS_OK(pthread_mutex_destroy(&global.mutex.state));
    for (int p = 0; p < global.size; p++) {
        ASSERT_SYS_OK(pthread_mutex_destroy(&global.mutex.buffers[p]));
        ASSERT_SYS_OK(pthread_cond_destroy(&global.buffer_update[p].cond));
    }
    free(global.mutex.buffers);
    free(global.buffer_update);

    // free memory
    for (int p = 0; p < global.size; p++) {
        if (p == global.rank) continue;
        buffer_entry* entry = global.buffers[p].next;
        while (entry != NULL) {
            buffer_entry* next = entry->next;
            free(entry->data);
            free(entry);
            entry = next;
        }
    }
    free(global.buffers);

    channels_finalize();
}

int MIMPI_World_size() {
    return global.size;
}

int MIMPI_World_rank() {
    return global.rank;
}

MIMPI_Retcode MIMPI_Send(
    void const *data,
    int count,
    int destination,
    int tag
) {
    if (destination == MIMPI_World_rank()) 
        return MIMPI_ERROR_ATTEMPTED_SELF_OP;
    if (destination < 0 || destination >= MIMPI_World_size())
        return MIMPI_ERROR_NO_SUCH_RANK;
    if (is_proc_done(destination))
        return MIMPI_ERROR_REMOTE_FINISHED;

    int fd = 20 + destination;

    // send header
    packet_header header;
    header.tag = tag;
    header.count = count;
    int res = chsend(fd, &header, sizeof(header));
    if ((res < 0 && errno == EPIPE) || (res >= 0 && res < sizeof(header)))
        return MIMPI_ERROR_REMOTE_FINISHED;
    ASSERT_SYS_OK(res);

    // send data in 512-byte blocks
    int offset = 0;
    while (offset < count) {
        int block_count = min(count - offset, 512);
        res = chsend(fd, data + offset, block_count);
        if ((res < 0 && errno == EPIPE) || (res >= 0 && res < block_count))
            return MIMPI_ERROR_REMOTE_FINISHED;
        ASSERT_SYS_OK(res);
        offset += block_count;
    }


    return MIMPI_SUCCESS;
}

MIMPI_Retcode MIMPI_Recv(
    void *data,
    int count,
    int source,
    int tag
) {
    if (source == MIMPI_World_rank())
        return MIMPI_ERROR_ATTEMPTED_SELF_OP;
    if (source < 0 || source >= MIMPI_World_size())
        return MIMPI_ERROR_NO_SUCH_RANK;

    ASSERT_SYS_OK(pthread_mutex_lock(&global.mutex.buffers[source]));
    global.buffer_update[source].tag = tag;
    global.buffer_update[source].count_target = count;
    global.buffer_update[source].found = false;

    buffer_entry* entry = pop(source, tag, count);

    while (entry == NULL && !is_proc_done(source)) {
        ASSERT_SYS_OK(pthread_cond_wait(&global.buffer_update[source].cond, &global.mutex.buffers[source]));
        if (global.buffer_update[source].found) // suppress spurious wakeups
            entry = pop(source, tag, count);
    }

    global.buffer_update[source].tag = NOWAIT_TAG;

    ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.buffers[source]));

    memcpy(data, entry->data, entry->count);
    free(entry->data);
    free(entry);

    return MIMPI_SUCCESS;
}

MIMPI_Retcode MIMPI_Barrier() {

    ASSERT_SYS_OK(pthread_mutex_lock(&global.mutex.state));
    if (global.states != 0) {
        ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.state));
        return MIMPI_ERROR_REMOTE_FINISHED;
    }
    ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.state));

    return MIMPI_SUCCESS;
}

MIMPI_Retcode MIMPI_Bcast(
    void *data,
    int count,
    int root
) {
    
    return MIMPI_SUCCESS;
}

MIMPI_Retcode MIMPI_Reduce(
    void const *send_data,
    void *recv_data,
    int count,
    MIMPI_Op op,
    int root
) {
    return MIMPI_SUCCESS;
}