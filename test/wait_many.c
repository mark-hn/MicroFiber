#include "test.h"

#define NTHREADS 64
#define SECRET 42

static int ready;
static int done;

/* not supposed to be able to access these, but need them for testing */
extern int interrupt_off(void);
extern int interrupt_set(int enabled);

static void
test_wait_parent_thread(Tid parent)
{
	int parentcode;
    Tid ret;

    /* make sure we can increment ready and then call wait atomically */
    int enabled = interrupt_off();
	ready++;
    ret = thread_wait(parent, &parentcode);
    interrupt_set(enabled);

	if (ret == 0)
		unintr_printf("%d: thread woken, parent exit %d\n",
			          thread_id(), parentcode);
	else if (ret == THREAD_INVALID)
		unintr_printf("%d: parent gone or waited for\n", thread_id());
	else	/* unexpected return value */
		assert(0);

    /* wait for NTHREADS + 1 to finish */
	if (__sync_fetch_and_add(&done, 1) == NTHREADS)
		unintr_printf("wait_many test done\n");
}

/*
 * Parent thread creates N children threads and yields. When the parent thread
 * runs again, it calls thread_exit(). 
 * Child threads all try to wait on the parent thread. For each child thread, 
 * we should only see "thread woken, parent exit 42" except one (the late waiter).
 */
int
main()
{
	Tid wait[NTHREADS], late_wait;
	Tid ret;
	long i;

	srandom(0);
    ready = 0;
	done = 0;
    printf("starting wait_many test\n");

    struct config config = { 
        .sched_name = "rand", .preemptive = true, .verbose = false
    };
	ut369_start(&config);


	for (i = 0; i < NTHREADS; i++) {
		wait[i] = thread_create((thread_entry_f)test_wait_parent_thread,
				                (void *)(long)thread_id(), 0);
		assert(thread_ret_ok(wait[i]));
	}

    while (ready < NTHREADS) {
        /* make sure some threads start waiting before we exit */
        ret = thread_yield(THREAD_ANY);
        /* With preemption, thread_yield() may be invoked after all child
        * threads have run and either waited or exited. So thread_yield may 
        * return THREAD_NONE. */
        assert(thread_ret_ok(ret) || ret == THREAD_NONE);
    }

    /* make sure the late waiter does not catch the train */
    interrupt_off();
    late_wait = thread_create((thread_entry_f)test_wait_parent_thread,
				              (void *)(long)thread_id(), 0);
    assert(thread_ret_ok(late_wait));
	thread_exit(SECRET);

	/* should never get here */
	unintr_printf("wait_many test failed\n");
	assert(0);
}