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

// internal functions

int get_proc_state(int rank) {
    mimpi_master_req req;
    req.req_type = MIMPI_REQ_GET_STATE;
    req.req_data.get_state.rank = rank;
    req.req_data.get_state.my_rank = global.rank;
    ASSERT_SYS_OK(chsend(60, &req, sizeof(req)));

    mimpi_master_res res;
    ASSERT_SYS_OK(chrecv(80, &res, sizeof(res)));
    return res.res_data.get_state.state;
}

// interface functions

void MIMPI_Init(bool enable_deadlock_detection) {
    channels_init();

    char* n_str = getenv("MIMPI_N");
    char* rank_str = getenv("MIMPI_RANK");
    global.rank = atoi(rank_str);
    global.size = atoi(n_str);

    // notify master that we are inside MIMPI
    mimpi_master_req req;
    req.req_type = MIMPI_REQ_SET_STATE;
    req.req_data.set_state.rank = global.rank;
    req.req_data.set_state.state = MIMPI_PROC_INSIDE;
    ASSERT_SYS_OK(chsend(60, &req, sizeof(req)));
}

void MIMPI_Finalize() {

    // notify master that we are after MIMPI
    mimpi_master_req req;
    req.req_type = MIMPI_REQ_SET_STATE;
    req.req_data.set_state.rank = global.rank;
    req.req_data.set_state.state = MIMPI_PROC_AFTER;
    ASSERT_SYS_OK(chsend(60, &req, sizeof(req)));
    
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
    if (get_proc_state(destination) == MIMPI_PROC_AFTER)
        return MIMPI_ERROR_REMOTE_FINISHED;

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

    // TODO: check buffer
    int state = get_proc_state(source);
    if (state == MIMPI_PROC_AFTER)
        return MIMPI_ERROR_REMOTE_FINISHED;
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