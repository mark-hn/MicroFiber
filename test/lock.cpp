#include "src/microfiber.hpp"
#include <cassert>
#include <cstdio>
#include <vector>
#include <memory>

constexpr int NTHREADS = 128;
constexpr int LOOPS = 8;
constexpr int NLOCKLOOPS = 100;

static std::unique_ptr<Lock> testlock;
static volatile unsigned long testval1;
static volatile unsigned long testval2;
static volatile unsigned long testval3;

int test_lock_thread(void *arg) {
    unsigned long num = reinterpret_cast<unsigned long>(arg);

    for (int i = 0; i < LOOPS; i++) {
        for (int j = 0; j < NLOCKLOOPS; j++) {
            assert(MicroFiber::is_interrupt_enabled());

            testlock->acquire();

            assert(MicroFiber::is_interrupt_enabled());

            testval1 = num;

            ThreadID ret = MicroFiber::thread_yield(static_cast<ThreadID>(MicroFiber::ThreadCodes::ANY));
            assert(ret >= 0 || ret == static_cast<int>(MicroFiber::ThreadCodes::NONE));

            testval2 = num * num;

            ret = MicroFiber::thread_yield(static_cast<ThreadID>(MicroFiber::ThreadCodes::ANY));
            assert(ret >= 0 || ret == static_cast<int>(MicroFiber::ThreadCodes::NONE));

            testval3 = num % 3;

            assert(testval2 == testval1 * testval1);
            assert(testval2 % 3 == (testval3 * testval3) % 3);
            assert(testval3 == testval1 % 3);
            assert(testval1 == num);
            assert(testval2 == num * num);
            assert(testval3 == num % 3);

            assert(MicroFiber::is_interrupt_enabled());
            testlock->release();
            assert(MicroFiber::is_interrupt_enabled());
        }

        MicroFiber::safe_printf("%d: thread %3lu passes\n", i, num);
    }

    MicroFiber::thread_exit(0);
}

int main() {
    std::vector<ThreadID> result(NTHREADS);

    std::printf("starting lock test\n");

    Config config = {
            .scheduler_name = Config::SchedulerType::Random,
            .is_preemptive = true
    };
    MicroFiber::microfiber_start(&config);

    assert(MicroFiber::is_interrupt_enabled());

    testlock = std::make_unique<Lock>();

    assert(MicroFiber::is_interrupt_enabled());

    for (int i = 0; i < NTHREADS; i++) {
        result[i] = MicroFiber::thread_create(test_lock_thread, reinterpret_cast<void *>(static_cast<uintptr_t>(i)), 0);
        assert(result[i] >= 0);
    }

    for (int i = 0; i < NTHREADS; i++) {
        int ret;
        ret = MicroFiber::thread_wait(result[i], &ret);
        assert(ret == 0);
    }

    assert(MicroFiber::is_interrupt_enabled());

    testlock.reset();

    assert(MicroFiber::is_interrupt_enabled());

    MicroFiber::safe_printf("lock test done\n");
    MicroFiber::thread_exit(0);
}
