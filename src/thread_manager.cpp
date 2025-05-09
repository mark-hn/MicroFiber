#include <ucontext.h>
#include "thread_manager.hpp"
#include "microfiber.hpp"
#include "interrupt_manager.hpp"
#include "queue.hpp"
#include "schedulers/scheduler.hpp"

std::vector<Thread> thread_array(MAX_THREAD_COUNT);
Thread *current_thread = nullptr;
unsigned thread_count = 0;

////////////////////////
/* THREAD OPERATIONS */
////////////////////////

void thread_init() {
    int enabled = InterruptManager::interrupt_off();

    Thread *t = &thread_array[0];
    t->id = 0;
    t->next = nullptr;
    t->in_queue = false;
    t->initialized = true;
    t->setcontext_called = false;
    t->wait_queue = new FifoQueue(MAX_THREAD_COUNT);
    t->num_reapers = 0;
    t->member_of = nullptr;
    t->state = Thread::State::RUNNING;
    t->prio = 0;

    current_thread = t;
    thread_count = 1;

    InterruptManager::interrupt_set(enabled);
}

ThreadID MicroFiber::get_thread_id() {
    return current_thread->id;
}

// Get the thread structure by ID
static Thread *thread_get(ThreadID tid) {
    if (tid < 0 || tid >= MAX_THREAD_COUNT || !thread_array[tid].initialized) {
        return nullptr;
    }
    return &thread_array[tid];
}

// Check if the thread is runnable
static bool thread_runnable(ThreadID tid) {
    int enabled = InterruptManager::interrupt_off();
    struct Thread *t = thread_get(tid);
    InterruptManager::interrupt_set(enabled);
    return (t != nullptr && (t->state == Thread::State::READY ||
                             t->state == Thread::State::RUNNING ||
                             t->state == Thread::State::KILLED));
}

// New thread starts executing here
static void thread_stub(int (*thread_main)(void *), void *arg) {
    InterruptManager::interrupt_set(1);
    int ret = thread_main(arg);
    MicroFiber::thread_exit(ret);
}

ThreadID MicroFiber::thread_create(const ThreadFunction &fn, void *arg, int priority) {
    int enabled = InterruptManager::interrupt_off();

    if (thread_count >= MAX_THREAD_COUNT) {
        InterruptManager::interrupt_set(enabled);
        return static_cast<ThreadID>(ThreadCodes::MAX_THREADS);
    }

    ThreadID tid = -1;
    for (int i = 0; i < MAX_THREAD_COUNT; i++) {
        if (!thread_array[i].initialized) {
            tid = i;
            break;
        }
    }
    assert(tid != -1);

    Thread *new_thread = &thread_array[tid];
    new_thread->id = tid;
    new_thread->next = nullptr;
    new_thread->in_queue = false;
    new_thread->initialized = true;
    new_thread->setcontext_called = false;
    new_thread->wait_queue = new FifoQueue(MAX_THREAD_COUNT);
    new_thread->num_reapers = 0;
    new_thread->member_of = nullptr;
    new_thread->state = Thread::State::READY;
    new_thread->prio = priority;

    getcontext(&(new_thread->context));

    void *stack = malloc(MIN_STACK_SIZE + 16);
    if (stack == nullptr) {
        new_thread->initialized = false;
        InterruptManager::interrupt_set(enabled);
        return static_cast<ThreadID>(ThreadCodes::NO_MEMORY);
    }
    unsigned long stack_top = (unsigned long) stack + MIN_STACK_SIZE + 8;

    new_thread->context.uc_mcontext.gregs[REG_RSP] = (greg_t) stack_top;
    new_thread->context.uc_mcontext.gregs[REG_RIP] = (greg_t) thread_stub;
    new_thread->context.uc_mcontext.gregs[REG_RDI] = (greg_t) fn;
    new_thread->context.uc_mcontext.gregs[REG_RSI] = (greg_t) arg;
    new_thread->stack = stack;

    thread_count++;

    assert(scheduler->enqueue(new_thread) == 0);
    if (scheduler->is_realtime()) {
        thread_yield(static_cast<ThreadID>(ThreadCodes::ANY));
    }

    InterruptManager::interrupt_set(enabled);
    return new_thread->id;
}

// Clean up a thread structure and make the thread id available for reuse
static void thread_destroy(Thread *dead) {
    int enabled = InterruptManager::interrupt_off();

    assert(dead != nullptr);
    assert(!dead->in_queue);
    assert(dead->state == Thread::State::EXITED || dead->state == Thread::State::KILLED);

    if (dead->stack != nullptr) {
        free(dead->stack);
        dead->stack = nullptr;
    }
    dead->initialized = false;
    dead->state = Thread::State::DEAD;
    dead->member_of = nullptr;
    delete dead->wait_queue;

    thread_count--;
    InterruptManager::interrupt_set(enabled);
}

// Context switch to the next thread
static void thread_switch(Thread *next) {
    assert(next != nullptr);
    assert(next != current_thread);

    getcontext(&current_thread->context);

    if (current_thread->setcontext_called && current_thread->state != Thread::State::KILLED &&
        current_thread->state != Thread::State::EXITED) {
        current_thread->setcontext_called = false;
        return;
    }

    current_thread->setcontext_called = true;
    current_thread = next;

    if (current_thread->state == Thread::State::KILLED) {
        MicroFiber::thread_exit(static_cast<int>(MicroFiber::ThreadCodes::KILLED));
        assert(0);
    } else {
        current_thread->state = Thread::State::RUNNING;
        setcontext(&current_thread->context);
        assert(0);
    }
}

ThreadID MicroFiber::thread_yield(ThreadID want_tid) {
    int enabled = InterruptManager::interrupt_off();

    Thread *next_thread;

    if (want_tid == get_thread_id()) {
        assert(thread_runnable(want_tid));
        InterruptManager::interrupt_set(enabled);
        return want_tid;
    }

    if (want_tid == static_cast<ThreadID>(MicroFiber::ThreadCodes::ANY)) {
        next_thread = scheduler->dequeue();
        if (next_thread == nullptr) {
            InterruptManager::interrupt_set(enabled);
            return static_cast<ThreadID>(MicroFiber::ThreadCodes::NONE);
        }
        if (next_thread->prio > current_thread->prio && thread_runnable(get_thread_id())) {
            assert(scheduler->enqueue(next_thread) == 0);
            InterruptManager::interrupt_set(enabled);
            return current_thread->id;
        }
    } else {
        next_thread = scheduler->remove(want_tid);
        if (next_thread == nullptr) {
            InterruptManager::interrupt_set(enabled);
            return static_cast<ThreadID>(MicroFiber::ThreadCodes::INVALID);
        }
    }

    assert(next_thread != current_thread);
    if (thread_runnable(get_thread_id())) {
        if (current_thread->state == Thread::State::RUNNING) {
            current_thread->state = Thread::State::READY;
        }
        assert(scheduler->enqueue(current_thread) == 0);
    }

    thread_switch(next_thread);
    InterruptManager::interrupt_set(enabled);

    return next_thread->id;
}

void MicroFiber::thread_exit(int exit_code) {
    assert(current_thread->state != Thread::State::EXITED);
    current_thread->state = Thread::State::EXITED;
    current_thread->exit_code = exit_code;

    assert(current_thread->wait_queue != nullptr);
    thread_wakeup(current_thread->wait_queue, 1);

    if (thread_yield(static_cast<ThreadID>(ThreadCodes::ANY)) == static_cast<ThreadID>(ThreadCodes::NONE)) {
        InterruptManager::interrupt_off();
        microfiber_exit(exit_code);
    }
}

ThreadID MicroFiber::thread_kill(ThreadID tid) {
    int enabled = InterruptManager::interrupt_off();

    Thread *victim = thread_get(tid);
    if (victim == nullptr || tid == current_thread->id || !victim->initialized) {
        InterruptManager::interrupt_set(enabled);
        return static_cast<ThreadID>(ThreadCodes::INVALID);
    }

    if (victim->state == Thread::State::EXITED) {
        InterruptManager::interrupt_set(enabled);
        return tid;
    }

    if (victim->state == Thread::State::BLOCKED) {
        assert(victim->member_of != nullptr);
        assert(victim->member_of->wait_queue != nullptr);
        victim->member_of->wait_queue->remove(victim->id);
        victim->state = Thread::State::KILLED;

        assert(scheduler->enqueue(victim) == 0);
        if (scheduler->is_realtime()) {
            thread_yield(static_cast<ThreadID>(ThreadCodes::ANY));
        }
    } else {
        victim->state = Thread::State::KILLED;
    }

    InterruptManager::interrupt_set(enabled);
    return tid;
}

void thread_end() {
    for (int i = 0; i < MAX_THREAD_COUNT; i++) {
        Thread *t = &thread_array[i];
        if (t->initialized) {
            if (t->stack != nullptr) {
                free(t->stack);
                t->stack = nullptr;
            }
            t->initialized = false;
            t->state = Thread::State::DEAD;
            t->member_of = nullptr;
            delete t->wait_queue;
        }
    }

    thread_count = 0;
    current_thread = nullptr;
}

void MicroFiber::set_thread_priority(int priority) {
    int enabled = InterruptManager::interrupt_off();

    current_thread->prio = priority;
    if (scheduler->is_realtime()) {
        thread_yield(static_cast<ThreadID>(ThreadCodes::ANY));
    }

    InterruptManager::interrupt_set(enabled);
}

ThreadID MicroFiber::thread_wait(ThreadID tid, int *exit_code) {
    int enabled = InterruptManager::interrupt_off();

    Thread *target_thread = thread_get(tid);
    if (target_thread == nullptr || tid == current_thread->id) {
        InterruptManager::interrupt_set(enabled);
        return static_cast<ThreadID>(ThreadCodes::INVALID);
    }

    if (target_thread->state == Thread::State::EXITED && target_thread->num_reapers > 0) {
        InterruptManager::interrupt_set(enabled);
        return static_cast<ThreadID>(ThreadCodes::INVALID);
    }

    if (target_thread->state == Thread::State::EXITED) {
        if (exit_code != nullptr) {
            *exit_code = target_thread->exit_code;
        }
        thread_destroy(target_thread);
        InterruptManager::interrupt_set(enabled);
        return 0;
    }

    assert(target_thread->wait_queue != nullptr);
    assert(target_thread->state != Thread::State::EXITED);

    target_thread->num_reapers++;
    current_thread->member_of = target_thread;

    thread_sleep(target_thread->wait_queue);

    assert(target_thread->state == Thread::State::EXITED);

    if (exit_code != nullptr) {
        *exit_code = target_thread->exit_code;
    }

    target_thread->num_reapers--;
    if (target_thread->num_reapers == 0) {
        thread_destroy(target_thread);
    }

    InterruptManager::interrupt_set(enabled);
    return 0;
}

ThreadID MicroFiber::thread_sleep(FifoQueue *queue) {
    int enabled = InterruptManager::interrupt_off();

    if (queue == nullptr) {
        InterruptManager::interrupt_set(enabled);
        return static_cast<ThreadID>(ThreadCodes::INVALID);
    }

    Thread *next = scheduler->dequeue();
    if (next == nullptr) {
        InterruptManager::interrupt_set(enabled);
        return static_cast<ThreadID>(ThreadCodes::NONE);
    }

    Thread *curr = current_thread;
    curr->state = Thread::State::BLOCKED;
    queue->push(curr);

    ThreadID ret = next->id;

    thread_switch(next);

    InterruptManager::interrupt_set(enabled);
    return ret;
}

ThreadID MicroFiber::thread_wakeup(FifoQueue *queue, bool wake_all) {
    int enabled = InterruptManager::interrupt_off();
    int num_woken = 0;

    if (queue == nullptr || queue->count() == 0) {
        InterruptManager::interrupt_set(enabled);
        return 0;
    }

    if (wake_all == 0) {
        Thread *woken = queue->pop();
        assert(woken != nullptr);
        if (woken->state != Thread::State::KILLED) {
            woken->state = Thread::State::READY;
        }
        assert(scheduler->enqueue(woken) == 0);
        if (scheduler->is_realtime()) {
            thread_yield(static_cast<ThreadID>(ThreadCodes::ANY));
        }
        num_woken++;
    } else {
        while (queue->count() > 0) {
            Thread *woken = queue->pop();
            assert(woken != nullptr);
            if (woken->state != Thread::State::KILLED) {
                woken->state = Thread::State::READY;
            }
            assert(scheduler->enqueue(woken) == 0);
            if (scheduler->is_realtime()) {
                thread_yield(static_cast<ThreadID>(ThreadCodes::ANY));
            }
            num_woken++;
        }
    }

    InterruptManager::interrupt_set(enabled);
    return num_woken;
}


//////////////////////
/* LOCK OPERATIONS */
//////////////////////

Lock::Lock() {
    flag = 0;
    owner = nullptr;
    wait_queue = new FifoQueue(MAX_THREAD_COUNT);
}

Lock::~Lock() {
    assert(flag == 0);
    assert(owner == nullptr);
    assert(wait_queue != nullptr);
    delete wait_queue;
}

void Lock::acquire() {
    int enabled = InterruptManager::interrupt_off();

    while (flag == 1) {
        wait_queue->push(current_thread);
        current_thread->state = Thread::State::BLOCKED;
        MicroFiber::thread_yield(static_cast<ThreadID>(MicroFiber::ThreadCodes::ANY));
    }

    flag = 1;
    owner = current_thread;

    InterruptManager::interrupt_set(enabled);
}

void Lock::release() {
    int enabled = InterruptManager::interrupt_off();

    assert(flag == 1);
    assert(owner == current_thread);

    flag = 0;
    owner = nullptr;

    if (wait_queue->count() > 0) {
        Thread *next = wait_queue->pop();
        assert(next != nullptr);
        assert(next->state == Thread::State::BLOCKED);
        next->state = Thread::State::READY;
        assert(scheduler->enqueue(next) == 0);
        if (scheduler->is_realtime()) {
            MicroFiber::thread_yield(static_cast<ThreadID>(MicroFiber::ThreadCodes::ANY));
        }
    }

    InterruptManager::interrupt_set(enabled);
}
