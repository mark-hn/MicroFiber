#include "src/microfiber.hpp"
#include <cassert>
#include <cstdio>
#include <sys/time.h>

#define DURATION 60000000
#define NPOTATO  128

static int potato[NPOTATO];
static int potato_lock = 0;
static struct timeval pstart;

static int
try_move_potato(int num, int pass) {
    int ret = 0;
    bool err;
    struct timeval pend, pdiff;

    if (!MicroFiber::is_interrupt_enabled()) {
        MicroFiber::safe_printf("try_move_potato: error, interrupts disabled\n");
    }

    err = __sync_bool_compare_and_swap(&potato_lock, 0, 1);
    if (!err) {
        return ret;
    }

    if (potato[num]) {
        potato[num] = 0;
        potato[(num + 1) % NPOTATO] = 1;
        gettimeofday(&pend, nullptr);
        timersub(&pend, &pstart, &pdiff);

        MicroFiber::safe_printf("%d: thread %3d passes potato "
                                "at time = %9.6f\n", pass, num,
                                (float) pdiff.tv_sec +
                                (float) pdiff.tv_usec / 1000000);

        if ((potato[(num + 1) % NPOTATO] != 1) || (potato[(num) % NPOTATO] != 0)) {
            MicroFiber::safe_printf("try_move_potato: unexpected potato move\n");
        }

        ret = 1;
    }

    err = __sync_bool_compare_and_swap(&potato_lock, 1, 0);
    assert(err);
    return ret;
}

[[noreturn]] static int do_potato(void *arg) {
    int num = reinterpret_cast<long>(arg);
    int ret;
    int pass = 1;

    MicroFiber::safe_printf("0: thread %3d made it to %s\n", num, __FUNCTION__);
    while (true) {
        ret = try_move_potato(num, pass);
        if (ret) {
            pass++;
        }
        MicroFiber::spin_wait(1);

        // Add some yields by some threads to scramble the list
        if (num > 4) {
            int ii;
            for (ii = 0; ii < num - 4; ii++) {
                if (!MicroFiber::is_interrupt_enabled()) {
                    MicroFiber::safe_printf("do_potato: error, "
                                            "interrupts disabled\n");
                }
                ret = MicroFiber::thread_yield(static_cast<ThreadID>(static_cast<int>(MicroFiber::ThreadCodes::ANY)));

                if (ret == static_cast<int>(MicroFiber::ThreadCodes::INVALID) ||
                    ret == static_cast<int>(MicroFiber::ThreadCodes::NO_MEMORY) ||
                    ret == static_cast<int>(MicroFiber::ThreadCodes::MAX_THREADS)) {
                    MicroFiber::safe_printf("do_potato: bad thread_yield in %d\n", num);
                }
            }
        }
    }
}

int main() {
    int ret;
    long ii;
    ThreadID potato_tids[NPOTATO];

    // Print messages before turning on interrupt
    printf("starting preemptive test\n");
    printf("this test will take %d seconds\n", DURATION / 1000000);
    gettimeofday(&pstart, nullptr);

    // Configure and start MicroFiber
    Config config = {
            .scheduler_name = Config::SchedulerType::Random,
            .is_preemptive = true
    };
    MicroFiber::microfiber_start(&config);

    // Spin for some time
    MicroFiber::spin_wait(INTERRUPT_INTERVAL * 5);

    potato[0] = 1;
    for (ii = 1; ii < NPOTATO; ii++) {
        potato[ii] = 0;
    }

    for (ii = 0; ii < NPOTATO; ii++) {
        potato_tids[ii] = MicroFiber::thread_create(do_potato, reinterpret_cast<void *>(ii), 0);

        if (potato_tids[ii] == static_cast<ThreadID>(
                static_cast<int>(MicroFiber::ThreadCodes::INVALID)) ||
            potato_tids[ii] == static_cast<ThreadID>(
                    static_cast<int>(MicroFiber::ThreadCodes::NO_MEMORY)) ||
            potato_tids[ii] == static_cast<ThreadID>(
                    static_cast<int>(MicroFiber::ThreadCodes::MAX_THREADS))) {
            MicroFiber::safe_printf("preemptive: bad create %ld -> id %d\n", ii, potato_tids[ii]);
        }
    }

    MicroFiber::spin_wait(DURATION);
    MicroFiber::safe_printf("cleaning hot potato\n");

    for (ii = 0; ii < NPOTATO; ii++) {
        if (!MicroFiber::is_interrupt_enabled()) {
            MicroFiber::safe_printf("preemptive: error, "
                                    "interrupts disabled\n");
        }

        ret = MicroFiber::thread_kill(potato_tids[ii]);
        if (ret == static_cast<int>(MicroFiber::ThreadCodes::INVALID) ||
            ret == static_cast<int>(MicroFiber::ThreadCodes::NO_MEMORY) ||
            ret == static_cast<int>(MicroFiber::ThreadCodes::MAX_THREADS)) {
            MicroFiber::safe_printf("preemptive: bad thread_kill %ld on id %d\n", ii, potato_tids[ii]);
        }
    }

    MicroFiber::safe_printf("preemptive test done\n");
    MicroFiber::thread_exit(0);
    assert(false);
}
