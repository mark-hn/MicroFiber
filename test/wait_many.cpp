#include "src/microfiber.hpp"
#include "src/interrupt_manager.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <atomic>

#define NTHREADS 64
#define SECRET 42

static std::atomic<int> ready(0);
static std::atomic<int> done(0);

// Thread function for child threads waiting on parent
static int test_wait_parent_thread(void *arg) {
    ThreadID parent = reinterpret_cast<long>(arg);
    int parentcode;
    ThreadID ret;

    // Make sure we can increment ready and then call wait atomically
    int enabled = InterruptManager::interrupt_off();
    ready++;
    ret = MicroFiber::thread_wait(parent, &parentcode);
    InterruptManager::interrupt_set(enabled);

    if (ret == 0)
        MicroFiber::safe_printf("%d: thread woken, parent exit %d\n", MicroFiber::get_thread_id(), parentcode);
    else if (ret == static_cast<int>(MicroFiber::ThreadCodes::INVALID))
        MicroFiber::safe_printf("%d: parent gone or waited for\n", MicroFiber::get_thread_id());
    else
        assert(0);

    // Wait for NTHREADS + 1 to finish
    if (done.fetch_add(1) == NTHREADS)
        MicroFiber::safe_printf("wait_many test done\n");

    return 0;
}

// Parent thread creates N children threads and yields. When the parent thread runs again, it calls thread_exit().
// Child threads all try to wait on the parent thread.
// ALl children (except one) should print "thread woken, parent exit 42".
int main() {
    ThreadID wait[NTHREADS], late_wait;
    ThreadID ret;
    long i;

    std::srand(0);
    ready = 0;
    done = 0;
    printf("starting wait_many test\n");

    // Configure MicroFiber with Random scheduler and preemptive behavior
    Config config = {
            Config::SchedulerType::Random,
            true  // is_preemptive
    };
    MicroFiber::microfiber_start(&config);

    // Create child threads
    for (i = 0; i < NTHREADS; i++) {
        wait[i] = MicroFiber::thread_create(test_wait_parent_thread,
                                            reinterpret_cast<void *>(static_cast<long>(MicroFiber::get_thread_id())),
                                            0);
        assert(wait[i] >= 0);
    }

    // Wait until all child threads are ready
    while (ready < NTHREADS) {
        // Make sure some threads start waiting before we exit
        ret = MicroFiber::thread_yield(static_cast<int>(MicroFiber::ThreadCodes::ANY));
        assert(ret >= 0 || ret == static_cast<int>(MicroFiber::ThreadCodes::NONE));
    }

    InterruptManager::interrupt_off();
    late_wait = MicroFiber::thread_create(test_wait_parent_thread,
                                          reinterpret_cast<void *>(static_cast<long>(MicroFiber::get_thread_id())),
                                          0);
    assert(late_wait >= 0);
    MicroFiber::thread_exit(SECRET);

    assert(0); // Should never get here
}
