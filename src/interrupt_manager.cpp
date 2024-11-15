#include <bits/types/siginfo_t.h>
#include "interrupt_manager.hpp"
#include "microfiber.hpp"

static void interrupt_handler(int sig, siginfo_t *sip, void *contextVP);

static void set_interrupt(void);

static void set_signal(sigset_t *setp);

void InterruptManager::interrupt_init() {
}

void InterruptManager::interrupt_end() {
}

int InterruptManager::interrupt_on() {
}

int InterruptManager::interrupt_off() {
}

int InterruptManager::interrupt_set(int enabled) {
}

bool MicroFiber::is_interrupt_enabled() {
}

void MicroFiber::spin_wait(int microseconds) {
}

int MicroFiber::safe_printf(const char *format, ...) {
}

static void set_signal(sigset_t *setp) {
}

static void interrupt_handler(int sig, siginfo_t *sip, void *contextVP) {
}

static void set_interrupt(void) {
}
