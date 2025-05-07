#include "test.h"

#define NTHREADS 128
#define LOOPS 10

static fifo_queue_t *queue;
static int done;
static int nr_sleeping;

/* not supposed to be able to access these, but need them for testing */
extern int interrupt_off(void);
extern int interrupt_set(int enabled);

static void
test_wakeup_thread(int num)
{
	int i;
	int ret;
	struct timeval start, end, diff;

	for (i = 0; i < LOOPS; i++) {
		int enabled;
		gettimeofday(&start, NULL);

		/* track the number of sleeping threads with interrupts
		 * disabled to avoid wakeup races. */
		enabled = interrupt_off();
		assert(enabled);
		__sync_fetch_and_add(&nr_sleeping, 1);
		ret = thread_sleep(queue);
		assert(thread_ret_ok(ret));
		interrupt_set(enabled);

		gettimeofday(&end, NULL);
		timersub(&end, &start, &diff);

		/* thread_sleep should wait at least 4-5 ms */
		if (diff.tv_sec == 0 && diff.tv_usec < 4000) {
			unintr_printf("%d: %s took %ld us. That's too fast."
				          " You must be busy looping\n",
				          num, __FUNCTION__, diff.tv_usec);
			goto out;
		}
	}
out:
	__sync_fetch_and_add(&done, 1);
}

int
main(int argc, const char * argv[])
{
	Tid ret;
	long ii;
	static Tid child[NTHREADS];
    int all, enabled;
	
    if (argc == 2) {
        all = atoi(argv[1]) ? 1 : 0;
    }
    else {
        fprintf(stderr, "usage: %s 0|1\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("starting wakeup test, all=%d\n", all);

	done = 0;
	nr_sleeping = 0;

	queue = queue_create(THREAD_MAX_THREADS);
	assert(queue);

    struct config config = { 
        .sched_name = "rand", .preemptive = true, .verbose = false,
    };
	ut369_start(&config);

    enabled = interrupt_off();

	/* initial thread sleep and wake up tests */
	ret = thread_sleep(NULL);
	assert(ret == THREAD_INVALID);
	unintr_printf("initial thread returns from sleep(NULL)\n");

	ret = thread_sleep(queue);
	assert(ret == THREAD_NONE);
	unintr_printf("initial thread returns from sleep(NONE)\n");

    interrupt_set(enabled);

	ret = thread_wakeup(NULL, 0);
	assert(ret == 0);
	ret = thread_wakeup(queue, 1);
	assert(ret == 0);

	/* create all threads */
	for (ii = 0; ii < NTHREADS; ii++) {
		child[ii] = thread_create((thread_entry_f)test_wakeup_thread,
					  (void *)ii, 0);
		assert(thread_ret_ok(child[ii]));
	}
    
out:
	while (__sync_fetch_and_add(&done, 0) < NTHREADS) {
		if (all) {
			/* wait until all threads have slept */
			if (__sync_fetch_and_add(&nr_sleeping, 0) < NTHREADS) {
				goto out;
			}
			/* we will wake up all threads in the thread_wakeup
			 * call below so set nr_sleeping to 0 */
			nr_sleeping = 0;
		} else {
			/* wait until at least one thread has slept */
			if (__sync_fetch_and_add(&nr_sleeping, 0) < 1) {
				goto out;
			}
			/* wake up one thread in the wakeup call below */
			__sync_fetch_and_add(&nr_sleeping, -1);
		}
		/* spin for 5 ms. this allows testing that the sleeping thread
		 * sleeps for at least 5 ms. */
		spin(5000);

		/* tests thread_wakeup */
		assert(interrupt_enabled());
		ret = thread_wakeup(queue, all);
		assert(interrupt_enabled());
		assert(ret >= 0);
		assert(all ? ret == NTHREADS : ret == 1);
	}
	/* we expect nr_sleeping is 0 at this point */
	assert(nr_sleeping == 0);
	assert(interrupt_enabled());

	/* no thread should be waiting on queue */
	queue_destroy(queue);

	/* wait for other threads to exit */
	while (thread_yield(THREAD_ANY) != THREAD_NONE);

	/* we don't check for memory leaks because while threads have exited,
	 * they may not have been destroyed yet. */
	unintr_printf("wakeup test done\n");

    thread_exit(0);
    assert(false);
    return 0;
}