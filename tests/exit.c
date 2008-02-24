/*
 *  Test process exit callbacks by having multiple threads all call
 *  process exit() at the same time.
 *
 *  There should be exactly one call to monitor_fini_process().
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

#define PROGRAM_TIME   6
#define NUM_THREADS    3

int program_time = PROGRAM_TIME;
int num_threads = NUM_THREADS;

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

void *
my_thread(void *v)
{
    long num = (long)v;

    printf("start thread: %ld\n", num);
    wait_for_time(program_time);
    printf("end thread: %ld\n", num);

    exit(0);
    return (NULL);
}

/*
 *  Program args: num_threads.
 */
int
main(int argc, char **argv)
{
    pthread_t td;
    long i;

    if (argc < 2 || sscanf(argv[1], "%d", &num_threads) < 1)
        num_threads = NUM_THREADS;
    printf("num_threads = %d\n\n", num_threads);

    gettimeofday(&start, NULL);

    for (i = 1; i <= num_threads; i++) {
        if (pthread_create(&td, NULL, my_thread, (void *)i) != 0)
            errx(1, "pthread_create failed");
    }

    wait_for_time(program_time/2);
    printf("-------------------------------------------------------\n");
    wait_for_time(program_time);

    printf("main exit\n");
    return (0);
}
