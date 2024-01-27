#include <malloc.h>
#include <pthread.h>
#include <stdatomic.h>

#include "SimpleQueue.h"

struct SimpleQueueNode;
typedef struct SimpleQueueNode SimpleQueueNode;

struct SimpleQueueNode {
    _Atomic(SimpleQueueNode*) next;
    Value item;
};

SimpleQueueNode* SimpleQueueNode_new(Value item)
{
    SimpleQueueNode* node = (SimpleQueueNode*)malloc(sizeof(SimpleQueueNode));
    atomic_init(&node->next, NULL);
    node->item = item;
    return node;
}

struct SimpleQueue {
    SimpleQueueNode* head;
    SimpleQueueNode* tail;
    pthread_mutex_t head_mtx;
    pthread_mutex_t tail_mtx;
};

SimpleQueue* SimpleQueue_new(void)
{
    SimpleQueue* queue = (SimpleQueue*)malloc(sizeof(SimpleQueue));
    
    // init mutexes
    pthread_mutex_init(&queue->head_mtx, NULL);
    pthread_mutex_init(&queue->tail_mtx, NULL);

    // add dummy boundary node
    SimpleQueueNode* dummy = SimpleQueueNode_new(EMPTY_VALUE);
    queue->head = dummy;
    queue->tail = dummy;

    return queue;
}

void SimpleQueue_delete(SimpleQueue* queue)
{

    // destroy mutexes
    pthread_mutex_destroy(&queue->head_mtx);
    pthread_mutex_destroy(&queue->tail_mtx);

    // free all nodes
    SimpleQueueNode* node = queue->head;
    SimpleQueueNode* tmp;
    while (node != NULL) {
        tmp = node;
        node = node->next;
        free(tmp);
    }

    free(queue);
}

void SimpleQueue_push(SimpleQueue* queue, Value item)
{
    SimpleQueueNode* node = SimpleQueueNode_new(item);

    pthread_mutex_lock(&queue->tail_mtx);
    atomic_store(&queue->tail->next, node);
    queue->tail = node;
    pthread_mutex_unlock(&queue->tail_mtx);
}

Value SimpleQueue_pop(SimpleQueue* queue)
{
    Value item;

    pthread_mutex_lock(&queue->head_mtx);
    SimpleQueueNode* head = atomic_load(&queue->head);
    SimpleQueueNode* new_head = atomic_load(&head->next);
    if (new_head == NULL) {
        pthread_mutex_unlock(&queue->head_mtx);
        return EMPTY_VALUE;
    }
    item = new_head->item;
    atomic_store(&queue->head, new_head);
    pthread_mutex_unlock(&queue->head_mtx);
    free(head);

    return item;
}

bool SimpleQueue_is_empty(SimpleQueue* queue)
{

    pthread_mutex_lock(&queue->head_mtx);
    SimpleQueueNode* node = atomic_load(&queue->head->next);
    pthread_mutex_unlock(&queue->head_mtx);

    return node == NULL;
}
