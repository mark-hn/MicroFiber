#include "scheduler.hpp"
#include "src/microfiber.hpp"
#include "src/thread_manager.hpp"

int RandScheduler::init() {
}

int RandScheduler::enqueue(std::shared_ptr<Thread> thread) {
}

std::shared_ptr<Thread> RandScheduler::dequeue() {
}

std::shared_ptr<Thread> RandScheduler::remove(int tid) {
}

void RandScheduler::destroy() {
}
