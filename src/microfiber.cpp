#include "microfiber.hpp"
#include "interrupt_manager.hpp"
#include "thread_manager.hpp"
#include "schedulers/scheduler.hpp"

#include <cstdlib>
#include <cassert>
#include <ucontext.h>

/* Exit status of the process */
static int exit_status = 0;

/* Context of the main thread for future clean-up */
static ucontext_t main_context;

/* This flag is used to detect stack mismatch when returning from MicroFiber_start */
static volatile bool called = false;


/* Ends the MicroFiber system by calling finalization routines before exiting */
static void MicroFiber_end() {
    assert(!MicroFiber::is_interrupt_enabled());
    InterruptManager::interrupt_end();
    thread_end();
    scheduler_end();
    exit(exit_status);
}

/* Start the MicroFiber threading system with the provided configuration */
void MicroFiber::microfiber_start(const Config *config) {
    // Initialize random seed
    srand(0);
    scheduler_init(config->scheduler_name);
    thread_init();
    if (config->is_preemptive)
        InterruptManager::interrupt_init();

    // Ensure interrupt is off
    assert(!MicroFiber::is_interrupt_enabled());

    // Save main thread's context to exit
    getcontext(&main_context);
    if (called == 1) {
        MicroFiber_end();
        assert(false);
    }
    called = true;

    // Enable interrupts
    InterruptManager::interrupt_on();
}

/* Cleans up resources and exits the process with the provided exit code */
[[noreturn]] void MicroFiber::microfiber_exit(int code) {
    assert(!MicroFiber::is_interrupt_enabled());
    exit_status = code;

    // Context switch to main thread and run system_end function
    setcontext(&main_context);
    assert(false);
}
