#include "src/microfiber.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>

#define NTHREADS 128

static int done;
static ThreadID wait[NTHREADS];

static int test_wait_thread(void *arg) {
    int num = reinterpret_cast<long>(arg);
    int exitcode;
    int rand_val = ((double) random()) / RAND_MAX * 1000000;
    int ret;

    // Make sure all threads are created before continuing
    while (__sync_fetch_and_add(&done, 0) < 1);

    // Spin wait for a random time between 0-1 s
    MicroFiber::spin_wait(rand_val);

    if (num > 0) {
        assert(MicroFiber::is_interrupt_enabled());

        // Wait for the previous thread to finish
        ret = MicroFiber::thread_wait(wait[num - 1], &exitcode);
        assert(MicroFiber::is_interrupt_enabled());
        assert(ret == 0);
        assert(exitcode == (num - 1 + static_cast<int>(MicroFiber::ThreadCodes::MAX_THREADS)));
        MicroFiber::spin_wait(rand_val / 10);

        // ID is printed in the order of thread creation
        MicroFiber::safe_printf("id = %d\n", num);
    } else {
        // Wait until every thread is sleeping
        while (MicroFiber::thread_yield(static_cast<ThreadID>(static_cast<int>(MicroFiber::ThreadCodes::ANY))) !=
               static_cast<ThreadID>(static_cast<int>(MicroFiber::ThreadCodes::NONE)));
    }

    MicroFiber::thread_exit(num + static_cast<int>(MicroFiber::ThreadCodes::MAX_THREADS)); // Exit with unique value
}

int main() {
    ThreadID ret;
    long i;
    int exitcode;

    printf("starting wait test\n");
    srandom(0);

    Config config = {
            .scheduler_name = Config::SchedulerType::Random,
            .is_preemptive = true
    };
    MicroFiber::microfiber_start(&config);

    // Initial thread wait tests
    ret = MicroFiber::thread_wait(MicroFiber::get_thread_id(), nullptr);
    assert(ret == static_cast<ThreadID>(static_cast<int>(MicroFiber::ThreadCodes::INVALID)));
    MicroFiber::safe_printf("initial thread returns from wait(0)\n");

    ret = MicroFiber::thread_wait(110, nullptr);
    assert(ret == static_cast<ThreadID>(static_cast<int>(MicroFiber::ThreadCodes::INVALID)));
    MicroFiber::safe_printf("initial thread returns from wait(110)\n");

    done = 0;
    // Create child threads
    for (i = 0; i < NTHREADS; i++) {
        wait[i] = MicroFiber::thread_create(test_wait_thread, reinterpret_cast<void *>(i), 0);
        assert(wait[i] >= 0); // Check if thread creation was successful
    }

    __sync_fetch_and_add(&done, 1);

    // Each thread will be waited on by the next thread, except for the last one
    ret = MicroFiber::thread_wait(wait[NTHREADS - 1], &exitcode);
    assert(ret == 0);
    assert(exitcode == (NTHREADS - 1 + static_cast<int>(MicroFiber::ThreadCodes::MAX_THREADS)));

    MicroFiber::safe_printf("wait test done\n");
    MicroFiber::thread_exit(0);
    assert(false);
}
