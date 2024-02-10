#include <malloc.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "HazardPointer.h"
#include "LLQueue.h"

struct LLNode;
typedef struct LLNode LLNode;
typedef _Atomic(LLNode*) AtomicLLNodePtr;

struct LLNode {
    AtomicLLNodePtr next;
    Value item;
};

LLNode* LLNode_new(Value item)
{
    LLNode* node = (LLNode*)malloc(sizeof(LLNode));
    atomic_init(&node->next, NULL);
    node->item = item;
    return node;
}

struct LLQueue {
    AtomicLLNodePtr head;
    AtomicLLNodePtr tail;
    HazardPointer hp;
};

LLQueue* LLQueue_new(void)
{
    _Atomic(LLQueue*) queue = (LLQueue*)malloc(sizeof(LLQueue));

    HazardPointer_initialize(&queue->hp);

    LLNode *dummy = LLNode_new(EMPTY_VALUE);
    queue->head = dummy;
    queue->tail = dummy;
    HazardPointer_protect(&queue->hp, &dummy);

    return queue;
}

void LLQueue_delete(LLQueue* queue)
{
    while (LLQueue_pop(queue) != EMPTY_VALUE);
    HazardPointer_finalize(&queue->hp);
    free(queue);
}

void LLQueue_push(LLQueue* queue, Value item)
{
    _Atomic(LLNode*) node = LLNode_new(item);
    LLNode* tail;
    LLNode* next;

    while (true) {
        HazardPointer_protect(&queue->hp, &queue->tail);
        tail = atomic_load(&queue->tail);
        next = atomic_load(&tail->next);
        if (tail == atomic_load(&queue->tail)) {
            if (next == NULL) {
                if (atomic_compare_exchange_strong(&tail->next, &next, node)) {
                    break;
                }
            } else {
                atomic_compare_exchange_strong(&queue->tail, &tail, next);
            }
        }
    }
    atomic_compare_exchange_strong(&queue->tail, &tail, node);
}

Value LLQueue_pop(LLQueue* queue)
{
    LLNode* head;
    LLNode* tail;
    LLNode* next;

    while (true) {
        HazardPointer_protect(&queue->hp, &queue->head);
        head = atomic_load(&queue->head);
        tail = atomic_load(&queue->tail);
        HazardPointer_protect(&queue->hp, &head->next);
        next = atomic_load(&head->next);
        if (head == atomic_load(&queue->head)) {
            if (head == tail) {
                if (next == NULL) {
                    return EMPTY_VALUE;
                }
                atomic_compare_exchange_strong(&queue->tail, &tail, next);
            } else {
                Value value = next->item;
                if (atomic_compare_exchange_strong(&queue->head, &head, next)) {
                    HazardPointer_retire(&queue->hp, head);
                    return value;
                }
            }
        }
    }
}

bool LLQueue_is_empty(LLQueue* queue)
{
    LLNode* head = atomic_load(&queue->head);
    LLNode* tail = atomic_load(&queue->tail);
    LLNode* next = atomic_load(&head->next);
    return head == tail && next == NULL;
}
