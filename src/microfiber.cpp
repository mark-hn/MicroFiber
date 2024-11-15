#include "microfiber.hpp"
#include "interrupt_manager.hpp"
#include "thread_manager.hpp"
#include "schedulers/scheduler.hpp"

#include <cstdlib>
#include <cassert>
#include <memory>
#include <csignal>

/* Exit status of the process */
static int exit_status = 0;

/* Context of the main thread for future clean-up */
static ucontext_t main_context;

/* This flag is used to detect stack mismatch when returning from MicroFiber_start */
static volatile bool called = false;


/* Ends the MicroFiber system by calling finalization routines before exiting */
static void MicroFiber_end() {
}

/* Start the MicroFiber threading system with the provided configuration */
void MicroFiber::microfiber_start(const Config &config) {
}

/* Cleans up resources and exits the process with the provided exit code */
[[noreturn]] void MicroFiber::microfiber_exit(int code) {
}
