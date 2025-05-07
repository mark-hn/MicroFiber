#include "test.h"

#define NTHREADS 128

static int done;
static Tid wait[NTHREADS];

static void
test_wait_thread(int num)
{
	int exitcode; /* get exit status from thread that we wait for */	
	int rand = ((double)random()) / RAND_MAX * 1000000;
	int ret;

	/* make sure that all threads are created before continuing */
	/* we use atomic operations for synchronization because we assume that
	 * lock/cv have not been implemented yet */
	while (__sync_fetch_and_add(&done, 0) < 1);
	
	/* spin for a random time between 0-1 s */
	spin(rand);
	
	if (num > 0) {
		assert(interrupt_enabled());
		/* wait on previous thread */
		ret = thread_wait(wait[num - 1], &exitcode);
		assert(interrupt_enabled());
		assert(ret == 0);
		assert(exitcode == (num - 1 + THREAD_MAX_THREADS));
		spin(rand / 10);
		/* id should print in ascending order, from 1-127 */
		unintr_printf("id = %d\n", num);
	} else {
		/* wait until everyone is sleeping */
		while(thread_yield(THREAD_ANY) != THREAD_NONE);
	}
	
	thread_exit(num + THREAD_MAX_THREADS); /* exit with unique value */
}

int
main()
{
	Tid ret;
	long i;
	int exitcode;
	
	printf("starting wait test\n");
	srandom(0);

    struct config config = { 
        .sched_name = "rand", .preemptive = true, .verbose = false
    };
	ut369_start(&config);

	/* initial thread wait tests */
	ret = thread_wait(thread_id(), NULL);
	assert(ret == THREAD_INVALID);
	unintr_printf("initial thread returns from wait(0)\n");

	ret = thread_wait(110, NULL);
	assert(ret == THREAD_INVALID);
	unintr_printf("initial thread returns from wait(110)\n");

	done = 0;
	/* create all threads */
	for (i = 0; i < NTHREADS; i++) {
		wait[i] = thread_create((thread_entry_f)test_wait_thread,
					(void *)i, 0);
		assert(thread_ret_ok(wait[i]));
	}

	__sync_fetch_and_add(&done, 1);

	/* Each thread will be waited on by the next thread, except for last,
	 * So main thread only needs to wait for final child thread it created.
	 */
	ret = thread_wait(wait[NTHREADS-1], &exitcode);
	assert(ret == 0);
	assert(exitcode == (NTHREADS - 1 + THREAD_MAX_THREADS));

	unintr_printf("wait test done\n");
    thread_exit(0);
    assert(false);
    return 0;
}