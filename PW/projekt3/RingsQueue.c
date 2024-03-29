#include <malloc.h>
#include <pthread.h>
#include <stdatomic.h>

#include <stdio.h>
#include <stdlib.h>

#include "HazardPointer.h"
#include "RingsQueue.h"

struct RingsQueueNode;
typedef struct RingsQueueNode RingsQueueNode;

struct RingsQueueNode {
    _Atomic(RingsQueueNode*) next;
    Value* buffer;
    atomic_int push_idx;
    atomic_int pop_idx;
};

struct RingsQueue {
    RingsQueueNode* head;
    RingsQueueNode* tail;
    pthread_mutex_t pop_mtx;
    pthread_mutex_t push_mtx;
};

RingsQueueNode* RingsQueueNode_new(void)
{
    RingsQueueNode* node = (RingsQueueNode*)malloc(sizeof(RingsQueueNode));
    atomic_init(&node->next, NULL);
    node->buffer = (Value*)calloc(RING_SIZE, sizeof(Value));
    node->push_idx = 0;
    node->pop_idx = 0;
    return node;
}

RingsQueue* RingsQueue_new(void)
{
    RingsQueue* queue = (RingsQueue*)malloc(sizeof(RingsQueue));

    pthread_mutex_init(&queue->pop_mtx, NULL);
    pthread_mutex_init(&queue->push_mtx, NULL);

    RingsQueueNode *dummy = (RingsQueueNode*)malloc(sizeof(RingsQueueNode));
    dummy->next = NULL;
    dummy->buffer = NULL;
    dummy->push_idx = 0;
    dummy->pop_idx = 0;

    queue->head = dummy;
    queue->tail = dummy;

    return queue;
}

void RingsQueue_delete(RingsQueue* queue)
{
    pthread_mutex_destroy(&queue->pop_mtx);
    pthread_mutex_destroy(&queue->push_mtx);

    for (RingsQueueNode* node = queue->head; node != NULL;) {
        RingsQueueNode* next = node->next;
        free(node->buffer);
        free(node);
        node = next;
    }

    free(queue);
}

void RingsQueue_push(RingsQueue* queue, Value item)
{
    pthread_mutex_lock(&queue->push_mtx);
    RingsQueueNode* tail = queue->tail;
    int push_idx = atomic_load(&tail->push_idx);
    if (push_idx == RING_SIZE || tail->buffer == NULL) {
        RingsQueueNode* new_tail = RingsQueueNode_new();
        atomic_store(&tail->next, new_tail);
        queue->tail = new_tail;
        tail = new_tail;
        push_idx = 0;
    }
    tail->buffer[push_idx] = item;
    atomic_store(&tail->push_idx, push_idx + 1);
    pthread_mutex_unlock(&queue->push_mtx);

}

Value RingsQueue_pop(RingsQueue* queue)
{
    pthread_mutex_lock(&queue->pop_mtx);
    RingsQueueNode* head = queue->head;
    RingsQueueNode* new_head = atomic_load(&head->next);

    if (new_head == NULL) {
        pthread_mutex_unlock(&queue->pop_mtx);
        return EMPTY_VALUE;
    } else if (atomic_load(&new_head->pop_idx) >= atomic_load(&new_head->push_idx)) {
        pthread_mutex_unlock(&queue->pop_mtx);
        return EMPTY_VALUE;
    } else if (atomic_load(&new_head->pop_idx) + 1 == RING_SIZE) {
        Value* buffer = new_head->buffer;
        new_head->buffer = NULL;
        queue->head = new_head;
        pthread_mutex_unlock(&queue->pop_mtx);
        Value item = buffer[RING_SIZE - 1];
        free(buffer);
        free(head);
        return item;
    } else {
        int pop_idx = atomic_load(&new_head->pop_idx);
        if (pop_idx == RING_SIZE) {
            pthread_mutex_unlock(&queue->pop_mtx);
            return EMPTY_VALUE;
        }
        Value item = new_head->buffer[pop_idx++];
        atomic_store(&new_head->pop_idx, pop_idx);
        pthread_mutex_unlock(&queue->pop_mtx);
        return item;
    }
}

bool RingsQueue_is_empty(RingsQueue* queue)
{
    pthread_mutex_lock(&queue->pop_mtx);
    RingsQueueNode* head = atomic_load(&queue->head->next);
    pthread_mutex_unlock(&queue->pop_mtx);
    int pop_idx = (head == NULL) ? RING_SIZE : atomic_load(&head->pop_idx);
    return pop_idx == RING_SIZE;
}
