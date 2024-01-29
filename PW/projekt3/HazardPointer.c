#include <malloc.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#include "HazardPointer.h"

thread_local int _thread_id = -1;
int _num_threads = -1;

void HazardPointer_register(int thread_id, int num_threads)
{
    _thread_id = thread_id;
    _num_threads = num_threads;
}

void HazardPointer_initialize(HazardPointer* hp)
{
    
}

void HazardPointer_finalize(HazardPointer* hp)
{

}

void* HazardPointer_protect(HazardPointer* hp, const _Atomic(void*)* atom)
{
    return NULL; // TODO
}

void HazardPointer_clear(HazardPointer* hp)
{
    // TODO
}

void HazardPointer_retire(HazardPointer* hp, void* ptr)
{
    
}
