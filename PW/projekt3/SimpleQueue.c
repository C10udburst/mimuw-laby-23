#include <malloc.h>
#include <pthread.h>

#include "HazardPointer.h"
#include "SimpleQueue.h"

struct SimpleQueueNode;
typedef struct SimpleQueueNode SimpleQueueNode;

struct SimpleQueueNode {
    SimpleQueueNode* next;
    Value item;
};

SimpleQueueNode* SimpleQueueNode_new(Value item)
{
    SimpleQueueNode* node = (SimpleQueueNode*)malloc(sizeof(SimpleQueueNode));
    node->next = NULL;
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

    bool lock_head = false;
    pthread_mutex_lock(&queue->head_mtx);
    pthread_mutex_lock(&queue->tail_mtx);
    if (queue->head == queue->tail || queue->head->next == queue->tail)
        lock_head = true;
    else 
        pthread_mutex_unlock(&queue->head_mtx);
    queue->tail->next = node;
    queue->tail = node;
    pthread_mutex_unlock(&queue->tail_mtx);
    if (lock_head)
        pthread_mutex_unlock(&queue->head_mtx);
}

Value SimpleQueue_pop(SimpleQueue* queue)
{
    Value item = EMPTY_VALUE;

    pthread_mutex_lock(&queue->head_mtx);
    SimpleQueueNode* node = queue->head->next;
    if (node != NULL) {
        item = node->item;
        queue->head = node;
    }
    pthread_mutex_unlock(&queue->head_mtx);

    return item;
}

bool SimpleQueue_is_empty(SimpleQueue* queue)
{

    pthread_mutex_lock(&queue->head_mtx);
    SimpleQueueNode* node = queue->head->next;
    pthread_mutex_unlock(&queue->head_mtx);

    return node == NULL;
}
