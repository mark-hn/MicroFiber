#ifndef MICROFIBER_THREAD_MANAGER_H
#define MICROFIBER_THREAD_MANAGER_H

#include <csignal>
#include "microfiber.hpp"

class Thread {
public:
    enum class State {
        RUNNING, READY, BLOCKED, KILLED, EXITED, DEAD
    };

    // Thread members
    ThreadID id;                // the thread's id
    bool initialized;           // flag to determine if the thread is initialized
    ucontext_t context;         // the thread's context
    int exit_code;              // the thread's exit code
    void *stack;                // the stack pointer
    bool setcontext_called;     // identify if the thread called setcontext
    FifoQueue *wait_queue;      // wait queue associated with the thread (threads that called thread_wait on this thread)
    int num_reapers;            // number of threads that are reaping this thread
    struct Thread *member_of;   // the thread that this thread is a wait queue member of
    int prio;                   // the thread's priority
    State state;                // the thread's state

    // Queue node members
    struct Thread *next;        // pointer to the next node
    bool in_queue;              // indicates whether the item is in a queue
};

// Forward declarations of functions
void thread_init();

void thread_end();

#endif //MICROFIBER_THREAD_MANAGER_H
