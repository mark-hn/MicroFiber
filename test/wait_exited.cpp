#include "src/microfiber.hpp"
#include "src/interrupt_manager.hpp"
#include <cassert>
#include <cstdio>

#define NTHREADS 8
#define SECRET 101

static int ready = 0;
static int gone = 0;

static int wait_for_exited_parent(void *arg) {
    int ppid = reinterpret_cast<long>(arg);
    int ret;

    MicroFiber::safe_printf("%d: thread started\n", MicroFiber::get_thread_id());
    __sync_fetch_and_add(&ready, 1);

    // Wait for the parent to exit
    while (__sync_fetch_and_add(&gone, 0) == 0) {
        ret = MicroFiber::thread_yield(static_cast<ThreadID>(static_cast<int>(MicroFiber::ThreadCodes::ANY)));

        // Check if return is valid (not an error code)
        assert(ret != static_cast<int>(MicroFiber::ThreadCodes::INVALID) &&
               ret != static_cast<int>(MicroFiber::ThreadCodes::NO_MEMORY) &&
               ret != static_cast<int>(MicroFiber::ThreadCodes::MAX_THREADS));
    }

    // Try to get the exit code of the parent thread
    int exit_code;
    ret = MicroFiber::thread_wait(ppid, &exit_code);
    if (ret == 0) {
        MicroFiber::safe_printf("%d: parent exit %d\n", MicroFiber::get_thread_id(), exit_code);
    } else {
        MicroFiber::safe_printf("%d: parented waited for\n", MicroFiber::get_thread_id());
    }

    if (__sync_fetch_and_add(&ready, -1) <= 1) {
        MicroFiber::safe_printf("wait_exited test done\n");
    }

    MicroFiber::thread_exit(0);
    assert(false); // Should never reach here
}

int main() {
    ThreadID ret;
    printf("starting wait_exited test\n");

    Config config = {
            .scheduler_name = Config::SchedulerType::Random,
            .is_preemptive = true
    };
    MicroFiber::microfiber_start(&config);

    // Create some child threads
    for (int i = 0; i < NTHREADS; i++) {
        ThreadID ret = MicroFiber::thread_create(wait_for_exited_parent,
                                                 reinterpret_cast<void *>(static_cast<long>(MicroFiber::get_thread_id())),
                                                 0);

        // Check if return is valid (not an error code)
        assert(ret != static_cast<int>(MicroFiber::ThreadCodes::INVALID) &&
               ret != static_cast<int>(MicroFiber::ThreadCodes::NO_MEMORY) &&
               ret != static_cast<int>(MicroFiber::ThreadCodes::MAX_THREADS));
    }

    // Wait for all child threads to be ready
    while (__sync_fetch_and_add(&ready, 0) != NTHREADS) {
        ret = MicroFiber::thread_yield(static_cast<ThreadID>(
                                               static_cast<int>(MicroFiber::ThreadCodes::ANY)));

        // Check if return is valid (not an error code)
        assert(ret != static_cast<int>(MicroFiber::ThreadCodes::INVALID) &&
               ret != static_cast<int>(MicroFiber::ThreadCodes::NO_MEMORY) &&
               ret != static_cast<int>(MicroFiber::ThreadCodes::MAX_THREADS));
    }

    // Turn off interrupts to ensure atomicity
    InterruptManager::interrupt_off();
    gone = 1;
    MicroFiber::thread_exit(SECRET);
    assert(false);
}
