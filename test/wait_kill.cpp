#include "src/microfiber.hpp"
#include <cassert>
#include <cstdio>

#define SECRET 42

static int test_wait_kill_thread(void *arg) {
    ThreadID parent = reinterpret_cast<long>(arg);
    ThreadID ret;
    int exitcode;

    // Child yields until there are no other runnable threads, ensures that parent executed thread_wait() on child
    while (MicroFiber::thread_yield(static_cast<ThreadID>(static_cast<int>(MicroFiber::ThreadCodes::ANY))) !=
           static_cast<ThreadID>(static_cast<int>(MicroFiber::ThreadCodes::NONE)));

    // Parent thread is blocked and waiting on this child
    ret = MicroFiber::thread_kill(parent);
    if (ret != parent) {
        MicroFiber::safe_printf("%s: bad thread_kill, expected %d got %d\n",
                                __FUNCTION__, parent, ret);
    }

    // This should be a no-op since the parent is already dead
    ret = MicroFiber::thread_wait(parent, &exitcode);

    // Killed thread should return the exit code KILLED
    assert(exitcode == static_cast<int>(MicroFiber::ThreadCodes::KILLED));
    MicroFiber::safe_printf("its over\n");
    return SECRET;
}

int main() {
    ThreadID child;
    ThreadID ret;

    printf("starting wait_kill test\n");

    Config config = {
            .scheduler_name = Config::SchedulerType::Random,
            .is_preemptive = true
    };
    MicroFiber::microfiber_start(&config);

    child = MicroFiber::thread_create(test_wait_kill_thread,
                                      reinterpret_cast<void *>(static_cast<long>(MicroFiber::get_thread_id())),
                                      0);

    // Check if thread creation was successful
    assert(child != static_cast<ThreadID>(static_cast<int>(MicroFiber::ThreadCodes::INVALID)) &&
           child != static_cast<ThreadID>(static_cast<int>(MicroFiber::ThreadCodes::NO_MEMORY)) &&
           child != static_cast<ThreadID>(static_cast<int>(MicroFiber::ThreadCodes::MAX_THREADS)));

    ret = MicroFiber::thread_wait(child, nullptr);

    // Child kills the parent, so we should not reach this point
    assert(ret == 0);
    MicroFiber::safe_printf("wait_kill test failed\n");
    return 0;
}
