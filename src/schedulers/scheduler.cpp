#include "scheduler.hpp"

// Global pointer to the current scheduler
Scheduler *scheduler;

// Set of available schedulers
std::vector<Scheduler *> schedulers = {
        new RandScheduler(),
        new FCFScheduler(),
        new PrioScheduler()
};

/* Initialize the scheduling subsystem */
bool scheduler_init(Scheduler::Type type) {
    scheduler = nullptr;

    auto it = std::find_if(schedulers.begin(), schedulers.end(),
                           [type](Scheduler *&s) {
                               return s->get_name() == (type == Scheduler::Type::Random ? "rand" :
                                                        type == Scheduler::Type::FCFS ? "fcfs" : "prio");
                           });

    if (it != schedulers.end()) {
        scheduler = *it;
        return scheduler->init() == 0;
    }

    return false;
}

void scheduler_end() {
    if (scheduler != nullptr) {
        scheduler->destroy();
    }
    scheduler = nullptr;
}
