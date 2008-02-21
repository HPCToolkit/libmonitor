/*
 *  Libmonitor core functions: main and exit.
 *
 *  Copyright (c) 2007-2008, Rice University.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *  * Neither the name of Rice University (RICE) nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 *  This software is provided by RICE and contributors "as is" and any
 *  express or implied warranties, including, but not limited to, the
 *  implied warranties of merchantability and fitness for a particular
 *  purpose are disclaimed. In no event shall RICE or contributors be
 *  liable for any direct, indirect, incidental, special, exemplary, or
 *  consequential damages (including, but not limited to, procurement of
 *  substitute goods or services; loss of use, data, or profits; or
 *  business interruption) however caused and on any theory of liability,
 *  whether in contract, strict liability, or tort (including negligence
 *  or otherwise) arising in any way out of the use of this software, even
 *  if advised of the possibility of such damage.
 *
 *  $Id$
 */

#include "config.h"
#include <sys/types.h>
#ifdef MONITOR_DYNAMIC
#include <dlfcn.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "monitor.h"

#ifdef MONITOR_DEBUG_DEFAULT_ON
int monitor_debug = 1;
#else
int monitor_debug = 0;
#endif

/*
 *----------------------------------------------------------------------
 *  MACROS and GLOBAL VARIABLES
 *----------------------------------------------------------------------
 */

#define START_MAIN_PARAM_LIST 			\
    int (*main) (int, char **, char **),	\
    int argc,					\
    char **argv,				\
    void (*init) (void),			\
    void (*fini) (void),			\
    void (*rtld_fini) (void),			\
    void *stack_end

#define START_MAIN_ARG_LIST  \
    main, argc, argv, init, fini, rtld_fini, stack_end

typedef int start_main_fcn_t(START_MAIN_PARAM_LIST);
typedef int main_fcn_t(int, char **, char **);
typedef void exit_fcn_t(int);
typedef int sigprocmask_fcn_t(int, const sigset_t *, sigset_t *);

#ifdef MONITOR_STATIC
extern main_fcn_t __real_main;
extern exit_fcn_t __real__exit;
#ifdef MONITOR_USE_SIGNALS
extern sigprocmask_fcn_t  __real_sigprocmask;
#endif
#endif

static start_main_fcn_t  *real_start_main = NULL;
static main_fcn_t  *real_main = NULL;
static exit_fcn_t  *real_u_exit = NULL;
static sigprocmask_fcn_t *real_sigprocmask = NULL;

static int monitor_argc = 0;
static char **monitor_argv = NULL;
static char **monitor_envp = NULL;

volatile static char monitor_init_library_called = 0;
volatile static char monitor_fini_library_called = 0;
volatile static char monitor_init_process_called = 0;
volatile static char monitor_fini_process_called = 0;

static void *main_stack_bottom = NULL;
extern void monitor_unwind_fence1;
extern void monitor_unwind_fence2;

/*
 *----------------------------------------------------------------------
 *  INIT FUNCTIONS
 *----------------------------------------------------------------------
 */

/*
 *  Normally run as part of monitor_normal_init(), but may be run
 *  earlier if someone calls pthread_create() before our library init
 *  function (dynamic case).
 */
void
monitor_early_init(void)
{
    MONITOR_RUN_ONCE(early_init);

    if (! monitor_debug) {
	if (getenv("MONITOR_DEBUG") != NULL)
	    monitor_debug = 1;
    }

    MONITOR_DEBUG("debug = %d\n", monitor_debug);
}

/*
 *  Run at library init time (dynamic), or in monitor faux main
 *  (static).
 */
static void
monitor_normal_init(void)
{
    MONITOR_RUN_ONCE(normal_init);

    monitor_early_init();
    MONITOR_DEBUG1("\n");

    /*
     * Always get _exit() first so that we have a way to exit if
     * needed.
     */
    MONITOR_GET_REAL_NAME_WRAP(real_u_exit, _exit);
#ifdef MONITOR_STATIC
    real_main = &__real_main;
#else
    MONITOR_REQUIRE_DLSYM(real_start_main, "__libc_start_main");
#endif

#ifdef MONITOR_USE_SIGNALS
    MONITOR_GET_REAL_NAME_WRAP(real_sigprocmask, sigprocmask);
    monitor_signal_init();
#else
    MONITOR_GET_REAL_NAME(real_sigprocmask, sigprocmask);
#endif
}

/*
 *----------------------------------------------------------------------
 *  LIBRARY and PROCESS FUNCTIONS
 *----------------------------------------------------------------------
 */

static void
monitor_begin_library_fcn(void)
{
    MONITOR_RUN_ONCE(begin_library);

    MONITOR_DEBUG1("\n");
    monitor_normal_init();

    MONITOR_DEBUG1("calling monitor_init_library() ...\n");
    monitor_init_library();
    monitor_init_library_called = 1;
}

void
monitor_end_library_fcn(void)
{
    if (monitor_fini_library_called)
	return;

    MONITOR_DEBUG1("calling monitor_fini_library() ...\n");
    monitor_fini_library();
    monitor_fini_library_called = 1;
}

void
monitor_begin_process_fcn(void)
{
    monitor_normal_init();

    MONITOR_DEBUG1("calling monitor_init_process() ...\n");
    monitor_init_process(monitor_argv[0], &monitor_argc,
			 monitor_argv, (unsigned)getpid());
    monitor_init_process_called = 1;
    monitor_fini_process_called = 0;
    monitor_fini_library_called = 0;
    monitor_thread_release();
}

void
monitor_end_process_fcn(void)
{
    if (monitor_fini_process_called)
	return;

    monitor_thread_shootdown();
    MONITOR_DEBUG1("calling monitor_fini_process() ...\n");
    monitor_fini_process();
    monitor_fini_process_called = 1;
}

/*
 *  Let the other modules see the command-line arguments, if needed.
 *  The Fortran mpi_init() uses this.
 */
void
monitor_get_main_args(int *argc_ptr, char ***argv_ptr, char ***env_ptr)
{
    if (argc_ptr != NULL)
	*argc_ptr = monitor_argc;
    if (argv_ptr != NULL)
	*argv_ptr = monitor_argv;
    if (env_ptr != NULL)
	*env_ptr = monitor_envp;
}

/*
 *----------------------------------------------------------------------
 *  SUPPORT FUNCTIONS, internal and external
 *----------------------------------------------------------------------
 */

/*
 *  Returns: 1 if address is within the body of the function at the
 *  bottom of the application's call stack, else 0.
 */
int
monitor_unwind_process_bottom_frame(void *addr)
{
    return (&monitor_unwind_fence1 <= addr && addr <= &monitor_unwind_fence2);
}

/*
 *  For internal monitor use only, clients should use
 *  monitor_stack_bottom().
 */
void *
monitor_get_main_stack_bottom(void)
{
    return (main_stack_bottom);
}

/*
 *  Client access to the real sigprocmask().
 *
 *  Note: this goes here rather than in signal.c because we need to
 *  provide this function even when not overriding signals.
 */
int
monitor_real_sigprocmask(int how, const sigset_t *set,
			 sigset_t *oldset)
{
    monitor_normal_init();
    return (*real_sigprocmask)(how, set, oldset);
}

/*
 *----------------------------------------------------------------------
 *  EXTERNAL OVERRIDES and their helper functions
 *----------------------------------------------------------------------
 */

/*
 *  Static case -- we get into __wrap_main() directly, by editing the
 *  link line so that the system call to main() comes here instead.
 *  In this case, there are no library init/fini functions.
 *
 *  Dynamic case -- we get into __libc_start_main() via LD_PRELOAD.
 */
int
__wrap_main(int argc, char **argv, char **envp)
{
    int ret;

    MONITOR_DEBUG1("\n");
    monitor_argc = argc;
    monitor_argv = argv;
    monitor_envp = envp;
#ifdef MONITOR_STATIC
    real_main = &__real_main;
#endif

    MONITOR_ASM_LABEL(monitor_unwind_fence1);
    main_stack_bottom = alloca(8);
    strncpy(main_stack_bottom, "stakbot", 8);
    monitor_begin_process_fcn();

    if (atexit(&monitor_end_process_fcn) != 0) {
	MONITOR_ERROR1("atexit failed\n");
    }
    ret = (*real_main)(monitor_argc, monitor_argv, monitor_envp);

    monitor_end_process_fcn();
    MONITOR_ASM_LABEL(monitor_unwind_fence2);

    return (ret);
}

#ifdef MONITOR_DYNAMIC
int
__libc_start_main(START_MAIN_PARAM_LIST)
{
    MONITOR_DEBUG1("\n");
    real_main = main;

    (*real_start_main)(__wrap_main, argc, argv, init, fini,
		       rtld_fini, stack_end);

    /* Never reached. */
    return (0);
}
#endif  /* MONITOR_DYNAMIC */

/*
 *  _exit and _Exit bypass library fini functions, so we need to call
 *  them here (in the dynamic case).
 */
void
MONITOR_WRAP_NAME(_exit)(int status)
{
    MONITOR_DEBUG1("\n");

    monitor_end_process_fcn();
#ifdef MONITOR_DYNAMIC
    monitor_end_library_fcn();
#endif

    (*real_u_exit)(status);

    /* Never reached, but silence a compiler warning. */
    exit(status);
}

void
MONITOR_WRAP_NAME(_Exit)(int status)
{
    MONITOR_DEBUG1("\n");

    monitor_end_process_fcn();
#ifdef MONITOR_DYNAMIC
    monitor_end_library_fcn();
#endif

    (*real_u_exit)(status);

    /* Never reached, but silence a compiler warning. */
    exit(status);
}

/*
 *  Library constructor (init) and destructor (fini) functions and
 *  __libc_start_main() are all dynamic only.
 */
#ifdef MONITOR_DYNAMIC
void __attribute__ ((constructor))
monitor_library_init_constructor(void)
{
    MONITOR_DEBUG1("\n");
    monitor_begin_library_fcn();
}

void __attribute__ ((destructor))
monitor_library_fini_destructor(void)
{
    MONITOR_DEBUG1("\n");
    monitor_end_library_fcn();
}

#endif  /* MONITOR_DYNAMIC */

/*
 *----------------------------------------------------------------------
 *  DEFAULT WEAK DEFINITIONS
 *----------------------------------------------------------------------
 */

/*
 *  Default, weak symbol definitions of monitor support functions.
 *  These definitions are superseded by other monitor modules, when
 *  those modules are present.  This allows compiling and linking a
 *  subset of monitor and delaying the choice of subset until link
 *  time (static case).
 */
void * __attribute__ ((weak))
monitor_stack_bottom(void)
{
    return (main_stack_bottom);
}

void * __attribute__ ((weak))
monitor_real_dlopen(const char *path, int flags)
{
    MONITOR_DEBUG1("(weak)\n");
    return (NULL);
}

int __attribute__ ((weak))
monitor_real_dlclose(void *handle)
{
    MONITOR_DEBUG1("(weak)\n");
    return (FAILURE);
}

int __attribute__ ((weak))
monitor_real_pthread_sigmask(int how, const sigset_t *set,
			     sigset_t *oldset)
{
    MONITOR_DEBUG1("(weak)\n");
    return (FAILURE);
}

int __attribute__ ((weak))
monitor_sigaction(int sig, monitor_sighandler_t *handler,
		  int flags, struct sigaction *act)
{
    MONITOR_DEBUG1("(weak)\n");
    return (FAILURE);
}

int __attribute__ ((weak))
monitor_unwind_thread_bottom_frame(void *addr)
{
    MONITOR_DEBUG1("(weak)\n");
    return (FALSE);
}

int __attribute__ ((weak))
monitor_mpi_comm_size(void)
{
    MONITOR_DEBUG1("(weak)\n");
    return (FAILURE);
}

int __attribute__ ((weak))
monitor_mpi_comm_rank(void)
{
    MONITOR_DEBUG1("(weak)\n");
    return (FAILURE);
}

/*
 *  Internal monitor functions.
 */
void __attribute__ ((weak))
monitor_thread_release(void)
{
    MONITOR_DEBUG1("(weak)\n");
    return;
}

void __attribute__ ((weak))
monitor_thread_shootdown(void)
{
    MONITOR_DEBUG1("(weak)\n");
    return;
}
