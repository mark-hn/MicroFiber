#include "queue.hpp"
#include <stdexcept>

FifoQueue::FifoQueue(unsigned capacity) {
    if (capacity <= 0) {
        throw std::invalid_argument("Capacity must be > 0");
    }
    this->first = nullptr;
    this->last = nullptr;
    this->len = 0;
    this->capacity = capacity;
}

FifoQueue::~FifoQueue() {
    assert(first == nullptr);
    assert(last == nullptr);
    assert(len == 0);
}

bool FifoQueue::node_in_queue(const Thread *node) {
    assert(node != nullptr);
    return node->in_queue;
}

Thread *FifoQueue::pop() {
    if (first == nullptr) return nullptr;

    Thread *removed = first;
    first = first->next;
    if (first == nullptr) last = nullptr;

    len--;
    removed->in_queue = false;
    removed->next = nullptr;

    return removed;
}

Thread *FifoQueue::top() const {
    return first;
}

int FifoQueue::push(Thread *node) {
    assert(node != nullptr);
    assert(node->in_queue == false);

    if (len >= capacity) return -1;

    if (first == nullptr) {
        first = node;
        last = node;
    } else {
        last->next = node;
        last = node;
    }

    last->in_queue = true;
    last->next = nullptr;
    len++;

    return 0;
}

int FifoQueue::push_sorted(Thread *node) {
    assert(node != nullptr);
    assert(node->in_queue == false);

    if (len >= capacity) return -1;

    if (first == nullptr) {
        first = last = node;
    } else {
        Thread *prev = nullptr;
        Thread *curr = first;
        while (curr != nullptr && curr->prio <= node->prio) {
            prev = curr;
            curr = curr->next;
        }

        if (prev == nullptr) {
            node->next = first;
            first = node;
        } else {
            prev->next = node;
            node->next = curr;
            if (curr == nullptr) {
                last = node;
            }
        }
    }

    node->in_queue = true;
    len++;
    return 0;
}

Thread *FifoQueue::remove(int id) {
    Thread *prev = nullptr;
    Thread *curr = first;

    while (curr != nullptr) {
        if (curr->id == id) {
            if (prev == nullptr) {
                first = curr->next;
                if (first == nullptr) last = nullptr;
            } else {
                prev->next = curr->next;
                if (curr->next == nullptr) last = prev;
            }
            curr->next = nullptr;
            curr->in_queue = false;
            len--;
            return curr;
        }
        prev = curr;
        curr = curr->next;
    }

    return nullptr;
}

unsigned FifoQueue::count() const {
    return len;
}
