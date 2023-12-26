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

struct global_t {
    int rank;
    int size;
    pthread_t* buffer_threads;
    int states;
    struct {
        pthread_mutex_t state;
        pthread_mutex_t* buffers;
    } mutex;
    pthread_cond_t* buffer_update;
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

void* buffer_thread(void* source_rank) {
    int rank = *(int*)source_rank;
    free(source_rank);
    int fd = 40 + rank;
    while (1)
    {
        packet_header header;
        int res = chrecv(fd, &header, sizeof(header));
        if ((res < 0 && errno != EPIPE) || res == 0) {
            ASSERT_SYS_OK(pthread_mutex_lock(&global.mutex.state));
            global.states |= (1 << rank);
            ASSERT_SYS_OK(pthread_cond_signal(&global.buffer_update[rank]));
            ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.state));
            break;
        }
        ASSERT_SYS_OK(res);

        void* data = malloc(header.count);
        ASSERT_SYS_OK(chrecv(fd, data, header.count));
        buffer_entry* entry = malloc(sizeof(buffer_entry));
        entry->tag = header.tag;
        entry->count = header.count;
        entry->data = data;
        entry->next = NULL;
        ASSERT_SYS_OK(pthread_mutex_lock(&global.mutex.buffers[rank]));
        buffer_entry* last = &global.buffers[rank];
        while (last->next != NULL)
            last = last->next;
        last->next = entry;
        ASSERT_SYS_OK(pthread_cond_signal(&global.buffer_update[rank]));
        ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.buffers[rank]));
    }
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

buffer_entry* pop(int rank, int tag) {
    ASSERT_SYS_OK(pthread_mutex_lock(&global.mutex.buffers[rank]));
    buffer_entry* entry = global.buffers[rank].next;
    buffer_entry* prev = &global.buffers[rank];
    while (entry != NULL) {
        if (ok_tag(entry->tag, tag)) {
            prev->next = entry->next;
            ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.buffers[rank]));
            return entry;
        }
        prev = entry;
        entry = entry->next;
    }
    ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.buffers[rank]));
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

    // initialize mutexes & conditions
    ASSERT_SYS_OK(pthread_mutex_init(&global.mutex.state, NULL));
    global.mutex.buffers = calloc(global.size, sizeof(pthread_mutex_t));
    global.buffer_update = calloc(global.size, sizeof(pthread_cond_t));
    for (int p = 0; p < global.size; p++) {
        ASSERT_SYS_OK(pthread_mutex_init(&global.mutex.buffers[p], NULL));
        ASSERT_SYS_OK(pthread_cond_init(&global.buffer_update[p], NULL));
    }
    
    // start buffer threads
    global.buffer_threads = calloc(global.size, sizeof(pthread_t));
    pthread_attr_t attr;
    ASSERT_SYS_OK(pthread_attr_init(&attr));
    // ASSERT_SYS_OK(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED));
    for (int p = 0; p < global.size; p++) {
        if (p == global.rank) continue;
        int* rank = malloc(sizeof(int));
        *rank = p;
        ASSERT_SYS_OK(pthread_create(&global.buffer_threads[p], &attr, buffer_thread, rank));
    }
    ASSERT_SYS_OK(pthread_attr_destroy(&attr));
}

void MIMPI_Finalize() {
    
    // kill buffer threads
    for (int p = 0; p < global.size; p++) {
        if (p == global.rank) continue;
        ASSERT_SYS_OK(pthread_cancel(global.buffer_threads[p]));
        ASSERT_SYS_OK(pthread_detach(global.buffer_threads[p]));
    }
    free(global.buffer_threads);

    // destroy mutexes & conditions
    ASSERT_SYS_OK(pthread_mutex_destroy(&global.mutex.state));
    for (int p = 0; p < global.size; p++) {
        ASSERT_SYS_OK(pthread_mutex_destroy(&global.mutex.buffers[p]));
        ASSERT_SYS_OK(pthread_cond_destroy(&global.buffer_update[p]));
    }
    free(global.mutex.buffers);
    free(global.buffer_update);
    
    // close all pipes
    for (int p = 0; p < global.size; p++) {
        if (p == global.rank) continue;
        ASSERT_SYS_OK(close(20 + p));
        ASSERT_SYS_OK(close(40 + p));
    }

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

    packet_header header;
    header.tag = tag;
    int fd = 20 + destination;
    int offset = 0;
    while (offset < count) {
        header.count = min(count - offset, 512 - sizeof(header));
        void* packet = malloc(sizeof(header) + header.count);
        memcpy(packet, &header, sizeof(header));
        memcpy(packet + sizeof(header), data + offset, header.count);
        ASSERT_SYS_OK(chsend(fd, packet, sizeof(header) + header.count));
        offset += header.count;
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
    int buffered = buf_size(source, tag);

    while (buffered < count) {
        if (is_proc_done(source)) {
            ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.buffers[source]));
            return MIMPI_ERROR_REMOTE_FINISHED;
        }
        ASSERT_SYS_OK(pthread_cond_wait(&global.buffer_update[source], &global.mutex.buffers[source]));
        buffered = buf_size(source, tag);
    }


    ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.buffers[source]));

    int offset = 0;
    while (offset < count) {
        buffer_entry* entry = pop(source, tag);
        memcpy(data + offset, entry->data, min(entry->count, count - offset));
        if (entry->count > count - offset) {
            buffer_entry* new_entry = malloc(sizeof(buffer_entry));
            new_entry->tag = entry->tag;
            new_entry->count = entry->count - (count - offset);
            new_entry->data = malloc(new_entry->count);
            memcpy(new_entry->data, entry->data + (count - offset), new_entry->count);
            ASSERT_SYS_OK(pthread_mutex_lock(&global.mutex.buffers[source]));
            new_entry->next = global.buffers[source].next;
            global.buffers[source].next = new_entry;
            ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.buffers[source]));
        }
        offset += min(entry->count, count - offset);
        free(entry->data);
        free(entry);
    }

    return MIMPI_SUCCESS;
}

MIMPI_Retcode MIMPI_Barrier() {
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