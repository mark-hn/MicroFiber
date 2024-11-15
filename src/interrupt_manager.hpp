#ifndef MICROFIBER_INTERRUPT_MANAGER_H
#define MICROFIBER_INTERRUPT_MANAGER_H

#define SIG_TYPE SIGALRM

class InterruptManager {
public:
    void interrupt_init();

    void interrupt_end();

    int interrupt_on();

    int interrupt_off();

    int interrupt_set(int enabled);
};


#endif //MICROFIBER_INTERRUPT_MANAGER_H
