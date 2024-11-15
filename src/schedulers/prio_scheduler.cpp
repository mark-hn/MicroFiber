#include "scheduler.hpp"
#include "src/microfiber.hpp"
#include "src/thread_manager.hpp"

int PrioScheduler::init() {
}

int PrioScheduler::enqueue(std::shared_ptr<Thread> thread) {
}

std::shared_ptr<Thread> PrioScheduler::dequeue() {
}

std::shared_ptr<Thread> PrioScheduler::remove(int tid) {
}

void PrioScheduler::destroy() {
}
