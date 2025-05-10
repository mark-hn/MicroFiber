#ifndef MICROFIBER_SCHEDULER_H
#define MICROFIBER_SCHEDULER_H


#include "src/microfiber.hpp"

class Thread;

class Scheduler {
public:
    enum class Type {
        Random, FCFS, Prio
    };

    virtual ~Scheduler() = default;

    virtual int init() = 0;

    virtual int enqueue(Thread *thread) = 0;

    virtual Thread *dequeue() = 0;

    virtual Thread *remove(int tid) = 0;

    virtual void destroy() = 0;

    [[nodiscard]] bool is_realtime() const {
        return realtime;
    }

    [[nodiscard]] const std::string &get_name() const {
        return name;
    }

protected:
    Scheduler(std::string name, bool realtime)
            : name(std::move(name)), realtime(realtime) {}

private:
    std::string name;
    bool realtime;
};


class RandScheduler : public Scheduler {
public:
    RandScheduler() : Scheduler("rand", false) {}

    int init() override;

    int enqueue(Thread *thread) override;

    Thread *dequeue() override;

    Thread *remove(int tid) override;

    void destroy() override;
};

class FCFScheduler : public Scheduler {
public:
    FCFScheduler() : Scheduler("fcfs", false) {}

    int init() override;

    int enqueue(Thread *thread) override;

    Thread *dequeue() override;

    Thread *remove(int tid) override;

    void destroy() override;
};

class PrioScheduler : public Scheduler {
public:
    PrioScheduler() : Scheduler("prio", true) {}

    int init() override;

    int enqueue(Thread *thread) override;

    Thread *dequeue() override;

    Thread *remove(int tid) override;

    void destroy() override;
};


extern Scheduler *scheduler;

bool scheduler_init(std::basic_string<char> type);

void scheduler_end();


#endif //MICROFIBER_SCHEDULER_H
