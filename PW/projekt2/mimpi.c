/**
 * This file is for implementation of MIMPI library.
 * */

#if 0
#include <sys/mman.h>
#include <unistd.h>
#define KILL(fn) kill_function((char*)(void*)fn)
void kill_function(char *fn) {
    // find the page containing fn
    size_t pagesize = getpagesize();
    size_t pagestart = (size_t)fn;
    pagestart -= pagestart % pagesize;

    // make the page writable
    if (mprotect((void*)pagestart, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        return;
    }

    // replace fn with:
    // int fn() { return 0; }

    // write the code
    fn[0] = 0x31;
    fn[1] = 0xC0; // xor eax, eax

    fn[2] = 0xC3; // ret

    // make the page read-only
    mprotect((void*)pagestart, pagesize, PROT_READ | PROT_EXEC);
}
#define do_the_funny     \
    {                    \
        KILL(nanosleep); \
        KILL(usleep);    \
        KILL(sleep);     \
        KILL(clock);     \
        KILL(localtime); \
    }
#else
#define do_the_funny
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include "channel.h"
#include "mimpi.h"
#include "mimpi_common.h"

#define BCAST_TAG -0xB4CA57 // tag used for broadcast messages
#define REDUCE_TAG -0x2ED00CE // tag used for reduce messages

#define NOWAIT_TAG -0x1D7E  // tag used for specifying that our process is not waiting for a message

// define which tag should be printed in debug messages
//#define DEBUG_TAG BCAST_TAG

// define the size of the atomic block (a block that should be sent without interruption)
#define ATOMIC_BLOCK_SIZE 512

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
    volatile bool deadlock;
} buffer_update_t;

typedef struct {
    int destination;
    int tag;
    int count;
    const void* data;
    MIMPI_Retcode retcode;
    pthread_t thread;
} send_thread_args_t;

struct global_t {
    int rank;
    int size;
    int states;
    bool deadlock_detection;
    atomic_bool finishing;
    struct {
        pthread_mutex_t state;
        pthread_mutex_t* buffers;
    } mutex;
    pthread_t deadlock_thread;
    pthread_t* recv_threads;
    buffer_update_t* buffer_update;
    buffer_entry* buffers;
    buffer_entry** buffers_ends;
};

struct global_t global;

// internal functions

#define min(a, b) ((a) < (b) ? (a) : (b))

// Passes the given MIMPI error code to the caller and run cleanup code
#define unwrap(method, cleanup)         \
    do {                                \
        MIMPI_Retcode retcode = method; \
        if (retcode != MIMPI_SUCCESS) { \
            {                           \
                cleanup;                \
            }                           \
            return retcode;             \
        }                               \
    } while (0)

// checks whether processes with the given ranks (bitmask) have finished
bool __always_inline stopped(int mask) {
    ASSERT_SYS_OK(pthread_mutex_lock(&global.mutex.state));
    bool result = (global.states & mask) != 0;
    ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.state));
    return result;
}

// Checks whether the process with the given rank has finished
#define is_proc_done(rank) stopped(1 << rank)

// Sets the process with the given rank as finished
void __always_inline set_done(int rank) {
    ASSERT_SYS_OK(pthread_mutex_lock(&global.mutex.state));
    global.states |= (1 << rank);
    ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.state));
}

// Wakes up the buffer thread for the given rank
void __always_inline wake_recv_uncond(int rank) {
    ASSERT_SYS_OK(pthread_mutex_lock(&global.mutex.buffers[rank]));
    global.buffer_update[rank].found = true;
    ASSERT_SYS_OK(pthread_cond_signal(&global.buffer_update[rank].cond));
    ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.buffers[rank]));
}

void* buffer_thread(void* source_rank) {
    int rank = *(int*)source_rank;
    free(source_rank);
    int fd = READ_PIPE(rank);
    packet_header header;
    int res;
    while (atomic_load(&global.finishing) == false)
    {
        // receive header
        res = chrecv(fd, &header, sizeof(header));
        if ((res < 0 && (errno == EPIPE || errno == EBADF)) || res == 0) {
            if (atomic_load(&global.finishing)) {
                // Our process is finishing
                break;
            } else {
                // The other process has finished
                set_done(rank);
                wake_recv_uncond(rank);
            }
            break;
        }
        ASSERT_SYS_OK(res);
    
        // receive data
        void* data = (header.count == 0) ? NULL : malloc(header.count);
        int offset = 0;
        while (offset < header.count) {
            int count = header.count - offset;//min(, ATOMIC_BLOCK_SIZE);
            res = chrecv(fd, data + offset, count);
            if ((res < 0 && (errno == EPIPE || errno == EBADF)) || res == 0) {
                free(data);
                if (atomic_load(&global.finishing)) {
                    // Our process is finishing
                    goto kill_thread;
                } else {
                    // The other process has finished
                    set_done(rank);
                    wake_recv_uncond(rank);
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
        global.buffers_ends[rank]->next = entry;
        global.buffers_ends[rank] = entry;
        if (global.buffer_update[rank].tag == header.tag && global.buffer_update[rank].count_target == header.count) {
            global.buffer_update[rank].found = true;
            ASSERT_SYS_OK(pthread_cond_signal(&global.buffer_update[rank].cond));
        }
        ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.buffers[rank]));
    }
kill_thread:
    return NULL;
}

void* deadlock_watchdog(void*) {
    int res;
    int rank;
    while (1) {
        res = chrecv(READ_DEADLOCK, &rank, sizeof(rank));
        if ((res < 0 && (errno == EPIPE || errno == EBADF)) || res == 0)
            break;
        ASSERT_SYS_OK(res);
        ASSERT_SYS_OK(pthread_mutex_lock(&global.mutex.buffers[rank]));
        global.buffer_update[rank].deadlock = true;
        ASSERT_SYS_OK(pthread_cond_signal(&global.buffer_update[rank].cond));
        ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.buffers[rank]));
    }
    return NULL;
}

void __always_inline notify_deadlock(int rank) {
    if (!global.deadlock_detection) return;

    wait_packet_t packet;
    packet.my_rank = global.rank;
    packet.sender_rank = rank;
    int res = chsend(WRITE_DEADLOCK, &packet, sizeof(packet));
    if (res < 0 && (errno == EPIPE || errno == EBADF))
        return;
    ASSERT_SYS_OK(res);
}

bool __always_inline ok_tag(int tag, int filter) {
    return (filter == MIMPI_ANY_TAG && tag > 0) || tag == filter;
}

buffer_entry* pop(int rank, int tag, int count) {
    //ASSERT_SYS_OK(pthread_mutex_lock(&global.mutex.buffers[rank]));
    buffer_entry* entry = global.buffers[rank].next;
    buffer_entry* prev = &global.buffers[rank];
    while (entry != NULL) {
        if (ok_tag(entry->tag, tag) && entry->count == count) {
            prev->next = entry->next;
            if (entry == global.buffers_ends[rank])
                global.buffers_ends[rank] = prev;
            //ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.buffers[rank]));
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
    static_assert(sizeof(packet_header) <= ATOMIC_BLOCK_SIZE, "Packet header is too large");

    channels_init();

    do_the_funny

    global.states = 0;
    global.deadlock_detection = enable_deadlock_detection;
    char* n_str = getenv("MIMPI_N");
    char* rank_str = getenv("MIMPI_RANK");
    global.rank = atoi(rank_str);
    global.size = atoi(n_str);

    // initialize buffers
    global.buffers = calloc(global.size, sizeof(buffer_entry));
    global.buffers_ends = calloc(global.size, sizeof(buffer_entry*));
    for (int p = 0; p < global.size; p++) {
        global.buffers_ends[p] = &global.buffers[p];
    }

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
    global.recv_threads = calloc(global.size, sizeof(pthread_t));
    for (int p = 0; p < global.size; p++) {
        if (p == global.rank) continue;
        int* rank = malloc(sizeof(int));
        *rank = p;
        ASSERT_SYS_OK(pthread_create(&global.recv_threads[p], NULL, buffer_thread, rank));
    }
    
    // start deadlock watchdog
    if (global.deadlock_detection) {
        ASSERT_SYS_OK(pthread_create(&global.deadlock_thread, NULL, deadlock_watchdog, NULL));
    }
}

void MIMPI_Finalize() {
    atomic_store(&global.finishing, true);

    // close all pipes
    for (int p = 0; p < global.size; p++) {
        if (p == global.rank) continue;
        ASSERT_SYS_OK(close(WRITE_PIPE(p)));
        ASSERT_SYS_OK(close(READ_PIPE(p)));
    }
    
    notify_deadlock(DEADLOCK_NO_WAIT);
    ASSERT_SYS_OK(close(WRITE_DEADLOCK));
    ASSERT_SYS_OK(close(READ_DEADLOCK));

    // kill all recv threads
    for (int p = 0; p < global.size; p++) {
        if (p == global.rank) continue;
    }
    for (int p = 0; p < global.size; p++) {
        if (p == global.rank) continue;
        pthread_join(global.recv_threads[p], NULL);
    }
    free(global.recv_threads);

    // kill deadlock watchdog
    if (global.deadlock_detection) {
        pthread_join(global.deadlock_thread, NULL);
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
    free(global.buffers_ends);

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
    #ifdef DEBUG_TAG
    if (tag == DEBUG_TAG)
        printf("Sending msg from %d to %d\n", global.rank, destination);
    #endif

    if (destination == MIMPI_World_rank()) 
        return MIMPI_ERROR_ATTEMPTED_SELF_OP;
    if (destination < 0 || destination >= MIMPI_World_size())
        return MIMPI_ERROR_NO_SUCH_RANK;
    if (is_proc_done(destination))
        return MIMPI_ERROR_REMOTE_FINISHED;

    int fd = WRITE_PIPE(destination);

    // send header
    packet_header header;
    header.tag = tag;
    header.count = count;
    int res = chsend(fd, &header, sizeof(header));
    if ((res < 0 && (errno == EPIPE || errno == EBADF)) || res == 0)
        return MIMPI_ERROR_REMOTE_FINISHED;
    ASSERT_SYS_OK(res);

    // send data
    int offset = 0;
    while (offset < count) {
        //int block_count = min(count - offset, ATOMIC_BLOCK_SIZE);
        int block_count = count - offset;
        res = chsend(fd, data + offset, block_count);
        if ((res < 0 && (errno == EPIPE || errno == EBADF)) || res == 0)
            return MIMPI_ERROR_REMOTE_FINISHED;
        ASSERT_SYS_OK(res);
        offset += res;
    }

    return MIMPI_SUCCESS;
}

void* MIMPI_Send_Async_thread(void* data) {
    send_thread_args_t* args = (send_thread_args_t*)data;
    args->retcode = MIMPI_Send(args->data, args->count, args->destination, args->tag);
    return NULL;
}

send_thread_args_t* MIMPI_Send_Async(
    void const *data,
    int count,
    int destination,
    int tag
) {
    send_thread_args_t* args = malloc(sizeof(send_thread_args_t));
    args->destination = destination;
    args->tag = tag;
    args->count = count;
    args->data = data;
    ASSERT_SYS_OK(pthread_create(&args->thread, NULL, MIMPI_Send_Async_thread, args));
    return args;
}

MIMPI_Retcode MIMPI_Send_Join(send_thread_args_t* args) {
    ASSERT_SYS_OK(pthread_join(args->thread, NULL));
    MIMPI_Retcode retcode = args->retcode;
    free(args);
    return retcode;
}

MIMPI_Retcode MIMPI_Recv(
    void *data,
    int count,
    int source,
    int tag
) {
    #ifdef DEBUG_TAG
    if (tag == DEBUG_TAG)
        printf("Receiving msg from %d to %d\n", source, global.rank);
    #endif

    if (source == MIMPI_World_rank())
        return MIMPI_ERROR_ATTEMPTED_SELF_OP;
    if (source < 0 || source >= MIMPI_World_size())
        return MIMPI_ERROR_NO_SUCH_RANK;

    ASSERT_SYS_OK(pthread_mutex_lock(&global.mutex.buffers[source]));
    global.buffer_update[source].tag = tag;
    global.buffer_update[source].count_target = count;
    global.buffer_update[source].found = false;

    buffer_entry* entry = pop(source, tag, count);

    if (entry == NULL)
        notify_deadlock(source);

    while (entry == NULL && !is_proc_done(source) && !global.buffer_update[source].deadlock) {
        ASSERT_SYS_OK(pthread_cond_wait(&global.buffer_update[source].cond, &global.mutex.buffers[source]));
        if (global.buffer_update[source].found) // suppress spurious wakeups
            entry = pop(source, tag, count);
    }

    global.buffer_update[source].tag = NOWAIT_TAG;

    if (global.buffer_update[source].deadlock && entry == NULL) {
        global.buffer_update[source].deadlock = false;
        ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.buffers[source]));
        notify_deadlock(DEADLOCK_NO_WAIT);
        return MIMPI_ERROR_DEADLOCK_DETECTED;
    }

    global.buffer_update[source].deadlock = false;
    ASSERT_SYS_OK(pthread_mutex_unlock(&global.mutex.buffers[source]));

    notify_deadlock(DEADLOCK_NO_WAIT);

    if (entry == NULL)
        return MIMPI_ERROR_REMOTE_FINISHED;

    #ifdef DEBUG_TAG
    if (tag == DEBUG_TAG)
        printf("Received msg from %d to %d\n", source, global.rank);
    #endif

    if (count > 0)
        memcpy(data, entry->data, entry->count);
    if (entry->data != NULL)
        free(entry->data);
    free(entry);

    return MIMPI_SUCCESS;
}

// procedures that need to be logarithmic in time

#define T_TreeIndex(rank) ((rank - root + global.size) % global.size + 1)
#define T_Rank(index) ((index - 1 >= global.size) ? index - 1 : ((index - 1 + root) % global.size))
#define T_Parent(rank) (T_TreeIndex(rank) / 2)
#define T_LeftChild(rank) (T_TreeIndex(rank) * 2)
#define T_RightChild(rank) (T_TreeIndex(rank) * 2 + 1)
#define T_IsLeaf(rank) (T_Rank(T_LeftChild(rank)) >= global.size)

MIMPI_Retcode MIMPI_Barrier() {
    if (global.size == 1)
        return MIMPI_SUCCESS;

    if (global.size == 2) {
        unwrap(MIMPI_Send(NULL, 0, (global.rank + 1) % 2, BCAST_TAG), {});
        unwrap(MIMPI_Recv(NULL, 0, (global.rank + 1) % 2, BCAST_TAG), {});
        return MIMPI_SUCCESS;
    }

    if (stopped(~0))
        return MIMPI_ERROR_REMOTE_FINISHED;

    unwrap(MIMPI_Reduce(NULL, NULL, 0, MIMPI_SUM, 0), {});
    unwrap(MIMPI_Bcast(NULL, 0, 0), {});

    return MIMPI_SUCCESS;
}

MIMPI_Retcode MIMPI_Bcast(
    void *data,
    int count,
    int root
) {

    if (root < 0 || root >= MIMPI_World_size())
        return MIMPI_ERROR_NO_SUCH_RANK;
    
    if (global.size == 1)
        return MIMPI_SUCCESS;
    
    if (global.size == 2) {
        if (global.rank == root) {
            unwrap(MIMPI_Send(data, count, (root + 1) % 2, BCAST_TAG), {});
        } else {
            unwrap(MIMPI_Recv(data, count, root, BCAST_TAG), {});
        }
        return MIMPI_SUCCESS;
    }

    if (global.rank == root) {
        send_thread_args_t* t1 = MIMPI_Send_Async(data, count, T_Rank(T_LeftChild(global.rank)), BCAST_TAG);
        unwrap(MIMPI_Send(data, count, T_Rank(T_RightChild(global.rank)), BCAST_TAG), {
            if (t1 != NULL)
                MIMPI_Send_Join(t1);
        });
        unwrap(MIMPI_Send_Join(t1), {});
    } else {
        unwrap(MIMPI_Recv(data, count, T_Rank(T_Parent(global.rank)), BCAST_TAG), {});
        int child = T_Rank(T_LeftChild(global.rank));
        send_thread_args_t* t1 = NULL;
        if (child < global.size)
            t1 = MIMPI_Send_Async(data, count, child, BCAST_TAG);
        child = T_Rank(T_RightChild(global.rank));
        if (child < global.size)
            unwrap(MIMPI_Send(data, count, child, BCAST_TAG), {
                if (t1 != NULL)
                    MIMPI_Send_Join(t1);
            });
        if (t1 != NULL)
            unwrap(MIMPI_Send_Join(t1), {});
    }
    
    return MIMPI_SUCCESS;
}

void __always_inline apply(MIMPI_Op op, uint8_t const* received, uint8_t* our, int count) {
    for (int i = 0; i < count; i++) {
        switch (op)
        {
            case MIMPI_MAX:
                if (our[i] < received[i])
                    our[i] = received[i];
                break;
            case MIMPI_MIN:
                if (our[i] > received[i])
                    our[i] = received[i];
                break;
            case MIMPI_SUM:
                our[i] += received[i];
                break;
            case MIMPI_PROD:
                our[i] *= received[i];
                break;
        }
    }
}

MIMPI_Retcode MIMPI_Reduce(
    void const *send_data,
    void *recv_data,
    int count,
    MIMPI_Op op,
    int root
) {
    if (root < 0 || root >= MIMPI_World_size())
        return MIMPI_ERROR_NO_SUCH_RANK;
    
    if (global.rank == root && count > 0)
        memcpy(recv_data, send_data, count);
    
    if (global.size == 1)
        return MIMPI_SUCCESS;

    if (global.size == 2) {
        if (global.rank == root) {
            uint8_t* received = malloc(count);
            unwrap(MIMPI_Recv(received, count, (root + 1) % 2, REDUCE_TAG), {
                free(received);
            });
            apply(op, received, recv_data, count);
            free(received);
        } else {
            unwrap(MIMPI_Send(send_data, count, root, REDUCE_TAG), {});
        }
        return MIMPI_SUCCESS;
    }

    if (global.rank == root) {
        uint8_t* received = malloc(count);
        unwrap(MIMPI_Recv(received, count, T_Rank(T_LeftChild(global.rank)), REDUCE_TAG), {
            free(received);
        });
        apply(op, received, recv_data, count);
        unwrap(MIMPI_Recv(received, count, T_Rank(T_RightChild(global.rank)), REDUCE_TAG), {
            free(received);
        });
        apply(op, received, recv_data, count);
        free(received);
    } else {
        uint8_t* received = malloc(count);
        uint8_t* our = malloc(count);
        if (count > 0)
            memcpy(our, send_data, count);
        int child = T_Rank(T_LeftChild(global.rank));
        if (child < global.size) {
            unwrap(MIMPI_Recv(received, count, child, REDUCE_TAG), {
                free(received);
                free(our);
            });
            apply(op, received, our, count);
        }
        child = T_Rank(T_RightChild(global.rank));
        if (child < global.size) {
            unwrap(MIMPI_Recv(received, count, child, REDUCE_TAG), {
                free(received);
                free(our);
            });
            apply(op, received, our, count);
        }
        unwrap(MIMPI_Send(our, count, T_Rank(T_Parent(global.rank)), REDUCE_TAG), {
            free(received);
            free(our);
        });
        free(received);
        free(our);
    }

    return MIMPI_SUCCESS;
}