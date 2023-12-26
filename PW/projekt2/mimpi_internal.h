#ifndef MIMPI_INTERNAL_H
#define MIMPI_INTERNAL_H

#include "mimpi.h"

bool is_proc_done(int rank);

typedef struct buffer_entry {
    int tag;
    int count;
    void* data;
    struct buffer_entry* next;
} buffer_entry;

int buf_size(int rank, int tag);

#endif