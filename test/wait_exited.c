#include "test.h"

#define NTHREADS 8
#define SECRET 101

static int ready = 0;
static int gone = 0;

/* not supposed to be able to access these, but need them for testing */
extern int interrupt_off(void);

static void
wait_for_exited_parent(int ppid)
{        
    int ret;

	unintr_printf("%d: thread started\n", thread_id());
    __sync_fetch_and_add(&ready, 1);

    /* wait for parent to exit */ 
    while(__sync_fetch_and_add(&gone, 0) == 0) {
        ret = thread_yield(THREAD_ANY);
        assert(thread_ret_ok(ret));
    }

    /* now that parent has exited, try to get its exit code */
    int exit_code;
    ret = thread_wait(ppid, &exit_code);
    if (ret == 0) {
        unintr_printf("%d: parent exit %d\n", thread_id(), exit_code);
    }
    else {
        unintr_printf("%d: parented waited for\n", thread_id());
    }

    if (__sync_fetch_and_add(&ready, -1) <= 1) {
        unintr_printf("wait_exited test done\n");
    }

	thread_exit(0);
}

int
main()
{
	Tid ret;
	printf("starting wait_exited test\n");

    struct config config = { 
        .sched_name = "rand", .preemptive = true, .verbose = false
    };
	ut369_start(&config);
	
    /* create some child threads */
    for (int i = 0; i < NTHREADS; i++) {
        Tid ret = thread_create((thread_entry_f)wait_for_exited_parent,
			      (void *)(long)thread_id(), 0);
        assert(thread_ret_ok(ret));
    }

    /* wait for all child threads to start */
    while(__sync_fetch_and_add(&ready, 0) != NTHREADS) {
        ret = thread_yield(THREAD_ANY);
        assert(thread_ret_ok(ret));
    }

    /* turn off interrupt so we can atomically set gone to 1 and 
       call thread_exit. */
    interrupt_off();
    gone = 1;
    thread_exit(SECRET);
    assert(false);
	return EXIT_FAILURE;
}

