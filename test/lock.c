#include "test.h"

#define NTHREADS 128
#define LOOPS 8
#define NLOCKLOOPS 100

static struct lock *testlock;
static volatile unsigned long testval1;
static volatile unsigned long testval2;
static volatile unsigned long testval3;

static void
test_lock_thread(unsigned long num)
{
	int i, j;

	for (i = 0; i < LOOPS; i++) {
		for (j = 0; j < NLOCKLOOPS; j++) {
			int ret;

			assert(interrupt_enabled());
			lock_acquire(testlock);
			assert(interrupt_enabled());

			testval1 = num;

			/* let's yield to make sure that even when other threads
			 * run, they cannot access the critical section. */
			assert(interrupt_enabled());
			ret = thread_yield(THREAD_ANY);
			assert(thread_ret_ok(ret) || ret == THREAD_NONE);

			testval2 = num * num;

			/* yield again */
			assert(interrupt_enabled());
			ret = thread_yield(THREAD_ANY);
			assert(thread_ret_ok(ret) || ret == THREAD_NONE);

			testval3 = num % 3;

			assert(testval2 == testval1 * testval1);
			assert(testval2 % 3 == (testval3 * testval3) % 3);
			assert(testval3 == testval1 % 3);
			assert(testval1 == num);
			assert(testval2 == num * num);
			assert(testval3 == num % 3);

			assert(interrupt_enabled());
			lock_release(testlock);
			assert(interrupt_enabled());
		}
		unintr_printf("%d: thread %3d passes\n", i, num);
	}
}

int
main()
{
	long i;
	Tid result[NTHREADS];
    int ret;

	printf("starting lock test\n");

    struct config config = { 
        .sched_name = "rand", .preemptive = true, .verbose = false
    };
	ut369_start(&config);

	assert(interrupt_enabled());
	testlock = lock_create();
	assert(interrupt_enabled());
	for (i = 0; i < NTHREADS; i++) {
		result[i] = thread_create((thread_entry_f)test_lock_thread,
					              (void *)i, 0);
		assert(thread_ret_ok(result[i]));
	}

	for (i = 0; i < NTHREADS; i++) {
		ret = thread_wait(result[i], NULL);
        assert(ret == 0);
	}

	assert(interrupt_enabled());
	lock_destroy(testlock);
	assert(interrupt_enabled());

	unintr_printf("lock test done\n");
	thread_exit(0);
    assert(false);
    return 0;
}