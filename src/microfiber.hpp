#ifndef MICROFIBER_HPP
#define MICROFIBER_HPP

#include <string>
#include <memory>
#include <functional>
#include "queue.hpp"

/* Maximum number of threads */
constexpr int MAX_THREAD_COUNT = 1024;

/* Minimum per-thread stack size */
constexpr int MIN_STACK_SIZE = 32768;

/* Interval for preemptive thread interrupts in microseconds */
constexpr int INTERRUPT_INTERVAL = 200;

/* Configuration for MicroFiber initialization */
struct Config {
    std::string scheduler_name;
    bool is_preemptive;
};

/* Type alias for a thread identifier */
using ThreadID = int;


class MicroFiber {
public:
    /* Thread identifiers and error codes */
    enum class ThreadCodes : int {
        INVALID = -1,
        ANY = -2,
        NONE = -3,
        MAX_THREADS = -4,
        NO_MEMORY = -5,
        KILLED = -6,
    };

    /* Thread entry point type */
    using ThreadFunction = int (*)(void *);

    /* Initialize MicroFiber */
    static void microfiber_start(const Config *config);

    /* Get the identifier of the currently running thread */
    static ThreadID get_thread_id();

    /* Create a thread to run the function fn(arg) with the given priority */
    static ThreadID thread_create(const ThreadFunction &fn, void *arg, int priority);

    /* Exit the current thread, releasing resources and switching to another thread if available */
    [[noreturn]]  static void thread_exit(int exit_code);

    /* Kill a thread specified by tid, stopping its execution */
    static ThreadID thread_kill(ThreadID tid);

    /* Yield execution to a specified thread or the next available thread in the queue */
    static ThreadID thread_yield(ThreadID tid);

    /* Check if interrupts are enabled */
    static bool is_interrupt_enabled();

    /* Busy-wait for a specified number of microseconds */
    static void spin_wait(int microseconds);

    /* Print a formatted message with interrupts temporarily disabled */
    static int safe_printf(const char *format, ...);

    /* Suspend the current thread, adding it to a wait queue, and switch to another thread */
    static ThreadID thread_sleep(FifoQueue *queue);

    /* Wake up threads from the wait queue and add them to the ready queue */
    static int thread_wakeup(FifoQueue *queue, bool wake_all);

    /* Wait for a specified thread to exit, and optionally retrieve its exit code */
    static int thread_wait(ThreadID tid, int *exit_code);

    /* Set the priority of the current thread */
    static void set_thread_priority(int priority);

private:
    /* Exit MicroFiber, cleaning up resources and exiting the process */
    [[noreturn]] static void microfiber_exit(int code);
};


struct Thread;

class Lock {
public:
    Lock();

    ~Lock();

    // Acquire a lock, blocking the calling thread if necessary
    void acquire();

    // Release a lock, allowing waiting threads to acquire it
    void release();

private:
    int flag;
    Thread *owner;
    FifoQueue *wait_queue;
};

#endif // MICROFIBER_HPP
