/**
 * This file is for implementation of MIMPI library.
 * */

#include <stdlib.h>
#include <stdio.h>
#include "channel.h"
#include "mimpi.h"
#include "mimpi_common.h"

struct {
    int rank;
    int size;
} global;

void MIMPI_Init(bool enable_deadlock_detection) {
    channels_init();

    char* n_str = getenv("MIMPI_N");
    char* rank_str = getenv("MIMPI_RANK");
    global.rank = atoi(rank_str);
    global.size = atoi(n_str);

    // TODO
}

void MIMPI_Finalize() {
    // TODO

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