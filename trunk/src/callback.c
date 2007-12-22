/*
 *  Libmonitor default callback functions.
 *
 *  Define the defaults as weak symbols and allow the client to
 *  override a subset of them.
 *
 *  This file is in the public domain.
 *
 *  $Id$
 */

#include "config.h"
#include "common.h"
#include "monitor.h"

void __attribute__ ((weak))
monitor_init_library(void)
{
    MONITOR_DEBUG1("(default callback)\n");
}

void __attribute__ ((weak))
monitor_fini_library(void)
{
    MONITOR_DEBUG1("(default callback)\n");
}

void __attribute__ ((weak))
monitor_init_process(char *process, int *argc, char **argv, unsigned pid)
{
    int i;

    MONITOR_DEBUG("(default callback) process = %s, argc = %d, pid = %u\n",
		  process, *argc, pid);
    for (i = 0; i < *argc; i++) {
	MONITOR_DEBUG("argv[%d] = %s\n", i, argv[i]);
    }
}

void __attribute__ ((weak))
monitor_fini_process(void)
{
    MONITOR_DEBUG1("(default callback)\n");
}

void __attribute__ ((weak))
monitor_init_thread_support(void)
{
    MONITOR_DEBUG1("(default callback)\n");
}

void * __attribute__ ((weak))
monitor_init_thread(unsigned tid)
{
    MONITOR_DEBUG("(default callback) tid = %u\n", tid);
    return (NULL);
}

void __attribute__ ((weak))
monitor_fini_thread(void *user_data)
{
    MONITOR_DEBUG("(default callback) data = %p\n", user_data);
}

void __attribute__ ((weak))
monitor_dlopen(const char *library)
{
    MONITOR_DEBUG("(default callback) library = %s\n", library);
}

void __attribute__ ((weak))
monitor_init_mpi(int *argc, char ***argv)
{
    int i;

    MONITOR_DEBUG("(default callback) size = %d, rank = %d, argc = %d\n",
		  monitor_mpi_comm_size(), monitor_mpi_comm_rank(), *argc);
    for (i = 0; i < *argc; i++) {
	MONITOR_DEBUG("argv[%d] = %s\n", i, (*argv)[i]);
    }
}

void __attribute__ ((weak))
monitor_fini_mpi(void)
{
    MONITOR_DEBUG1("(default callback)\n");
}
