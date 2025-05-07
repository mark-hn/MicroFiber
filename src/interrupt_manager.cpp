#include <bits/types/siginfo_t.h>
#include "interrupt_manager.hpp"
#include "microfiber.hpp"
#include <cassert>
#include <bits/sigaction.h>
#include <csignal>
#include <sys/time.h>
#include <cstdarg>

static void interrupt_handler(int sig, siginfo_t *sip, void *contextVP);

static void set_interrupt();

static void set_signal(sigset_t *setp);

static int init = 0;

void InterruptManager::interrupt_init() {
    struct sigaction action;
    int error;

    assert(!init);
    init = 1;
    action.sa_handler = NULL;
    action.sa_sigaction = interrupt_handler;
    error = sigemptyset(&action.sa_mask);
    assert(!error);

    // Use SA_SIGINFO to get the signal number and context in the handler
    action.sa_flags = SA_SIGINFO;
    if (sigaction(SIG_TYPE, &action, nullptr)) {
        perror("Setting up signal handler");
        assert(0);
    }

    // Block the signal while the handler is running to avoid re-entry
    interrupt_off();
    set_interrupt();
}

void InterruptManager::interrupt_end() {
    signal(SIG_TYPE, SIG_IGN);
    init = 0;
}

int InterruptManager::interrupt_on() {
    return interrupt_set(1);
}

int InterruptManager::interrupt_off() {
    return interrupt_set(0);
}

int InterruptManager::interrupt_set(int enabled) {
    int ret;
    sigset_t mask, omask;

    set_signal(&mask);
    if (enabled) {
        ret = sigprocmask(SIG_UNBLOCK, &mask, &omask);
    } else {
        ret = sigprocmask(SIG_BLOCK, &mask, &omask);
    }
    assert(!ret);
    return (sigismember(&omask, SIG_TYPE) ? 0 : 1);
}

bool MicroFiber::is_interrupt_enabled() {
    sigset_t mask;
    int ret;

    if (!init)
        return false;

    ret = sigprocmask(0, nullptr, &mask);
    assert(!ret);
    return (sigismember(&mask, SIG_TYPE) ? 0 : 1);
}

void MicroFiber::spin_wait(int microseconds) {
    struct timeval start{}, end{}, diff{};
    int ret;

    ret = gettimeofday(&start, nullptr);
    assert(!ret);
    while (true) {
        gettimeofday(&end, nullptr);
        timersub(&end, &start, &diff);

        if ((diff.tv_sec * 1000000 + diff.tv_usec) >= microseconds) {
            break;
        }
    }
}

int MicroFiber::safe_printf(const char *format, ...) {
    int ret, enabled;
    va_list args;

    enabled = InterruptManager::interrupt_off();
    va_start(args, format);
    ret = vprintf(format, args);
    va_end(args);
    InterruptManager::interrupt_set(enabled);
    return ret;
}

static void set_signal(sigset_t *setp) {
    int ret;
    ret = sigemptyset(setp);
    assert(!ret);
    ret = sigaddset(setp, SIG_TYPE);
    assert(!ret);
}

// This function is called when the signal is received
static void interrupt_handler(int sig, siginfo_t *sip, void *contextVP) {
    (void) sig;
    (void) sip;
    (void) contextVP;

    assert(!MicroFiber::is_interrupt_enabled());

    set_interrupt();
    MicroFiber::thread_yield(static_cast<ThreadID>(MicroFiber::ThreadCodes::ANY));
}

// Set the interval for the timer to trigger the interrupt
static void set_interrupt() {
    int ret;
    struct itimerval val{};

    val.it_interval.tv_sec = 0;
    val.it_interval.tv_usec = 0;

    val.it_value.tv_sec = 0;
    val.it_value.tv_usec = INTERRUPT_INTERVAL;

    ret = setitimer(ITIMER_REAL, &val, nullptr);
    assert(!ret);
}
