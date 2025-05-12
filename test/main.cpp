#include "src/microfiber.hpp"
#include <cassert>
#include <cstdlib>
#include <array>
#include <string>
#include <vector>
#include <malloc.h>

constexpr int NTHREADS = 128;
constexpr int LOOPS = 10;

std::array<long *, MAX_THREAD_COUNT> stack_array;
int flag_value = 0;

int set_flag(int val) {
    return __sync_lock_test_and_set(&flag_value, val);
}

[[noreturn]] int hello(void *arg) {
    const char *msg = static_cast<const char *>(arg);
    ThreadID tid = MicroFiber::get_thread_id();
    printf("message: %s\n", msg);
    char str[20];
    sprintf(str, "%3.0f\n", static_cast<float>(tid));

    while (true) {
        MicroFiber::thread_yield(static_cast<ThreadID>(MicroFiber::ThreadCodes::ANY));
    }
}

int fact(void *arg) {
    int n = reinterpret_cast<intptr_t>(arg);
    ThreadID tid = MicroFiber::get_thread_id();
    stack_array[tid] = reinterpret_cast<long *>(&n);
    if (n == 1) {
        return 1;
    }
    return n * fact(reinterpret_cast<void *>(n - 1));
}

void suicide() {
    int ret = set_flag(1);
    assert(ret == 0);
    MicroFiber::thread_exit(0);
    assert(false);
}

int finale(void *arg) {
    printf("finale running\n");
    ThreadID ret = MicroFiber::thread_yield(static_cast<ThreadID>(MicroFiber::ThreadCodes::ANY));
    assert(ret == static_cast<ThreadID>(MicroFiber::ThreadCodes::NONE));
    ret = MicroFiber::thread_yield(static_cast<ThreadID>(MicroFiber::ThreadCodes::ANY));
    assert(ret == static_cast<ThreadID>(MicroFiber::ThreadCodes::NONE));
    printf("main test done\n");
    return 42;
}

void grand_finale() {
    printf("for my grand finale, I will destroy myself\n");
    printf("while my talented assistant prints \"main test done\"\n");
    ThreadID ret = MicroFiber::thread_create(finale, nullptr, 0);
    assert(ret >= 0);
    MicroFiber::thread_exit(0);
}

int main() {
    Config config = {
            Config::SchedulerType::Random,
            false
    };

    MicroFiber::microfiber_start(&config);

    assert(MicroFiber::get_thread_id() == 0);
    ThreadID ret = MicroFiber::thread_yield(0);
    assert(ret >= 0);
    printf("initial thread returns from yield(0)\n");

    ret = MicroFiber::thread_yield(static_cast<ThreadID>(MicroFiber::ThreadCodes::ANY));
    assert(ret == static_cast<ThreadID>(MicroFiber::ThreadCodes::NONE));
    printf("initial thread returns from yield(ANY)\n");

    ret = MicroFiber::thread_yield(0xDEADBEEF);
    assert(ret == static_cast<ThreadID>(MicroFiber::ThreadCodes::INVALID));
    printf("initial thread returns from yield(INVALID)\n");

    ret = MicroFiber::thread_yield(16);
    assert(ret == static_cast<ThreadID>(MicroFiber::ThreadCodes::INVALID));
    printf("initial thread returns from yield(INVALID2)\n");

    struct mallinfo2 minfo = mallinfo2();
    size_t allocated_space = minfo.uordblks;

    const char *msg = "hello from first thread";
    ret = MicroFiber::thread_create(hello, const_cast<char *>(msg), 0);

    minfo = mallinfo2();
    if (minfo.uordblks <= allocated_space) {
        printf("it appears that the thread stack is not being allocated dynamically\n");
        assert(false);
    }

    printf("my id is %d\n", MicroFiber::get_thread_id());
    assert(ret >= 0);
    ThreadID ret2 = MicroFiber::thread_yield(ret);
    assert(ret2 == ret);

    stack_array[MicroFiber::get_thread_id()] = reinterpret_cast<long *>(&ret);

    std::vector<ThreadID> child(MAX_THREAD_COUNT);
    std::vector<std::string> messages(NTHREADS);
    for (int ii = 0; ii < NTHREADS; ++ii) {
        messages[ii] = "hello from thread " + std::to_string(ii);
        ret = MicroFiber::thread_create(hello, const_cast<char *>(messages[ii].c_str()), 0);
        assert(ret >= 0);
        child[ii] = ret;
    }

    printf("my id is %d\n", MicroFiber::get_thread_id());
    for (int ii = 0; ii < NTHREADS; ++ii) {
        ret = MicroFiber::thread_yield(child[ii]);
        assert(ret == child[ii]);
    }

    printf("destroying all threads\n");
    ret = MicroFiber::thread_kill(ret2);
    assert(ret == ret2);
    ret = MicroFiber::thread_wait(ret2, nullptr);
    assert(ret >= 0);

    for (int ii = 0; ii < NTHREADS; ++ii) {
        ret = MicroFiber::thread_kill(child[ii]);
        assert(ret == child[ii]);
        ret = MicroFiber::thread_wait(child[ii], nullptr);
        assert(ret >= 0);
    }

    printf("creating  %d threads\n", MAX_THREAD_COUNT - 1);
    for (int ii = 0; ii < MAX_THREAD_COUNT - 1; ++ii) {
        child[ii] = MicroFiber::thread_create(fact, reinterpret_cast<void *>(10), 0);
        assert(child[ii] >= 0);
    }

    ret = MicroFiber::thread_create(fact, reinterpret_cast<void *>(10), 0);
    assert(ret == static_cast<ThreadID>(MicroFiber::ThreadCodes::MAX_THREADS));

    printf("running   %d threads\n", MAX_THREAD_COUNT - 1);
    for (int ii = 0; ii < MAX_THREAD_COUNT; ++ii) {
        ret = MicroFiber::thread_yield(ii);
        if (ii == 0) {
            assert(ret >= 0);
        }
    }

    for (int ii = 0; ii < MAX_THREAD_COUNT; ++ii) {
        for (int jj = 0; jj < MAX_THREAD_COUNT; ++jj) {
            if (ii == jj) continue;
            long stack_sep = reinterpret_cast<long>(stack_array[ii]) - reinterpret_cast<long>(stack_array[jj]);
            if (labs(stack_sep) < MIN_STACK_SIZE) {
                printf("stacks of threads %d and %d are too close\n", ii, jj);
                assert(false);
            }
        }
    }

    printf("reaping  %d threads\n", MAX_THREAD_COUNT - 1);
    for (int ii = 0; ii < MAX_THREAD_COUNT - 1; ++ii) {
        ret = MicroFiber::thread_wait(child[ii], nullptr);
        assert(ret >= 0);
    }

    printf("creating  %d threads\n", MAX_THREAD_COUNT - 1);
    for (int ii = 0; ii < MAX_THREAD_COUNT - 1; ++ii) {
        child[ii] = MicroFiber::thread_create(fact, reinterpret_cast<void *>(10), 0);
        assert(child[ii] >= 0);
    }

    printf("destroying %d threads\n", MAX_THREAD_COUNT / 2);
    for (int ii = 0; ii < MAX_THREAD_COUNT; ii += 2) {
        ret = MicroFiber::thread_kill(child[ii]);
        assert(ret >= 0);
        ret = MicroFiber::thread_wait(child[ii], nullptr);
        assert(ret >= 0);
    }

    for (int ii = 0; ii < MAX_THREAD_COUNT - 1; ++ii) {
        ret = MicroFiber::thread_wait(child[ii], nullptr);
    }

    ret = MicroFiber::thread_kill(MicroFiber::get_thread_id());
    assert(ret == static_cast<ThreadID>(MicroFiber::ThreadCodes::INVALID));
    printf("testing some destroys even though I'm the only thread\n");

    ret = MicroFiber::thread_kill(42);
    assert(ret == static_cast<ThreadID>(MicroFiber::ThreadCodes::INVALID));
    ret = MicroFiber::thread_kill(-42);
    assert(ret == static_cast<ThreadID>(MicroFiber::ThreadCodes::INVALID));
    ret = MicroFiber::thread_kill(MAX_THREAD_COUNT + 1000);
    assert(ret == static_cast<ThreadID>(MicroFiber::ThreadCodes::INVALID));

    printf("testing destroy self\n");
    int flag = set_flag(0);
    ret = MicroFiber::thread_create(reinterpret_cast<MicroFiber::ThreadFunction>(suicide), nullptr, 0);
    assert(ret >= 0);
    ret = MicroFiber::thread_yield(ret);
    assert(ret >= 0);
    flag = set_flag(0);
    assert(flag == 1);
    ret = MicroFiber::thread_yield(ret);
    assert(ret == static_cast<ThreadID>(MicroFiber::ThreadCodes::INVALID));

    grand_finale();
    printf("\n\nBUG: test should not get here\n\n");
    assert(false);
}
