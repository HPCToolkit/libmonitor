/*
 *  Libmonitor fork and exec functions.
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
 *
 *  Override functions:
 *
 *    fork, vfork
 *    execl, execlp, execle, execv, execvp, execve
 *    system
 *
 *  Support functions:
 *
 *    monitor_real_fork
 */

#include "config.h"
#include <sys/types.h>
#include <sys/wait.h>
#ifdef MONITOR_DYNAMIC
#include <dlfcn.h>
#endif
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "monitor.h"

/*
 *----------------------------------------------------------------------
 *  GLOBAL VARIABLES and EXTERNAL FUNCTIONS
 *----------------------------------------------------------------------
 */

typedef pid_t fork_fcn_t(void);
typedef int execv_fcn_t(const char *path, char *const argv[]);
typedef int execve_fcn_t(const char *path, char *const argv[],
			 char *const envp[]);
typedef int sigaction_fcn_t(int, const struct sigaction *,
			    struct sigaction *);
typedef int sigprocmask_fcn_t(int, const sigset_t *, sigset_t *);

#ifdef MONITOR_STATIC
extern fork_fcn_t    __real_fork;
extern execv_fcn_t   __real_execv;
extern execv_fcn_t   __real_execvp;
extern execve_fcn_t  __real_execve;
extern sigaction_fcn_t    __real_sigaction;
extern sigprocmask_fcn_t  __real_sigprocmask;
#endif

static fork_fcn_t    *real_fork = NULL;
static execv_fcn_t   *real_execv = NULL;
static execv_fcn_t   *real_execvp = NULL;
static execve_fcn_t  *real_execve = NULL;
static sigaction_fcn_t    *real_sigaction = NULL;
static sigprocmask_fcn_t  *real_sigprocmask = NULL;

/*
 *----------------------------------------------------------------------
 *  INTERNAL HELPER FUNCTIONS
 *----------------------------------------------------------------------
 */

static void
monitor_fork_init(void)
{
    MONITOR_RUN_ONCE(fork_init);

    monitor_early_init();
    MONITOR_GET_REAL_NAME_WRAP(real_fork, fork);
    MONITOR_GET_REAL_NAME_WRAP(real_execv, execv);
    MONITOR_GET_REAL_NAME_WRAP(real_execvp, execvp);
    MONITOR_GET_REAL_NAME_WRAP(real_execve, execve);
    MONITOR_GET_REAL_NAME_WRAP(real_sigaction, sigaction);
    MONITOR_GET_REAL_NAME_WRAP(real_sigprocmask, sigprocmask);
}

/*
 *  Copy the execl() argument list of first_arg followed by arglist
 *  into an argv array, including the terminating NULL.  If envp is
 *  non-NULL, then there is an extra argument after NULL.  va_start
 *  and va_end are in the calling function.
 */
#define MONITOR_INIT_ARGV_SIZE  32
static void
monitor_copy_va_args(char ***argv, char ***envp,
		     const char *first_arg, va_list arglist)
{
    int argc, size = MONITOR_INIT_ARGV_SIZE;
    char *arg;

    *argv = malloc(size * sizeof(char *));
    if (*argv == NULL) {
	MONITOR_ERROR1("malloc failed\n");
    }

    /*
     * Include the terminating NULL in the argv array.
     */
    (*argv)[0] = (char *)first_arg;
    argc = 1;
    do {
	arg = va_arg(arglist, char *);
	if (argc >= size) {
	    size *= 2;
	    *argv = realloc(*argv, size * sizeof(char *));
	    if (*argv == NULL) {
		MONITOR_ERROR1("realloc failed\n");
	    }
	}
	(*argv)[argc++] = arg;
    } while (arg != NULL);

    if (envp != NULL) {
	*envp = va_arg(arglist, char **);
    }
}

/*
 *  Approximate whether the real exec will succeed by testing if
 *  "file" is executable, either the pathname itself or somewhere on
 *  PATH, depending on whether file contains '/'.  The problem is that
 *  we have to decide whether to call monitor_fini_process() before we
 *  know if exec succeeds, since exec doesn't return on success.
 *
 *  Returns: 1 if "file" is executable, else 0.
 */
#define COLON  ":"
#define SLASH  '/'
static int
monitor_is_executable(const char *file)
{
    char buf[PATH_MAX], *path;
    int  file_len, path_len;

    if (file == NULL) {
	MONITOR_DEBUG1("attempt to exec NULL path\n");
	return (0);
    }
    /*
     * If file contains '/', then try just the file name itself,
     * otherwise, search for it on PATH.
     */
    if (strchr(file, SLASH) != NULL)
	return (access(file, X_OK) == 0);

    file_len = strlen(file);
    path = getenv("PATH");
    path += strspn(path, COLON);
    while (*path != 0) {
	path_len = strcspn(path, COLON);
	if (path_len + file_len + 2 > PATH_MAX) {
	    MONITOR_DEBUG("path length %d exceeds PATH_MAX %d\n",
			  path_len + file_len + 2, PATH_MAX);
	}
	memcpy(buf, path, path_len);
	buf[path_len] = SLASH;
	memcpy(&buf[path_len + 1], file, file_len);
	buf[path_len + 1 + file_len] = 0;
	if (access(buf, X_OK) == 0)
	    return (1);
	path += path_len;
	path += strspn(path, COLON);
    }

    return (0);
}

/*
 *----------------------------------------------------------------------
 *  CLIENT SUPPORT FUNCTIONS
 *----------------------------------------------------------------------
 */

pid_t
monitor_real_fork(void)
{
    monitor_fork_init();
    return (*real_fork)();
}

/*
 *----------------------------------------------------------------------
 *  FORK and EXEC OVERRIDE FUNCTIONS
 *----------------------------------------------------------------------
 */

/*
 *  Override fork() and vfork().
 *
 *  Note: vfork must be immediately followed by exec and can't even
 *  return from our override function.  Thus, we have to use the real
 *  fork (not the real vfork) in both cases.
 */
static pid_t
monitor_fork(void)
{
    void *user_data;
    pid_t ret;

    monitor_fork_init();
    MONITOR_DEBUG1("calling monitor_pre_fork() ...\n");
    user_data = monitor_pre_fork();

    ret = (*real_fork)();
    if (ret != 0) {
	/* Parent process. */
	if (ret < 0) {
	    MONITOR_DEBUG("real fork failed (%d): %s\n",
			  errno, strerror(errno));
	}
	MONITOR_DEBUG1("calling monitor_post_fork() ...\n");
	monitor_post_fork(ret, user_data);
    }
    else {
	/* Child process. */
	MONITOR_DEBUG("application forked, parent = %d\n", (int)getppid());
	monitor_begin_process_fcn(user_data, TRUE);
    }

    return (ret);
}

pid_t
MONITOR_WRAP_NAME(fork)(void)
{
    return monitor_fork();
}

pid_t
MONITOR_WRAP_NAME(vfork)(void)
{
    return monitor_fork();
}

/*
 *  Override execl() and execv().
 *
 *  Note: we test if "path" is executable as an approximation to
 *  whether exec will succeed, and thus whether we should call
 *  monitor_fini_process.  But we still try the real exec in either
 *  case.
 */
static int
monitor_execv(const char *path, char *const argv[])
{
    int ret, is_exec;

    monitor_fork_init();
    is_exec = access(path, X_OK) == 0;
    MONITOR_DEBUG("about to execv, expecting %s, pid: %d, path: %s\n",
		  (is_exec ? "success" : "failure"),
		  (int)getpid(), path);
    if (is_exec) {
	monitor_end_process_fcn(MONITOR_EXIT_EXEC);
#ifdef MONITOR_DYNAMIC
	monitor_end_library_fcn();
#endif
    }
    ret = (*real_execv)(path, argv);

    /* We only get here if real_execv fails. */
    if (is_exec) {
	MONITOR_WARN("unexpected execv failure on pid: %d\n",
		     (int)getpid());
    }
    return (ret);
}

/*
 *  Override execlp() and execvp().
 */
static int
monitor_execvp(const char *file, char *const argv[])
{
    int ret, is_exec;

    monitor_fork_init();
    is_exec = monitor_is_executable(file);
    MONITOR_DEBUG("about to execvp, expecting %s, pid: %d, file: %s\n",
		  (is_exec ? "success" : "failure"),
		  (int)getpid(), file);
    if (is_exec) {
	monitor_end_process_fcn(MONITOR_EXIT_EXEC);
#ifdef MONITOR_DYNAMIC
	monitor_end_library_fcn();
#endif
    }
    ret = (*real_execvp)(file, argv);

    /* We only get here if real_execvp fails. */
    if (is_exec) {
	MONITOR_WARN("unexpected execvp failure on pid: %d\n",
		     (int)getpid());
    }
    return (ret);
}

/*
 *  Override execle() and execve().
 */
static int
monitor_execve(const char *path, char *const argv[], char *const envp[])
{
    int ret, is_exec;

    monitor_fork_init();
    is_exec = access(path, X_OK) == 0;
    MONITOR_DEBUG("about to execve, expecting %s, pid: %d, path: %s\n",
		  (is_exec ? "success" : "failure"),
		  (int)getpid(), path);
    if (is_exec) {
	monitor_end_process_fcn(MONITOR_EXIT_EXEC);
#ifdef MONITOR_DYNAMIC
	monitor_end_library_fcn();
#endif
    }
    ret = (*real_execve)(path, argv, envp);

    /* We only get here if real_execve fails. */
    if (is_exec) {
	MONITOR_WARN("unexpected execve failure on pid: %d\n",
		     (int)getpid());
    }
    return (ret);
}

/*
 *  There's a subtlety in the signatures between the variadic args in
 *  execl() and the argv array in execv().
 *
 *    execl(const char *path, const char *arg, ...)
 *    execv(const char *path, char *const argv[])
 *
 *  In execv(), the elements of argv[] are const, but not the strings
 *  they point to.  Since we can't change the system declaratons,
 *  somewhere we have to drop the const.  (ugh)
 */
int
MONITOR_WRAP_NAME(execl)(const char *path, const char *arg, ...)
{
    char **argv;
    va_list arglist;

    va_start(arglist, arg);
    monitor_copy_va_args(&argv, NULL, arg, arglist);
    va_end(arglist);

    return monitor_execv(path, argv);
}

int
MONITOR_WRAP_NAME(execlp)(const char *file, const char *arg, ...)
{
    char **argv;
    va_list arglist;

    va_start(arglist, arg);
    monitor_copy_va_args(&argv, NULL, arg, arglist);
    va_end(arglist);

    return monitor_execvp(file, argv);
}

int
MONITOR_WRAP_NAME(execle)(const char *path, const char *arg, ...)
{
    char **argv, **envp;
    va_list arglist;

    va_start(arglist, arg);
    monitor_copy_va_args(&argv, &envp, arg, arglist);
    va_end(arglist);

    return monitor_execve(path, argv, envp);
}

int
MONITOR_WRAP_NAME(execv)(const char *path, char *const argv[])
{
    return monitor_execv(path, argv);
}

int
MONITOR_WRAP_NAME(execvp)(const char *path, char *const argv[])
{
    return monitor_execvp(path, argv);
}

int
MONITOR_WRAP_NAME(execve)(const char *path, char *const argv[],
			  char *const envp[])
{
    return monitor_execve(path, argv, envp);
}

/*
 *  Reimplement and override system().  Stevens describes the issues
 *  with signals and how to do this.
 *
 *  This allows us to do three things: (1) replace vfork() with fork,
 *  (2) provide pre/post_fork() callbacks, and (3) selectively monitor
 *  or not the child process.  Note: the libc system() does a direct
 *  syscall for fork, thus bypassing our override.
 *
 *  For now, always un-monitor the child by just unsetting LD_PRELOAD.
 */
#define SHELL  "/bin/sh"
int
MONITOR_WRAP_NAME(system)(const char *command)
{
    struct sigaction ign_act, old_int, old_quit;
    sigset_t sigchld_set, old_set;
    char *arglist[4];
    void *user_data;
    pid_t pid;
    int status;

    monitor_fork_init();
    MONITOR_DEBUG("command = %s\n", command);
    /*
     * command == NULL tests if the shell is available and returns
     * non-zero if yes.
     */
    if (command == NULL) {
	status = !access(SHELL, X_OK);
	MONITOR_DEBUG("status = %d\n", status);
	return (status);
    }
    /*
     * Ignore SIGINT and SIGQUIT, and block SIGCHLD.
     */
    memset(&ign_act, 0, sizeof(struct sigaction));
    ign_act.sa_handler = SIG_IGN;
    sigemptyset(&sigchld_set);
    sigaddset(&sigchld_set, SIGCHLD);

    MONITOR_DEBUG1("calling monitor_pre_fork() ...\n");
    user_data = monitor_pre_fork();
    (*real_sigaction)(SIGINT, &ign_act, &old_int);
    (*real_sigaction)(SIGQUIT, &ign_act, &old_quit);
    (*real_sigprocmask)(SIG_BLOCK, &sigchld_set, &old_set);

    pid = (*real_fork)();
    if (pid < 0) {
	/* Fork failed. */
	MONITOR_DEBUG1("real fork failed\n");
	status = -1;
    }
    else if (pid == 0) {
	/* Child process. */
	(*real_sigaction)(SIGINT, &old_int, NULL);
	(*real_sigaction)(SIGQUIT, &old_quit, NULL);
	(*real_sigprocmask)(SIG_SETMASK, &old_set, NULL);
	arglist[0] = SHELL;
	arglist[1] = "-c";
	arglist[2] = (char *)command;
	arglist[3] = NULL;
	unsetenv("LD_PRELOAD");
	(*real_execv)("/bin/sh", arglist);
	monitor_real_exit(127);
    }
    else {
	/* Parent process. */
	while (waitpid(pid, &status, 0) < 0) {
	    if (errno != EINTR) {
		status = -1;
		break;
	    }
	}
    }

    (*real_sigaction)(SIGINT, &old_int, NULL);
    (*real_sigaction)(SIGQUIT, &old_quit, NULL);
    (*real_sigprocmask)(SIG_SETMASK, &old_set, NULL);

    MONITOR_DEBUG1("calling monitor_post_fork() ...\n");
    monitor_post_fork(pid, user_data);

    MONITOR_DEBUG("status = %d\n", status);
    return (status);
}
