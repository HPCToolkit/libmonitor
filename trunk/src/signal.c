/*
 *  Libmonitor signal functions.
 *
 *  Copyright (c) 2007, Rice University.
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
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "monitor.h"

/*
 *----------------------------------------------------------------------
 *  MACROS
 *----------------------------------------------------------------------
 */

#ifdef NSIG
#define MONITOR_NSIG  (NSIG)
#else
#define MONITOR_NSIG  (128)
#endif

/*  Sa_flags that monitor requires and forbids.
 */
#define SAFLAGS_REQUIRED   (SA_SIGINFO | SA_RESTART)
#define SAFLAGS_FORBIDDEN  (SA_RESETHAND | SA_ONSTACK)

/*
 *----------------------------------------------------------------------
 *  GLOBAL VARIABLES
 *----------------------------------------------------------------------
 */

typedef int  sigaction_fcn_t(int, const struct sigaction *,
			    struct sigaction *);
typedef int  sigprocmask_fcn_t(int, const sigset_t *, sigset_t *);
typedef void sighandler_fcn_t(int);

#ifdef MONITOR_STATIC
extern sigaction_fcn_t    __real_sigaction;
extern sigprocmask_fcn_t  __real_sigprocmask;
#endif

static sigaction_fcn_t    *real_sigaction = NULL;
static sigprocmask_fcn_t  *real_sigprocmask = NULL;

struct monitor_signal_entry {
    struct sigaction  mse_appl_act;
    struct sigaction  mse_kern_act;
    monitor_sighandler_t  *mse_client_handler;
    int   mse_client_flags;
    char  mse_avoid;
    char  mse_invalid;
    char  mse_noterm;
    char  mse_stop;
};

static struct monitor_signal_entry
monitor_signal_array[MONITOR_NSIG];

/*  Signals that monitor treats as totally hands-off.
 */
static int monitor_signal_avoid_list[] = {
    SIGKILL, SIGSTOP, -1
};

/*  Signals whose default actions do not terminate the process.
 */
static int monitor_signal_noterm_list[] = {
    SIGCHLD, SIGCONT, SIGURG, SIGWINCH, -1
};

/*  Signals whose default action is to stop the process.
 */
static int monitor_signal_stop_list[] = {
    SIGTSTP, SIGTTIN, SIGTTOU, -1
};


/*
 *----------------------------------------------------------------------
 *  INTERNAL HELPER FUNCTIONS
 *----------------------------------------------------------------------
 */

/*
 *  Catch all signals here.  Offer them to the client first (if
 *  registered), or else apply the application's disposition.
 */
static void
monitor_signal_handler(int sig, siginfo_t *info, void *context)
{
    struct monitor_signal_entry *mse;
    struct sigaction action, *sa;
    int ret;

    if (sig <= 0 || sig >= MONITOR_NSIG ||
	monitor_signal_array[sig].mse_avoid ||
	monitor_signal_array[sig].mse_invalid) {
	MONITOR_WARN("invalid signal: %d\n", sig);
	return;
    }

    /*
     * Try the client first, if it has registered a handler.  If the
     * client returns, then its return code describes what further
     * action to take, where 0 means no further action.
     */
    mse = &monitor_signal_array[sig];
    if (mse->mse_client_handler != NULL) {
	ret = (mse->mse_client_handler)(sig, info, context);
	if (ret == 0)
	    return;
    }

    /*
     * Apply the applicaton's action: ignore, default or handler.
     */
    sa = &mse->mse_appl_act;
    if (sa->sa_handler == SIG_IGN) {
	/*
	 * Ignore the signal.
	 */
	return;
    }
    else if (sa->sa_handler == SIG_DFL) {
	/*
	 * Invoke the default action: ignore the signal, stop the
	 * process, or (by default) terminate the process.
	 *
	 * XXX - may want to siglongjmp() or _exit() directly before
	 * ending the process, or may want to restart some signals
	 * (SIGSEGV, SIGFPE, etc).
	 */
	if (mse->mse_noterm)
	    return;

	if (mse->mse_stop) {
	    raise(SIGSTOP);
	    return;
	}

	monitor_end_process_fcn();
#ifdef MONITOR_DYNAMIC
	monitor_end_library_fcn();
#endif
	action.sa_handler = SIG_DFL;
	action.sa_flags = 0;
	sigemptyset(&action.sa_mask);
	(*real_sigaction)(sig, &action, NULL);
	(*real_sigprocmask)(SIG_SETMASK, &action.sa_mask, NULL);
	raise(sig);
    }
    else {
	/*
	 * Invoke the application's handler.
	 */
	if (sa->sa_flags & SA_SIGINFO) {
	    (*sa->sa_sigaction)(sig, info, context);
	} else {
	    (*sa->sa_handler)(sig);
	}
    }
}

/*
 *  Install our signal hander for all signals except ones on the avoid
 *  list.
 */
void
monitor_signal_init(void)
{
    struct monitor_signal_entry *mse;
    struct sigaction *sa;
    int i, sig, ret, num_avoid, num_valid, num_invalid;

    MONITOR_RUN_ONCE(signal_init);
    MONITOR_GET_REAL_NAME_WRAP(real_sigaction, sigaction);
    MONITOR_GET_REAL_NAME_WRAP(real_sigprocmask, sigprocmask);

    memset(monitor_signal_array, 0, sizeof(monitor_signal_array));
    for (i = 0; monitor_signal_avoid_list[i] > 0; i++) {
	monitor_signal_array[monitor_signal_avoid_list[i]].mse_avoid = 1;
    }
    for (i = 0; monitor_signal_noterm_list[i] > 0; i++) {
	monitor_signal_array[monitor_signal_noterm_list[i]].mse_noterm = 1;
    }
    for (i = 0; monitor_signal_stop_list[i] > 0; i++) {
	monitor_signal_array[monitor_signal_stop_list[i]].mse_stop = 1;
    }

    num_avoid = 0;
    num_valid = 0;
    num_invalid = 0;
    for (sig = 1; sig < MONITOR_NSIG; sig++) {
	mse = &monitor_signal_array[sig];
	if (mse->mse_avoid) {
	    num_avoid++;
	} else {
	    sa = &mse->mse_kern_act;
	    sa->sa_sigaction = &monitor_signal_handler;
	    sigemptyset(&sa->sa_mask);
	    sa->sa_flags = SAFLAGS_REQUIRED;
	    ret = (*real_sigaction)(sig, sa, &mse->mse_appl_act);
	    if (ret == 0) {
		num_valid++;
	    } else {
		mse->mse_invalid = 1;
		num_invalid++;
	    }
	}
    }

    MONITOR_DEBUG("valid: %d, invalid: %d, avoid: %d, max signum: %d\n",
		  num_valid, num_invalid, num_avoid, MONITOR_NSIG - 1);
}

/*
 *  Delete from "set" any signals that the client catches.
 */
void
monitor_remove_client_signals(sigset_t *set)
{
    struct monitor_signal_entry *mse;
    int sig;

    for (sig = 1; sig < MONITOR_NSIG; sig++) {
	mse = &monitor_signal_array[sig];
	if (!mse->mse_avoid && !mse->mse_invalid &&
	    mse->mse_client_handler != NULL) {
	    sigdelset(set, sig);
	}
    }
}

/*
 *  Adjust sa_flags according to the required and forbidden sets.
 */
static inline int
monitor_adjust_saflags(int flags)
{
    return (flags | SAFLAGS_REQUIRED) & ~(SAFLAGS_FORBIDDEN);
}

/*
 *----------------------------------------------------------------------
 *  SIGACTION OVERRIDES and EXTERNAL FUNCTIONS
 *----------------------------------------------------------------------
 */

/*
 *  The client's sigaction.  If "act" is non-NULL, then use it for
 *  sa_flags and sa_mask, but "flags" are unused for now.
 *
 *  Returns: 0 on success, -1 if invalid signal number.
 */
int
monitor_sigaction(int sig, monitor_sighandler_t *handler,
		  int flags, struct sigaction *act)
{
    struct monitor_signal_entry *mse;
    sigset_t my_set;

    monitor_signal_init();
    if (sig <= 0 || sig >= MONITOR_NSIG ||
	monitor_signal_array[sig].mse_avoid ||
	monitor_signal_array[sig].mse_invalid) {
	MONITOR_DEBUG("client sigaction: %d (invalid)\n", sig);
	return (-1);
    }

    MONITOR_DEBUG("client sigaction: %d (caught)\n", sig);
    mse = &monitor_signal_array[sig];
    mse->mse_client_handler = handler;
    mse->mse_client_flags = flags;
    if (act != NULL) {
	mse->mse_kern_act.sa_flags = monitor_adjust_saflags(act->sa_flags);
	mse->mse_kern_act.sa_mask = act->sa_mask;
	monitor_remove_client_signals(&mse->mse_kern_act.sa_mask);
	(*real_sigaction)(sig, &mse->mse_kern_act, NULL);
    }

    return (0);
}

/*
 *  Override the application's signal(2) and sigaction(2).
 *
 *  Returns: 0 on success, -1 if invalid signal number.
 */
static int
monitor_appl_sigaction(int sig, const struct sigaction *act,
		       struct sigaction *oldact)
{
    struct monitor_signal_entry *mse;

    monitor_signal_init();
    if (sig <= 0 || sig >= MONITOR_NSIG ||
	monitor_signal_array[sig].mse_invalid) {
	MONITOR_DEBUG("application sigaction: %d (invalid)\n", sig);
	errno = EINVAL;
	return (-1);
    }

    /*
     * Use the system sigaction for signals on the avoid list.
     */
    mse = &monitor_signal_array[sig];
    if (mse->mse_avoid) {
	MONITOR_DEBUG("application sigaction: %d (avoid)\n", sig);
	return (*real_sigaction)(sig, act, oldact);
    }

    MONITOR_DEBUG("application sigaction: %d (caught)\n", sig);
    if (oldact != NULL) {
	*oldact = mse->mse_appl_act;
    }
    if (act != NULL) {
	/*
	 * Use the application's flags and mask, adjusted for
	 * monitor's needs.
	 */
	mse->mse_appl_act = *act;
	mse->mse_kern_act.sa_flags = monitor_adjust_saflags(act->sa_flags);
	mse->mse_kern_act.sa_mask = act->sa_mask;
	monitor_remove_client_signals(&mse->mse_kern_act.sa_mask);
	(*real_sigaction)(sig, &mse->mse_kern_act, NULL);
    }

    return (0);
}

int
MONITOR_WRAP_NAME(sigaction)(int sig, const struct sigaction *act,
			     struct sigaction *oldact)
{
    return monitor_appl_sigaction(sig, act, oldact);
}

sighandler_fcn_t *
MONITOR_WRAP_NAME(signal)(int sig, sighandler_fcn_t *handler)
{
    struct sigaction act, oldact;
    int ret;

    act.sa_handler = handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);

    if (monitor_appl_sigaction(sig, &act, &oldact) == 0)
	return (oldact.sa_handler);
    else {
	errno = EINVAL;
	return (SIG_ERR);
    }
}

/*
 *  Allow the application to modify the signal proc mask, but don't
 *  let it block a signal that the client catches.
 */
int
MONITOR_WRAP_NAME(sigprocmask)(int how, const sigset_t *set,
			       sigset_t *oldset)
{
    sigset_t my_set;

    monitor_signal_init();
    if (how == SIG_BLOCK || how == SIG_SETMASK) {
	my_set = *set;
	monitor_remove_client_signals(&my_set);
	set = &my_set;
    }

    return (*real_sigprocmask)(how, set, oldset);
}
