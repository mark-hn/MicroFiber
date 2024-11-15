#include <cassert>
#include "src/thread_manager.hpp"
#include "queue.hpp"

struct fifo_queue {
    node_item_t *first;     // first element of the queue
    node_item_t *last;      // last element of the queue
    unsigned len;           // number of elements of the queue
    unsigned capacity;      // capacity of the queue
};

bool node_in_queue(node_item_t * node)
{
    assert(node != nullptr);
    return node->in_queue;
}

fifo_queue_t * queue_create(unsigned capacity)
{
    if (capacity <= 0) {
        return nullptr;
    }
    auto *queue = static_cast<fifo_queue_t *>(malloc(sizeof(fifo_queue_t)));
    if (queue == nullptr) {
        return nullptr;
    }
    queue->first = nullptr;
    queue->last = nullptr;
    queue->len = 0;
    queue->capacity = capacity;
    return queue;
}

void queue_destroy(fifo_queue_t * queue)
{
    assert(queue != nullptr);
    assert(queue->first == nullptr);
    assert(queue->last == nullptr);
    assert(queue->len == 0);
    free(queue);
}

node_item_t * queue_pop(fifo_queue_t * queue)
{
    assert(queue != nullptr);
    if (queue->first == nullptr) {
        return nullptr;
    }

    node_item_t *removed = queue->first;
    queue->first = reinterpret_cast<node_item_t *>(queue->first->next);
    if (queue->first == nullptr) {
        queue->last = nullptr;
    }
    queue->len--;

    removed->in_queue = false;
    removed->next = nullptr;
    return removed;
}

node_item_t * queue_top(fifo_queue_t * queue)
{
    assert(queue != nullptr);
    if (queue->first == nullptr) {
        return nullptr;
    }
    return queue->first;
}

int queue_push(fifo_queue_t * queue, node_item_t * node)
{
    assert(queue != nullptr);
    assert(node != nullptr);
    assert(node->in_queue == false);
    if (queue->len >= queue->capacity) {
        return -1;
    }

    if (queue->first == nullptr) {
        queue->first = node;
        queue->last = node;
    }
    else {
        queue->last->next = reinterpret_cast<struct thread *>(node);
        queue->last = reinterpret_cast<node_item_t *>(queue->last->next);
    }
    queue->last->in_queue = true;
    queue->last->next = nullptr;
    queue->len++;

    return 0;
}

int queue_push_sorted(fifo_queue_t * queue, node_item_t * node)
{
    assert(queue != nullptr);
    assert(node != nullptr);
    assert(node->in_queue == false);
    if (queue->len >= queue->capacity) {
        return -1;
    }

    if (queue->first == nullptr) {
        queue->first = node;
        queue->last = node;
    }
    else {
        node_item_t *prev = nullptr;
        node_item_t *curr = queue->first;
        while (curr != nullptr) {
            if (curr->prio > node->prio) {
                break;
            }
            prev = curr;
            curr = reinterpret_cast<node_item_t *>(curr->next);
        }
        if (prev == nullptr) {
            node->next = reinterpret_cast<struct thread *>(queue->first);
            queue->first = node;
        }
        else {
            prev->next = reinterpret_cast<struct thread *>(node);
            node->next = reinterpret_cast<struct thread *>(curr);
            if (curr == nullptr) {
                queue->last = node;
            }
        }
    }
    queue->last->in_queue = true;
    queue->last->next = nullptr;
    queue->len++;

    return 0;
}

node_item_t * queue_remove(fifo_queue_t * queue, int id)
{
    assert(queue != nullptr);

    node_item_t *prev = nullptr;
    node_item_t *curr = queue->first;
    while (curr != nullptr) {
        if (curr->id == id) {
            if (prev == nullptr) {
                queue->first = reinterpret_cast<node_item_t *>(curr->next);
                if (queue->first == nullptr) {
                    queue->last = nullptr;
                }
            }
            else {
                prev->next = curr->next;
                if (curr->next == nullptr) {
                    queue->last = prev;
                }
            }
            queue->len--;
            curr->next = nullptr;
            curr->in_queue = false;
            return curr;
        }
        prev = curr;
        curr = reinterpret_cast<node_item_t *>(curr->next);
    }
    return nullptr;
}

unsigned int queue_count(fifo_queue_t * queue)
{
    return queue->len;
}
