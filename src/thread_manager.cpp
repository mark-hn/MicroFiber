#include "thread_manager.hpp"
#include "microfiber.hpp"
#include "interrupt_manager.hpp"

std::vector<Thread> thread_array(MAX_THREAD_COUNT);
Thread *current_thread = nullptr;
unsigned thread_count = 0;

////////////////////////
/* THREAD OPERATIONS */
////////////////////////

void thread_init() {
}

ThreadID MicroFiber::get_thread_id() {
}

static Thread *thread_get(ThreadID tid) {
}

static bool thread_runnable(ThreadID tid) {
}

static void thread_stub(int (*thread_main)(void *), void *arg) {
}

ThreadID MicroFiber::thread_create(const ThreadFunction &fn, void *arg, int priority) {
}

static void thread_destroy(Thread *dead) {
}

static void thread_switch(Thread *next) {
}

ThreadID MicroFiber::thread_yield(ThreadID tid) {
}

void MicroFiber::thread_exit(int exit_code) {
}

ThreadID MicroFiber::thread_kill(ThreadID tid) {
}

void thread_end() {
}

void MicroFiber::set_thread_priority(int priority) {
}

ThreadID MicroFiber::thread_wait(ThreadID tid, int *exit_code) {
}

ThreadID MicroFiber::thread_sleep(FifoQueuePtr queue) {
}

ThreadID MicroFiber::thread_wakeup(FifoQueuePtr queue, bool wake_all) {
}


//////////////////////
/* LOCK OPERATIONS */
//////////////////////

struct lock {
    int flag;
    struct thread *owner;
    MicroFiber::FifoQueuePtr wait_queue;
};

std::unique_ptr<MicroFiber::Lock> MicroFiber::create_lock() {
}

void MicroFiber::destroy_lock(std::unique_ptr<Lock> lock) {
}

void MicroFiber::acquire_lock(Lock &lock) {
}

void MicroFiber::release_lock(Lock &lock) {
}
