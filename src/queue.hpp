#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <cstddef>
#include <cassert>
#include "thread_manager.hpp"

class Thread;

class FifoQueue {
public:
    explicit FifoQueue(unsigned capacity);

    ~FifoQueue();

    // Check if the node is already in the queue
    static bool node_in_queue(const Thread *node);

    // Returns the node at the top of the queue and removes it from the queue, or NULL if the queue is empty
    Thread *pop();

    // Returns the node at the top of the queue without popping it, or NULL if the queue is empty
    [[nodiscard]] Thread *top() const;

    // Insert the node to the end of the queue, returns 0 on success, -1 if the queue is at capacity
    int push(Thread *node);

    // Insert the node to the queue in sorted order based on priority, returns 0 on success, -1 if the queue is at capacity
    int push_sorted(Thread *node);

    // Returns the node in the queue with the given id and removes it from the queue, or NULL if not found
    Thread *remove(int id);

    // Returns the number of items currently in the queue
    [[nodiscard]] unsigned count() const;

private:
    Thread *first;
    Thread *last;
    unsigned len;
    unsigned capacity;
};

#endif // QUEUE_HPP
