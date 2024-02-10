#include <malloc.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <string.h>

#include "HazardPointer.h"

thread_local int _thread_id = -1;
int _num_threads = -1;
thread_local void* _retired[MAX_THREADS];
thread_local int _retired_count = 0;

void HazardPointer_register(int thread_id, int num_threads)
{
    _thread_id = thread_id;
    _num_threads = num_threads;
    _retired_count = 0;
}

void HazardPointer_initialize(HazardPointer* hp)
{
    for (int i = 0; i < MAX_THREADS; ++i) {
        atomic_store(&hp->pointer[i], NULL);
    }
}

void HazardPointer_finalize(HazardPointer* hp)
{
    for (int i = 0; i < MAX_THREADS; ++i) {
        atomic_store(&hp->pointer[i], NULL);
    }
    for (int i = 0; i < RETIRED_THRESHOLD; ++i) {
        free(_retired[i]);
        _retired[i] = NULL;
    }
}

void* HazardPointer_protect(HazardPointer* hp, const _Atomic(void*)* atom)
{
    if (atom == NULL) {
        return NULL;
    }
    void* ptr = atomic_load(atom);
    atomic_store(&hp->pointer[_thread_id], ptr);
    return ptr;
}

void HazardPointer_clear(HazardPointer* hp)
{
    atomic_store(&hp->pointer[_thread_id], NULL);
}

void HazardPointer_retire(HazardPointer* hp, void* ptr)
{
    if (_retired_count >= RETIRED_THRESHOLD) {
        _retired_count = 0;
        for (int i=0; i < RETIRED_THRESHOLD; i++) {
            void* rptr = _retired[i];
            _retired[i] = NULL;
            for (int j=0; j < _num_threads; j++) {
                if (rptr == atomic_load(&hp->pointer[j]))
                    goto cant_retire;
            }
            free(rptr);
            cant_retire:
        }

        _retired[0] = atomic_load(&hp->pointer[_thread_id]);
        if (_retired[0] != NULL)
            _retired_count++;
    }

    _retired[_retired_count++] = ptr;
}
