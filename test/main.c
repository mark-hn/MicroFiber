#include "test.h"

#define DURATION  60000000
#define NTHREADS       128
#define LOOPS	        10

static void grand_finale();
static int hello(char *msg);
static int fact(int n);
static void suicide();
static int finale();
static int set_flag(int val);

long *stack_array[THREAD_MAX_THREADS];

int
main(void)
{
	Tid ret;
	Tid ret2;
	size_t allocated_space;

	struct config conf = {
		.preemptive = false,
		.sched_name = "rand",
	};

	/* start the threading system */
	ut369_start(&conf);
	
	/* See ut369.h -- initial thread must be Tid 0 */
	assert(thread_id() == 0);
	ret = thread_yield(0);
	assert(thread_ret_ok(ret));
	printf("initial thread returns from yield(0)\n");
	ret = thread_yield(THREAD_ANY);
	assert(ret == THREAD_NONE);
	printf("initial thread returns from yield(ANY)\n");
	ret = thread_yield(0xDEADBEEF);
	assert(ret == THREAD_INVALID);
	printf("initial thread returns from yield(INVALID)\n");
	ret = thread_yield(16);
	assert(ret == THREAD_INVALID);
	printf("initial thread returns from yield(INVALID2)\n");

	struct mallinfo minfo;
	minfo = mallinfo();
	allocated_space = minfo.uordblks;
	/* create a thread */
	ret = thread_create((thread_entry_f)hello, "hello from first thread", 0);
	minfo = mallinfo();
	if ((size_t)minfo.uordblks <= allocated_space) {
		printf("it appears that the thread stack is not being"
		       "allocated dynamically\n");
		assert(0);
	}
	printf("my id is %d\n", thread_id());
	assert(thread_ret_ok(ret));
	ret2 = thread_yield(ret);
	assert(ret2 == ret);

	/* store address of some variable on stack */
	stack_array[thread_id()] = (long *)&ret;

	int ii, jj;
	/* we will be using THREAD_MAX_THREADS threads later */
	Tid child[THREAD_MAX_THREADS];
	char msg[NTHREADS][THREAD_MAX_THREADS];
	/* create NTHREADS threads */
	for (ii = 0; ii < NTHREADS; ii++) {
		ret = snprintf(msg[ii], THREAD_MAX_THREADS - 1, "hello from thread %3d", ii);
		assert(ret > 0);
		child[ii] = thread_create((thread_entry_f)hello, msg[ii], 0);
		assert(thread_ret_ok(child[ii]));
	}
	printf("my id is %d\n", thread_id());
	for (ii = 0; ii < NTHREADS; ii++) {
		ret = thread_yield(child[ii]);
		assert(ret == child[ii]);
	}

	/* reap then destroy NTHREADS + 1 threads we just created */
	printf("destroying all threads\n");
	ret = thread_kill(ret2);
	assert(ret == ret2);
	ret = thread_wait(ret2, NULL);
	assert(thread_ret_ok(ret));
	for (ii = 0; ii < NTHREADS; ii++) {
		ret = thread_kill(child[ii]);
		assert(ret == child[ii]);
		ret = thread_wait(child[ii], NULL);
		assert(thread_ret_ok(ret));
	}

	/*
	 * create maxthreads-1 threads
	 */
	printf("creating  %d threads\n", THREAD_MAX_THREADS - 1);
	for (ii = 0; ii < THREAD_MAX_THREADS - 1; ii++) {
		child[ii] = thread_create((thread_entry_f)fact, (void *)10, 0);
		assert(thread_ret_ok(child[ii]));
	}
	/*
	 * Now we're out of threads. Next create should fail.
	 */
	ret = thread_create((thread_entry_f)fact, (void *)10, 0);
	assert(ret == THREAD_NOMORE);
	/*
	 * Now let them all run.
	 */
	printf("running   %d threads\n", THREAD_MAX_THREADS - 1);
	for (ii = 0; ii < THREAD_MAX_THREADS; ii++) {
		ret = thread_yield(ii);
		if (ii == 0) {
			/* 
			 * Guaranteed that first yield will find someone. 
			 * Later ones may or may not depending on who
			 * stub schedules  on exit.
			 */
			assert(thread_ret_ok(ret));
		}
	}

	/* check that the thread stacks are sufficiently far apart */
	for (ii = 0; ii < THREAD_MAX_THREADS; ii++) {
		for (jj = 0; jj < THREAD_MAX_THREADS; jj++) {
			if (ii == jj)
				continue;
			long stack_sep = (long)(stack_array[ii]) -
				(long)(stack_array[jj]);
			if ((labs(stack_sep) < THREAD_MIN_STACK)) {
				printf("stacks of threads %d and %d "
				       "are too close\n", ii, jj);
				assert(0);
			}
		}
	}

	/*
	 * Reap zombies
	 */
	printf("reaping  %d threads\n", THREAD_MAX_THREADS - 1);
	for (ii = 0; ii < THREAD_MAX_THREADS - 1; ii++) {
		ret = thread_wait(child[ii], NULL);
		assert(thread_ret_ok(ret));
	}

	/*
	 * Create another maxthreads-1 threads.
	 */
	printf("creating  %d threads\n", THREAD_MAX_THREADS - 1);
	for (ii = 0; ii < THREAD_MAX_THREADS - 1; ii++) {
		child[ii] = thread_create((thread_entry_f)fact, (void *)10, 0);
		assert(thread_ret_ok(child[ii]));
	}

	/*
	 * Now destroy some explicitly and let the others run
	 */
	printf("destroying %d threads\n", THREAD_MAX_THREADS / 2);
	for (ii = 0; ii < THREAD_MAX_THREADS; ii += 2) {
		ret = thread_kill(child[ii]);
		assert(thread_ret_ok(ret));
		ret = thread_wait(child[ii], NULL);
		assert(thread_ret_ok(ret));
	}

	for (ii = 0; ii < THREAD_MAX_THREADS - 1; ii++) {
		ret = thread_wait(child[ii], NULL);
	}

	ret = thread_kill(thread_id());
	assert(ret == THREAD_INVALID);
	printf("testing some destroys even though I'm the only thread\n");

	ret = thread_kill(42);
	assert(ret == THREAD_INVALID);
	ret = thread_kill(-42);
	assert(ret == THREAD_INVALID);
	ret = thread_kill(THREAD_MAX_THREADS + 1000);
	assert(ret == THREAD_INVALID);

	/*
	 * Create a thread that destroys itself. Control should come back here
	 * after that thread runs.
	 */
	printf("testing destroy self\n");
	int flag = set_flag(0);
	ret = thread_create((thread_entry_f)suicide, NULL, 0);
	assert(thread_ret_ok(ret));
	ret = thread_yield(ret);
	assert(thread_ret_ok(ret));
	flag = set_flag(0);
	assert(flag == 1);	/* Other thread ran */
	/* That thread is gone now */
	ret = thread_yield(ret);
	assert(ret == THREAD_INVALID);

	grand_finale();
	printf("\n\nBUG: test should not get here\n\n");
	assert(0);
	return EXIT_FAILURE;
}

static void
grand_finale()
{
	int ret;

	printf("for my grand finale, I will destroy myself\n");
	printf("while my talented assistant prints \"a2 test done\"\n");
	ret = thread_create((thread_entry_f)finale, NULL, 0);
	assert(thread_ret_ok(ret));
	thread_exit(0);
}

static int
hello(char *msg)
{
	Tid ret = thread_id();
	char str[20];

	printf("message: %s\n", msg);

	/* we cast ret to a float because that helps to check
	 * whether the stack alignment of the frame pointer is correct */
	sprintf(str, "%3.0f\n", (float)ret);

	while (1) {
		thread_yield(THREAD_ANY);
	}

	return ret;
}

static int
fact(int n)
{
	/* store address of some variable on stack */
	stack_array[thread_id()] = (long *)&n;
	if (n == 1) {
		return 1;
	}
	return n * fact(n - 1);
}

static void
suicide()
{
	int ret = set_flag(1);
	assert(ret == 0);
	thread_exit(0);
	assert(0);
}

static int flag_value;

/* sets flag_value to val, returns old value of flag_value */
static int
set_flag(int val)
{
	return __sync_lock_test_and_set(&flag_value, val);
}

static int
finale()
{
	int ret;
	printf("finale running\n");
	ret = thread_yield(THREAD_ANY);
	assert(ret == THREAD_NONE);
	ret = thread_yield(THREAD_ANY);
	assert(ret == THREAD_NONE);
	printf("a2 test done\n");
	return 42;
}
