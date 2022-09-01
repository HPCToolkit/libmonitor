/*
 *  Adapt the exit.c test to launch several threads and call exit()
 *  from *one* of them (main or side).  Don't join or exit the other
 *  threads, just let them run.
 *
 *  This tests who gets fini-thread and who gets fini-process.
 *
 *  By contrast, exit.c has multiple calls to exit() and pthread_exit()
 *  as a stress test of concurrency.
 *
 *  Copyright (c) 2007-2022, Rice University.
 *  See the file LICENSE for details.
 *
 *  $Id$
 */

#include <sys/time.h>
#include <sys/types.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PROGRAM_TIME   3
#define NUM_THREADS    4
#define EXIT_THREAD    2

int program_time = PROGRAM_TIME;
int num_threads =  NUM_THREADS;
int exit_thread =  EXIT_THREAD;

struct timeval start;

void
wait_for_time(int secs)
{
    struct timeval now;

    for (;;) {
	gettimeofday(&now, NULL);
	if (now.tv_sec >= start.tv_sec + secs)
	    break;
	usleep(1);
    }
}

/*
 *  Run all threads, including main.  The exit thread calls exit() and
 *  the others just keep running.
 */
void *
my_thread(void *val)
{
    long num = (long) val;
    int is_exit = (num == exit_thread);

    printf("start thread: %ld\n", num);

    wait_for_time(program_time - 1);

    if (num == 0) {
	printf("----------------------------------------\n");
    }

    wait_for_time(program_time);

    if (is_exit) {
	printf("exit thread: %ld\n\n", num);
	exit(0);
    }

    /* other threads keep running */
    wait_for_time(program_time + 10);

    printf("end thread: %ld (%s) -- should not get here\n",
	   num, (is_exit ? "yes" : "no"));
    
    return NULL;
}

/*
 *  Program args: num_threads, exit_thread.
 */
int
main(int argc, char **argv)
{
    pthread_t td;
    long i;

    if (argc < 2 || sscanf(argv[1], "%d", &num_threads) < 1) {
        num_threads = NUM_THREADS;
    }
    if (argc < 3 || sscanf(argv[2], "%d", &exit_thread) < 1) {
	exit_thread = EXIT_THREAD;
    }
    printf("num_threads = %d, exit_thread = %d\n\n",
	   num_threads, exit_thread);

    gettimeofday(&start, NULL);

    for (i = 1; i <= num_threads - 1; i++) {
        if (pthread_create(&td, NULL, my_thread, (void *)i) != 0)
            errx(1, "pthread_create failed");
    }

    my_thread(0);

    printf("main exit -- should not get here\n");

    return 0;
}
