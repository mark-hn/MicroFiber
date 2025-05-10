#include "scheduler.hpp"

static std::vector<Thread *> ready_queue;

int RandScheduler::init() {
    ready_queue.clear();
    ready_queue.reserve(MAX_THREAD_COUNT);
    return 0;
}

int RandScheduler::enqueue(Thread *thread) {
    if (ready_queue.size() >= MAX_THREAD_COUNT) {
        return static_cast<int>(MicroFiber::ThreadCodes::MAX_THREADS);
    }
    ready_queue.push_back(thread);
    return 0;
}

Thread *RandScheduler::dequeue() {
    if (ready_queue.empty()) {
        return nullptr;
    }

    int i = rand() % ready_queue.size();
    Thread *ret = ready_queue[i];

    // Replace selected with last element
    ready_queue[i] = ready_queue.back();
    ready_queue.pop_back();

    return ret;
}

Thread *RandScheduler::remove(int tid) {
    for (size_t i = 0; i < ready_queue.size(); i++) {
        if (ready_queue[i]->id == tid) {
            Thread *ret = ready_queue[i];
            assert(!ready_queue.empty());
            ready_queue[i] = ready_queue.back();
            ready_queue.pop_back();
            return ret;
        }
    }
    return nullptr;
}

void RandScheduler::destroy() {
    ready_queue.clear();
    ready_queue.shrink_to_fit();
}
