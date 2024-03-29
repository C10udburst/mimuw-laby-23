#include <malloc.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "BLQueue.h"
#include "HazardPointer.h"

struct BLNode;
typedef struct BLNode BLNode;
typedef _Atomic(BLNode*) AtomicBLNodePtr;

struct BLNode {
    AtomicBLNodePtr next;
    Value* buffer;
    atomic_int push_idx;
    atomic_int pop_idx;
};

struct BLQueue {
    AtomicBLNodePtr head;
    AtomicBLNodePtr tail;
    HazardPointer hp;
};

struct BLNode* BLNode_new(void)
{
    BLNode* node = (BLNode*)malloc(sizeof(BLNode));
    atomic_init(&node->next, NULL);
    node->buffer = (Value*)calloc(BUFFER_SIZE, sizeof(Value));
    node->push_idx = 0;
    node->pop_idx = 0;
    return node;
}

BLQueue* BLQueue_new(void)
{
    BLQueue* queue = (BLQueue*)malloc(sizeof(BLQueue));

    HazardPointer_initialize(&queue->hp);

    BLNode *dummy = BLNode_new();
    dummy->push_idx = BUFFER_SIZE;
    dummy->pop_idx = BUFFER_SIZE;
    queue->head = dummy;
    queue->tail = dummy;

    return queue;
}

void BLQueue_delete(BLQueue* queue)
{
    BLNode* next = atomic_load(&queue->head);
    while (next != NULL) {
        BLNode* tmp = next;
        next = atomic_load(&next->next);
        free(tmp->buffer);
        free(tmp);
    }
    HazardPointer_finalize(&queue->hp);
    free(queue);
}

void BLQueue_push(BLQueue* queue, Value item)
{
    BLNode* tail;
    BLNode* next;

    while (true) {
        tail = atomic_load(&queue->tail);
        HazardPointer_protect(&queue->hp, &tail);
        atomic_int idx = atomic_fetch_add(&tail->push_idx, 1);
        if (idx + 1 < BUFFER_SIZE) {
            tail->buffer[idx] = item;
            return;
        } else
            atomic_store(&tail->push_idx, BUFFER_SIZE);
        next = atomic_load(&tail->next);
        HazardPointer_protect(&queue->hp, &next);
        if (tail == atomic_load(&queue->tail)) {
            if (next == NULL) {
                BLNode* node = BLNode_new();
                node->buffer[0] = item;
                node->push_idx = 1;
                if (atomic_compare_exchange_strong(&tail->next, &next, node)) {
                    break;
                }
            } else {
                atomic_compare_exchange_strong(&queue->tail, &tail, next);
            }
        }
    }
}

Value BLQueue_pop(BLQueue* queue)
{
    BLNode * head;
    BLNode * tail;
    BLNode * next;

    while (true) {
        head = atomic_load(&queue->head);
        atomic_int idx = atomic_fetch_add(&head->pop_idx, 1);
        if (idx + 1 < BUFFER_SIZE) {
            return head->buffer[idx];
        } else
            atomic_store(&head->pop_idx, BUFFER_SIZE);
        tail = atomic_load(&queue->tail);
        next = atomic_load(&head->next);
        if (head == atomic_load(&queue->head)) {
            if (head == tail) {
                if (next == NULL)
                    return EMPTY_VALUE;
                atomic_compare_exchange_strong(&queue->tail, &tail, next);
            } else {
                if (atomic_compare_exchange_strong(&queue->head, &head, next)) {
                    HazardPointer_retire(&queue->hp, head);
                    continue;
                }
            }
        }
    }
}

bool BLQueue_is_empty(BLQueue* queue)
{
    BLNode* head = atomic_load(&queue->head);
    BLNode* tail = atomic_load(&queue->tail);
    BLNode* next = atomic_load(&head->next);
    return head == tail && next == NULL;
}
