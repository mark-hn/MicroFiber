#include "src/microfiber.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>

#define NTHREADS 32
#define PRIO_MAX 40

int display_prio(void *arg) {
    int prio = reinterpret_cast<long>(arg);
    printf("%d: my priority is %d\n", MicroFiber::get_thread_id(), prio);
    return 0;
}

int create_and_preempt(void *arg) {
    int prio = reinterpret_cast<long>(arg);

    // We should be preempted by the newly created thread
    ThreadID ret = MicroFiber::thread_create(create_and_preempt,
                                             reinterpret_cast<void *>(static_cast<long>(prio)),
                                             prio);

    if (ret == static_cast<ThreadID>(static_cast<int>(MicroFiber::ThreadCodes::MAX_THREADS))) {
        return MicroFiber::get_thread_id();
    } else {
        printf("%d: created child %d\n", MicroFiber::get_thread_id(), ret);
    }

    // Grab the exit code from the zombie thread
    int exitcode;
    int ret2 = MicroFiber::thread_wait(ret, &exitcode);
    assert(ret2 == 0);
    assert(exitcode == ret);

    return MicroFiber::get_thread_id();
}

static Lock *lock;

int lock_and_print(void *arg) {
    int prio = reinterpret_cast<long>(arg);
    lock->acquire();
    printf("%d: holding lock with priority %d\n", MicroFiber::get_thread_id(), prio);
    lock->release();
    return 0;
}

static int next_prio = 1;

int wait_then_killed(void *arg) {
    int ptid = reinterpret_cast<long>(arg);
    int ret;

    if (ptid >= 0) {
        printf("%d: killing %d\n", MicroFiber::get_thread_id(), ptid);
        ret = MicroFiber::thread_kill(ptid);
        assert(ret == ptid);
    }

    // Decrease priority for the next thread
    int my_prio = __sync_fetch_and_add(&next_prio, 1);
    ret = MicroFiber::thread_create(wait_then_killed,
                                    reinterpret_cast<void *>(static_cast<long>(MicroFiber::get_thread_id())),
                                    my_prio);

    if (ret >= 0) {
        MicroFiber::thread_wait(ret, nullptr);
        assert(false); // Should not reach here
    } else {
        assert(ret == static_cast<int>(MicroFiber::ThreadCodes::MAX_THREADS));
        printf("prio test done.\n");
    }

    MicroFiber::thread_exit(0);
}

int main() {
    int ret;
    ThreadID child[NTHREADS];
    Config config = {
            .scheduler_name = Config::SchedulerType::Prio,
            .is_preemptive = false
    };

    MicroFiber::microfiber_start(&config);

    for (int &i: child) {
        // Generate a random priority between 1 and PRIO_MAX
        int prio = rand() % (PRIO_MAX - 1) + 1;
        ret = MicroFiber::thread_create(display_prio, reinterpret_cast<void *>(static_cast<long>(prio)), prio);
        assert(ret >= 0);
        i = ret;
    }

    MicroFiber::set_thread_priority(PRIO_MAX);
    display_prio(reinterpret_cast<void *>(static_cast<long>(PRIO_MAX)));

    for (int i: child) {
        ret = MicroFiber::thread_wait(i, nullptr);
        assert(ret == 0);
    }

    // Run the function in the initial thread
    create_and_preempt(reinterpret_cast<void *>(20));

    lock = new Lock();
    MicroFiber::set_thread_priority(0);

    for (int &i: child) {
        // Generate a random priority between 1 and 3
        int prio = (rand() % 3) + 1;
        ret = MicroFiber::thread_create(lock_and_print, reinterpret_cast<void *>(static_cast<long>(prio)), prio);
        assert(ret >= 0);
        i = ret;
    }

    // Let all child threads block on acquiring the lock
    lock->acquire();
    MicroFiber::set_thread_priority(PRIO_MAX);

    // Initial thread can only run after all child threads block (lowest priority)
    lock->release();

    for (int i: child) {
        ret = MicroFiber::thread_wait(i, nullptr);
        assert(ret == 0);
    }

    delete lock;

    MicroFiber::set_thread_priority(0);
    wait_then_killed(reinterpret_cast<void *>(-1));

    assert(false); // Should not reach here
}
