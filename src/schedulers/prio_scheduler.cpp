#include "scheduler.hpp"

static std::unique_ptr<FifoQueue> ready_queue;

int PrioScheduler::init() {
    ready_queue = std::make_unique<FifoQueue>(MAX_THREAD_COUNT);
    if (!ready_queue) {
        return static_cast<int>(MicroFiber::ThreadCodes::NO_MEMORY);
    }
    return 0;
}

int PrioScheduler::enqueue(Thread *thread) {
    assert(thread->state == Thread::State::READY || thread->state == Thread::State::KILLED);
    if (ready_queue->push_sorted(thread) == -1) {
        return static_cast<int>(MicroFiber::ThreadCodes::MAX_THREADS);
    }
    return 0;
}

Thread *PrioScheduler::dequeue() {
    return ready_queue->pop();
}

Thread *PrioScheduler::remove(int tid) {
    return ready_queue->remove(tid);
}

void PrioScheduler::destroy() {
    ready_queue.reset();
}
