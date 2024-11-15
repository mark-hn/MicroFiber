#include "scheduler.hpp"
#include "src/microfiber.hpp"
#include "src/thread_manager.hpp"

int FCFScheduler::init() {
}

int FCFScheduler::enqueue(std::shared_ptr<Thread> thread) {
}

std::shared_ptr<Thread> FCFScheduler::dequeue() {
}

std::shared_ptr<Thread> FCFScheduler::remove(int tid) {
}

void FCFScheduler::destroy() {
}
