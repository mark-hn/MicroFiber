#include "test.h"

#define NTHREADS 32
#define PRIO_MAX 40

int 
display_prio(int prio)
{
    printf("%d: my priority is %d\n", thread_id(), prio);
    return 0;
}

int 
create_and_preempt(int prio)
{
    /* we should be preempted by the newly created thread */
    int ret = thread_create((thread_entry_f)create_and_preempt, 
                            (void *)(long)prio, prio);
    
    if (ret == THREAD_NOMORE) {
        return thread_id();
    }
    else {
        printf("%d: created child %d\n", thread_id(), ret);
    }

    /* grab exit code from zombie */
    int exitcode;
    int ret2 = thread_wait(ret, &exitcode);
    assert(ret2 == 0);
    assert(exitcode == ret);
    
    return thread_id();
}

struct lock * lock;

int
lock_and_print(int prio)
{
    lock_acquire(lock);
    printf("%d: holding lock with priority %d\n", thread_id(), prio);
    lock_release(lock);
    return 0;
}

static int next_prio = 1;

void
wait_then_killed(int ptid)
{
    int ret;

    if (ptid >= 0) {
        printf("%d: killing %d\n", thread_id(), ptid);
        ret = thread_kill(ptid);
        assert(ret == ptid);
    }

    /* decrease priority for the next thread */
    int my_prio = __sync_fetch_and_add(&next_prio, 1);
    ret = thread_create((thread_entry_f)wait_then_killed, 
          (void *)(long)thread_id(), my_prio);
    
    if (ret >= 0) {
        thread_wait(ret, NULL);

        /* should not reach here. we should be killed */
        assert(false);
    }
    else {
        assert(ret == THREAD_NOMORE);
        printf("prio test done.\n");
    }

    thread_exit(0);
}

int
main()
{
    int ret;
    Tid child[NTHREADS];
    struct config config = { 
        .sched_name = "prio", .preemptive = false, .verbose = false
    };
	ut369_start(&config);

    for (int i = 0; i < NTHREADS; i++) {
        /* generate a random priority between 1 and 30 */
        int prio = rand() % (PRIO_MAX - 1) + 1;
        ret = thread_create((thread_entry_f)display_prio, (void *)(long)prio, prio);
        assert(ret >= 0);
        child[i] = ret;
    }

    set_priority(PRIO_MAX);
    display_prio(PRIO_MAX);

    for (int i = 0; i < NTHREADS; i++) {
        ret = thread_wait(child[i], NULL);
        assert(ret == 0);
    }

    /* this runs the function in initial thread */
    create_and_preempt(20);

    lock = lock_create();
    set_priority(0);

    for (int i = 0; i < NTHREADS; i++) {
        /* create 3 priority classes: 1, 2, and 3 */
        int prio = (rand() % 3) + 1;
        ret = thread_create((thread_entry_f)lock_and_print, (void *)(long)prio, prio);
        assert(ret >= 0);
        child[i] = ret;
    }

    /* let all child threads block on acquring lock */
    lock_acquire(lock);
    set_priority(PRIO_MAX);

    /* initial thread can only run after all child threads block (lowest priority) */
    lock_release(lock);

    for (int i = 0; i < NTHREADS; i++) {
        ret = thread_wait(child[i], NULL);
        assert(ret == 0);
    }

    lock_destroy(lock);

    set_priority(0);
    wait_then_killed(-1);

    /* should not reach here */
    assert(false);
    return 0;
}