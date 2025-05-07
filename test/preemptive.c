#include "test.h"

#define DURATION 60000000
#define NPOTATO  128

static int potato[NPOTATO];
static int potato_lock = 0;
static struct timeval pstart;


static int
try_move_potato(int num, int pass)
{
	int ret = 0;
	int err;
	struct timeval pend, pdiff;

	if (!interrupt_enabled()) {
		unintr_printf("try_move_potato: error, interrupts disabled\n");
	}

	err = __sync_bool_compare_and_swap(&potato_lock, 0, 1);
	if (!err) {	/* couldn't acquire lock */
		return ret;
	}

	if (potato[num]) {
		potato[num] = 0;
		potato[(num + 1) % NPOTATO] = 1;
		gettimeofday(&pend, NULL);
		timersub(&pend, &pstart, &pdiff);
		unintr_printf("%d: thread %3d passes potato "
			    	  "at time = %9.6f\n", pass, num,
			    	  (float)pdiff.tv_sec +
			    	  (float)pdiff.tv_usec / 1000000);
		if ((potato[(num + 1) % NPOTATO] != 1)
		    || (potato[(num) % NPOTATO] != 0)) {
			unintr_printf("try_move_potato: unexpected potato move\n");
		}
		ret = 1;
	}

	err = __sync_bool_compare_and_swap(&potato_lock, 1, 0);
	assert(err);
	return ret;
}


static void
do_potato(int num)
{
	int ret;
	int pass = 1;

	unintr_printf("0: thread %3d made it to %s\n", num, __FUNCTION__);
	while (1) {
		ret = try_move_potato(num, pass);
		if (ret) {
			pass++;
		}
		spin(1);
		/* Add some yields by some threads to scramble the list */
		if (num > 4) {
			int ii;
			for (ii = 0; ii < num - 4; ii++) {
				if (!interrupt_enabled()) {
					unintr_printf("do_potato: error, "
						          "interrupts disabled\n");
				}
				ret = thread_yield(THREAD_ANY);				
				if (!thread_ret_ok(ret)) {
					unintr_printf("do_potato: "
						      	  "bad thread_yield in %d\n", num);
				}			       
			}
		}
	}
}


int
main()
{
	int ret;
	long ii;
	Tid potato_tids[NPOTATO];

	/* print messages before turning on interrupt */
	printf("starting preemptive test\n");
	printf("this test will take %d seconds\n", DURATION / 1000000);
	gettimeofday(&pstart, NULL);
	
	/* show interrupt handler output, we will turn it off later */
	const bool verbose = true;
	struct config config = { 
        .sched_name = "rand", .preemptive = true, .verbose = verbose
    };
	ut369_start(&config);
	
	/* spin for some time, so you see the interrupt handler output */
	spin(SIG_INTERVAL * 5);
	interrupt_quiet();

	potato[0] = 1;
	for (ii = 1; ii < NPOTATO; ii++) {
		potato[ii] = 0;
	}

	for (ii = 0; ii < NPOTATO; ii++) {
		potato_tids[ii] =
			thread_create((thread_entry_f)do_potato, (void *)ii, 0);
		if (!thread_ret_ok(potato_tids[ii])) {
			unintr_printf("preemptive: bad create "
				      "%ld -> id %d\n", ii, potato_tids[ii]);
		} 
	}

	spin(DURATION);
	unintr_printf("cleaning hot potato\n");

	for (ii = 0; ii < NPOTATO; ii++) {
		if (!interrupt_enabled()) {
			unintr_printf("preemptive: error, "
				      	  "interrupts disabled\n");
		} 

		ret = thread_kill(potato_tids[ii]);
		if (!thread_ret_ok(ret)) {
			unintr_printf("preemptive: bad thread_kill "
				     	  "%ld on id %d\n", ii, potato_tids[ii]);
		}			       
	}

	unintr_printf("preemptive test done\n");
	thread_exit(0);
	assert(false);
	return 0;	
}
