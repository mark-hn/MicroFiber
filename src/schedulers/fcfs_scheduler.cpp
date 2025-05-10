#include "scheduler.hpp"

static std::unique_ptr<FifoQueue> ready_queue;

int FCFScheduler::init() {
    ready_queue = std::make_unique<FifoQueue>(MAX_THREAD_COUNT);
    if (!ready_queue) {
        return static_cast<int>(MicroFiber::ThreadCodes::NO_MEMORY);
    }
    return 0;
}

int FCFScheduler::enqueue(Thread *thread) {
    assert(thread->state == Thread::State::READY);
    if (ready_queue->push(thread) == -1) {
        return static_cast<int>(MicroFiber::ThreadCodes::MAX_THREADS);
    }
    return 0;
}

Thread *FCFScheduler::dequeue() {
    return ready_queue->pop();
}

Thread *FCFScheduler::remove(int tid) {
    return ready_queue->remove(tid);
}

void FCFScheduler::destroy() {
    ready_queue.reset();
}
