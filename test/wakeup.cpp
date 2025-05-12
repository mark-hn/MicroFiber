#include "src/microfiber.hpp"
#include "src/interrupt_manager.hpp"
#include "src/queue.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <sys/time.h>

#define NTHREADS 128
#define LOOPS 10

static FifoQueue *queue;
static std::atomic<int> done(0);
static std::atomic<int> nr_sleeping(0);

// Helper function to spin wait
static void spin(int microseconds) {
    MicroFiber::spin_wait(microseconds);
}

// Thread function that sleeps and wakes up
static int test_wakeup_thread(void *arg) {
    int num = reinterpret_cast<long>(arg);
    int i;
    ThreadID ret;
    struct timeval start, end, diff;

    for (i = 0; i < LOOPS; i++) {
        int enabled;
        gettimeofday(&start, nullptr);

        // Track the number of sleeping threads with interrupts disabled to avoid wakeup races.
        enabled = InterruptManager::interrupt_off();
        assert(enabled);
        nr_sleeping.fetch_add(1);
        ret = MicroFiber::thread_sleep(queue);
        assert(ret >= 0);
        InterruptManager::interrupt_set(enabled);

        gettimeofday(&end, nullptr);
        timersub(&end, &start, &diff);

        // thread_sleep should wait at least 4-5 ms
        if (diff.tv_sec == 0 && diff.tv_usec < 4000) {
            MicroFiber::safe_printf("%d: %s took %ld us. That's too fast."
                                    " You must be busy looping\n",
                                    num, __func__, diff.tv_usec);
            goto out;
        }
    }
    out:
    done.fetch_add(1);
    return 0;
}

int main(int argc, const char *argv[]) {
    ThreadID ret;
    long ii;
    static ThreadID child[NTHREADS];
    int all, enabled;

    if (argc == 2) {
        all = atoi(argv[1]) ? 1 : 0;
    } else {
        fprintf(stderr, "usage: %s 0|1\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("starting wakeup test, all=%d\n", all);

    done = 0;
    nr_sleeping = 0;

    queue = new FifoQueue(MAX_THREAD_COUNT);
    assert(queue);

    // Configure MicroFiber with Random scheduler and preemptive behavior
    Config config = {
            Config::SchedulerType::Random,
            true  // is_preemptive
    };
    MicroFiber::microfiber_start(&config);

    enabled = InterruptManager::interrupt_off();

    // Initial thread sleep and wake up tests
    ret = MicroFiber::thread_sleep(nullptr);
    assert(ret == static_cast<int>(MicroFiber::ThreadCodes::INVALID));
    MicroFiber::safe_printf("initial thread returns from sleep(NULL)\n");

    ret = MicroFiber::thread_sleep(queue);
    assert(ret == static_cast<int>(MicroFiber::ThreadCodes::NONE));
    MicroFiber::safe_printf("initial thread returns from sleep(NONE)\n");

    InterruptManager::interrupt_set(enabled);

    ret = MicroFiber::thread_wakeup(nullptr, false);
    assert(ret == 0);
    ret = MicroFiber::thread_wakeup(queue, true);
    assert(ret == 0);

    // Create all threads
    for (ii = 0; ii < NTHREADS; ii++) {
        child[ii] = MicroFiber::thread_create(test_wakeup_thread, reinterpret_cast<void *>(ii), 0);
        assert(child[ii] >= 0); // Check for valid thread ID
    }

    out:
    while (done.load() < NTHREADS) {
        if (all) {
            // Wait until all threads have slept
            if (nr_sleeping.load() < NTHREADS) {
                goto out;
            }
            // We will wake up all threads in the thread_wakeup call below so set nr_sleeping to 0
            nr_sleeping.store(0);
        } else {
            // Wait until at least one thread has slept
            if (nr_sleeping.load() < 1) {
                goto out;
            }
            // Wake up one thread in the wakeup call below
            nr_sleeping.fetch_add(-1);
        }
        // Spin for 5 ms to allow testing that the sleeping thread is not busy looping
        spin(5000);

        assert(MicroFiber::is_interrupt_enabled());
        ret = MicroFiber::thread_wakeup(queue, all == 1);
        assert(MicroFiber::is_interrupt_enabled());
        assert(ret >= 0);
        assert(all ? ret == NTHREADS : ret == 1);
    }
    // We expect nr_sleeping is 0 at this point
    assert(nr_sleeping.load() == 0);
    assert(MicroFiber::is_interrupt_enabled());

    // No thread should be waiting on queue
    delete queue;

    // Wait for other threads to exit
    while (MicroFiber::thread_yield(static_cast<int>(MicroFiber::ThreadCodes::ANY)) !=
           static_cast<int>(MicroFiber::ThreadCodes::NONE));

    MicroFiber::safe_printf("wakeup test done\n");

    MicroFiber::thread_exit(0);
    assert(false); // Should not reach here
}
