/*
 *  Test end-of-process shootdown in libmonitor.
 *
 *  Run several threads with various functions that respond to
 *  shootdown in different ways:
 *    0. loop of pthread_testcancel(),
 *    1. loop of cycles with no cancellation points,
 *    2. sigwait(),
 *    3+. exit() or _exit().
 *
 *  Verify that every thread gets a fini-thread callback and the
 *  process gets one fini-process callback, preferably in the main
 *  thread.
 *
 *  Copyright (c) 2007-2012, Rice University.
 *  See the file LICENSE for details.
 *
 *  $Id$
 */

#include <sys/types.h>
#include <sys/time.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_THREADS  300
#define MIN_THREADS    5
#define PROG_TIME      4

int num_threads;
char *func_type;

struct thread_info {
    pthread_t self;
    int tid;
};
struct thread_info thread[MAX_THREADS];

int sigwait_list[] = {
    SIGUSR1, SIGUSR2, SIGWINCH, SIGPWR, -1
};

struct timeval start;

void
atexit_handler(void)
{
    pthread_t self = pthread_self();
    int k, tid;

    tid = -1;
    for (k = 0; k < num_threads; k++) {
	if (pthread_equal(self, thread[k].self)) {
	    tid = k;
	    break;
	}
    }
    printf("%s: tid: %d, self: %p\n", __func__, tid, (void *)self);
}

void
cleanup_handler(void *p)
{
    struct thread_info *ti = (struct thread_info *)p;
    pthread_t self = pthread_self();
    int tid = ti->tid;

    if (! pthread_equal(self, ti->self)) {
	errx(1, "cleanup handler in wrong thread: expected: %p, actual: %p\n",
	     (void *)(ti->self), (void *)self);
    }
    printf("%s: tid: %d, self: %p\n", __func__, tid, (void *)self);
}

/*
 *  Run a loop of pthread_testcancel().  This is the easiest possible
 *  thread to get into via pthread_cancel().
 */
void
run_test_cancel(struct thread_info *ti)
{
    pthread_t self = ti->self;
    int tid = ti->tid;

    printf("tid: %d, self: %p -- begin %s\n", tid, (void *)self, __func__);

    for (;;) {
	pthread_testcancel();
    }
}

/*
 *  Run a loop of cycles with no cancellation points.  Pthread cancel
 *  will not get into this thread.
 */
void
run_cycles(struct thread_info *ti)
{
    pthread_t self = ti->self;
    int tid = ti->tid;
    double x, sum;

    printf("tid: %d, self: %p -- begin %s\n", tid, (void *)self, __func__);

    for (;;) {
	sum = 0.0;
	for (x = 1.0; x < 5000000; x += 1.0) {
	    sum += x;
	}
	if (sum < 0.0) {
	    printf("sum is negative: %g\n", sum);
	}
    }
}

/*
 *  Run a loop of sigwait() waiting on the signals that monitor uses
 *  for thread shootdown.
 */
void
run_sigwait(struct thread_info *ti)
{
    pthread_t self = ti->self;
    int tid = ti->tid;
    char buf[500];
    sigset_t set;
    int k, ret, sig, pos;
    char delim;

    sigemptyset(&set);
    pos = sprintf(buf, "tid: %d, self: %p -- begin %s", tid, (void *)self, __func__);
    delim = ':';
    for (k = 0; sigwait_list[k] > 0; k++) {
	sigaddset(&set, sigwait_list[k]);
	pos += sprintf(&buf[pos], "%c %d", delim, sigwait_list[k]);
	delim = ',';
    }
    printf("%s\n", buf);

    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
	err(1, "pthread_sigmask failed");
    }
    for (;;) {
	ret = sigwait(&set, &sig);
	printf("%s: tid: %d, signal: %d, ret: %d\n", __func__, tid, sig, ret);
    }
}

/*
 *  Wait for PROG_TIME seconds, then call exit().  Always run at least
 *  one of these threads.
 */
void
run_exit(struct thread_info *ti)
{
    pthread_t self = ti->self;
    int tid = ti->tid;
    struct timeval now, end;
    int do_mesg;

    printf("tid: %d, self: %p -- begin %s\n", tid, (void *)self, __func__);

    end.tv_sec = start.tv_sec + PROG_TIME;
    do_mesg = (tid == num_threads - 1);
    for (;;) {
	gettimeofday(&now, NULL);
	if (do_mesg && now.tv_sec >= end.tv_sec - 2) {
	    printf("------------------------------\n");
	    do_mesg = 0;
	}
	if (now.tv_sec >= end.tv_sec) {
	    break;
	}
    }
    printf("%s: tid: %d -- calling exit(%d)\n", __func__, tid, tid);
    exit(tid);
}

/*
 *  Wait for PROG_TIME seconds, then call _exit().
 */
void
run_u_exit(struct thread_info *ti)
{
    pthread_t self = ti->self;
    int tid = ti->tid;
    struct timeval now, end;

    printf("tid: %d, self: %p -- begin %s\n", tid, (void *)self, __func__);

    end.tv_sec = start.tv_sec + PROG_TIME;
    for (;;) {
	gettimeofday(&now, NULL);
	if (now.tv_sec >= end.tv_sec) {
	    break;
	}
    }
    printf("%s: tid: %d -- calling _exit(%d)\n", __func__, tid, tid);
    _exit(tid);
}

void *
my_thread(void *data)
{
    struct thread_info *ti = data;
    int tid = ti->tid;

    ti->self = pthread_self();

    pthread_cleanup_push(&cleanup_handler, ti);

    if (tid == num_threads - 1) {
	run_exit(ti);
    }
    if (strncmp(func_type, "cancel", 3) == 0
	|| strncmp(func_type, "testcancel", 3) == 0) {
	run_test_cancel(ti);
    }
    else if (strncmp(func_type, "cycles", 3) == 0) {
	run_cycles(ti);
    }
    else if (strncmp(func_type, "sigwait", 3) == 0) {
	run_sigwait(ti);
    }
    else if (strncmp(func_type, "exit", 3) == 0) {
	run_exit(ti);
    }
    else if (strncmp(func_type, "_exit", 3) == 0) {
	run_u_exit(ti);
    }
    else if (tid == 0) {
	run_test_cancel(ti);
    }
    else if (tid == 1) {
	run_cycles(ti);
    }
    else if (tid == 2) {
	run_sigwait(ti);
    }
    else if (tid % 2 == 1) {
	run_u_exit(ti);
    }
    else {
	run_exit(ti);
    }

    printf("==> end of thread %d, should not get here\n", tid);

    pthread_cleanup_pop(0);

    return NULL;
}

/*
 *  Program args: num_threads, function type.
 *
 *  Function type is: cancel, cycles, sigwait, exit or _exit.
 *
 *  Default is to run one of each.
 */
int
main(int argc, char **argv)
{
    struct thread_info *ti;
    pthread_t td;
    int num, min_threads;

    if (argc < 2 || sscanf(argv[1], "%d", &num_threads) < 1) {
	num_threads = MIN_THREADS;
    }
    func_type = "various";
    min_threads = MIN_THREADS;
    if (argc >= 3) {
	func_type = argv[2];
	min_threads = 2;
    }
    if (num_threads < min_threads) {
	num_threads = min_threads;
    }
    printf("num threads: %d, function type: %s\n", num_threads, func_type);

    if (atexit(&atexit_handler) != 0) {
	err(1, "atexit failed");
    }

    gettimeofday(&start, NULL);

    for (num = 1; num < num_threads; num++) {
	ti = &thread[num];
	ti->tid = num;
        if (pthread_create(&td, NULL, &my_thread, ti) != 0) {
            err(1, "pthread_create failed");
	}
    }
    ti = &thread[0];
    ti->tid = 0;
    my_thread(ti);

    printf("==> end of main, should not get here\n");
    return 42;
}
