/*
 *  John's program to launch a pthread by dlopen("libpthread.so") and
 *  dlsym("pthread_create").  This method seems to bypass monitor's
 *  normal way of overriding pthread_create().
 *
 *  Copyright (c) 2007-2023, Rice University.
 *  See the file LICENSE for details.
 *
 *  $Id$
 */

#include <sys/time.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define LIB_PTHREAD  "/usr/lib64/libpthread.so.0"

#define NUM_THREADS  1

#define PTHREAD_CREATE_PARAM_LIST	\
    pthread_t *thread,			\
    const pthread_attr_t *attr,		\
    void *start_routine,		\
    void *arg

typedef int pthread_create_fcn_t (PTHREAD_CREATE_PARAM_LIST);

int num_threads = NUM_THREADS;

struct timeval start;

//------------------------------------------------------------

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
my_thread(void *val)
{
    long num = (long) val;

    printf("start thread: %ld\n", num);

    wait_for_time(2);

    printf("end thread: %ld\n", num);

    return NULL;
}

/*
 *  Program args: num_threads
 */
int
main(int argc, char **argv)
{
    void * handle;
    pthread_create_fcn_t * pthr_create_fcn;
    pthread_t td;
    long i;

    if (argc < 2 || sscanf(argv[1], "%d", &num_threads) < 1) {
        num_threads = NUM_THREADS;
    }
    printf("num_threads = %d\n", num_threads);

    gettimeofday(&start, NULL);

    handle = dlopen(LIB_PTHREAD, RTLD_LAZY);
    if (handle == NULL) {
        err(1, "unable to dlopen: %s", LIB_PTHREAD);
    }

    pthr_create_fcn = dlsym(handle, "pthread_create");
    if (pthr_create_fcn == NULL) {
        err(1, "unable to dlsym: %s", "pthread_create");
    }

    for (i = 1; i <= num_threads; i++) {
        if ((*pthr_create_fcn) (&td, NULL, my_thread, (void *)i) != 0) {
	    errx(1, "pthread_create failed");
	}
    }

    wait_for_time(4);

    printf("done\n");

    return 0;
}
