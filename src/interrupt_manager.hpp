#ifndef MICROFIBER_INTERRUPT_MANAGER_H
#define MICROFIBER_INTERRUPT_MANAGER_H

#define SIG_TYPE SIGALRM

class InterruptManager {
public:
    static void interrupt_init();

    static void interrupt_end();

    static int interrupt_on();

    static int interrupt_off();

    static int interrupt_set(int enabled);
};


#endif //MICROFIBER_INTERRUPT_MANAGER_H
