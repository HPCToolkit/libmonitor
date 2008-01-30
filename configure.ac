dnl
dnl  Libmonitor top-level configure.ac.
dnl
dnl  Copyright (c) 2007-2008, Rice University.
dnl  All rights reserved.
dnl
dnl  Redistribution and use in source and binary forms, with or without
dnl  modification, are permitted provided that the following conditions are
dnl  met:
dnl
dnl  * Redistributions of source code must retain the above copyright
dnl    notice, this list of conditions and the following disclaimer.
dnl
dnl  * Redistributions in binary form must reproduce the above copyright
dnl    notice, this list of conditions and the following disclaimer in the
dnl    documentation and/or other materials provided with the distribution.
dnl
dnl  * Neither the name of Rice University (RICE) nor the names of its
dnl    contributors may be used to endorse or promote products derived from
dnl    this software without specific prior written permission.
dnl
dnl  This software is provided by RICE and contributors "as is" and any
dnl  express or implied warranties, including, but not limited to, the
dnl  implied warranties of merchantability and fitness for a particular
dnl  purpose are disclaimed. In no event shall RICE or contributors be
dnl  liable for any direct, indirect, incidental, special, exemplary, or
dnl  consequential damages (including, but not limited to, procurement of
dnl  substitute goods or services; loss of use, data, or profits; or
dnl  business interruption) however caused and on any theory of liability,
dnl  whether in contract, strict liability, or tort (including negligence
dnl  or otherwise) arising in any way out of the use of this software, even
dnl  if advised of the possibility of such damage.
dnl
dnl  $Id$
dnl

AC_INIT([libmonitor], [0.72])
AC_COPYRIGHT([libmonitor is Copyright (c) 2007-2008, Rice University.
See the file LICENSE for details.])

AC_CONFIG_AUX_DIR([config])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AM_MAINTAINER_MODE

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_CONFIG_FILES([src/monitor-link], [chmod +x src/monitor-link])
AC_CONFIG_FILES([src/monitor-run],  [chmod +x src/monitor-run])

AC_PROG_CC
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

AC_SUBST([wrap_list], ["main _exit"])
AC_SUBST([mpi_cpp_flags])

#
# Option: --enable-debug=no
#
AC_ARG_ENABLE([dummy-blank-line], [AS_HELP_STRING([ ], [ ])])
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
	[debug mode is always on [default=no]])],
    [],
    [enable_debug=no])

if test "x$enable_debug" = xyes ; then
    AC_DEFINE([MONITOR_DEBUG_DEFAULT_ON], [1],
	[Debug mode defaults to on.])
fi

#
# Option: --enable-link-preload=yes
# Requires dlsym and __libc_start_main.
#
AC_ARG_ENABLE([link-preload],
    [AS_HELP_STRING([--enable-link-preload],
	[build libmonitor.so shared library to run monitor with LD_PRELOAD
	[default=yes]])],
    [],
    [enable_link_preload=yes])

if test "x$enable_link_preload" = xyes ; then
    AC_SEARCH_LIBS([dlsym], [dl], [], [enable_link_preload=no])
fi
if test "x$enable_link_preload" = xyes ; then
    AC_SEARCH_LIBS([__libc_start_main], [c], [], [enable_link_preload=no])
fi

AM_CONDITIONAL([MONITOR_TEST_LINK_PRELOAD],
    [test x$enable_link_preload = xyes])

#
# Option: --enable-link-static=yes
#
AC_ARG_ENABLE([link-static],
    [AS_HELP_STRING([--enable-link-static],
	[build libmonitor_wrap.a library to link monitor statically
	[default=yes]])],
    [],
    [enable_link_static=yes])

AM_CONDITIONAL([MONITOR_TEST_LINK_STATIC],
    [test x$enable_link_static = xyes])

#
# Option: --enable-dlfcn=yes
# Note: AC uses 'enable_dlopen', so we use dlfcn.
#
AC_ARG_ENABLE([dlfcn],
    [AS_HELP_STRING([--enable-dlfcn],
	[include support for dlopen [default=yes]])],
    [],
    [enable_dlfcn=yes])

if test "x$enable_dlfcn" = xyes ; then
    AC_SEARCH_LIBS([dlopen], [dl], [], [enable_dlfcn=no])
fi

if test "x$enable_dlfcn" = xyes ; then
    AC_DEFINE([MONITOR_USE_DLOPEN], [1],
	[Include support for dlopen.])
    wrap_list="${wrap_list} dlopen dlclose"
fi

AM_CONDITIONAL([MONITOR_TEST_USE_DLOPEN],
    [test x$enable_dlfcn = xyes])

#
# Option: --enable-fork=yes
# Requires fork and exec families.
#
AC_ARG_ENABLE([fork],
    [AS_HELP_STRING([--enable-fork],
        [include support for fork and exec families [default=yes]])],
    [],
    [enable_fork=yes])

if test "x$enable_fork" = xyes ; then
    AC_SEARCH_LIBS([fork], [c], [], [enable_fork=no])
fi
if test "x$enable_fork" = xyes ; then
    AC_SEARCH_LIBS([execve], [c], [], [enable_fork=no])
fi
if test "x$enable_fork" = xyes ; then
    AC_DEFINE([MONITOR_USE_FORK], [1],
	[Include support for fork and exec families.])
    wrap_list="${wrap_list} fork vfork"
    wrap_list="${wrap_list} execl execlp execle execv execvp execve"
fi

AM_CONDITIONAL([MONITOR_TEST_USE_FORK],
    [test x$enable_fork = xyes])

#
# Option: --enable-pthreads=yes
# Requires libpthread.
#
AC_ARG_ENABLE([pthreads],
    [AS_HELP_STRING([--enable-pthreads],
	[include pthread support [default=yes]])],
    [],
    [enable_pthreads=yes])

if test "x$enable_pthreads" = xyes ; then
    AC_SEARCH_LIBS([pthread_create], [pthread], [], [enable_pthreads=no])
fi

if test "x$enable_pthreads" = xyes ; then
    AC_DEFINE([MONITOR_USE_PTHREADS], [1],
	[Include support for pthreads.])
    wrap_list="${wrap_list} pthread_create"
    #
    # See if pthread_equal and pthread_cleanup_push/pop are macros or
    # library functions.
    #
    AC_CHECK_LIB([pthread], [pthread_equal],
        [AC_DEFINE([MONITOR_PTHREAD_EQUAL_IS_FCN], [1],
	    [pthread_equal is a function, not a macro.])])
    AC_CHECK_LIB([pthread], [pthread_cleanup_push],
        [AC_DEFINE([MONITOR_PTHREAD_CLEANUP_PUSH_IS_FCN], [1],
	    [pthread_cleanup_push is a function, not a macro.])])
fi

AM_CONDITIONAL([MONITOR_TEST_USE_PTHREADS],
    [test x$enable_pthreads = xyes])

#
# Option: --enable-signals=yes
#
AC_ARG_ENABLE([signals],
    [AS_HELP_STRING([--enable-signals],
        [include support for signals [default=yes]])],
    [],
    [enable_signals=yes])

if test "x$enable_signals" = xyes ; then
    AC_SEARCH_LIBS([sigaction], [c], [], [enable_signals=no])
fi
if test "x$enable_signals" = xyes ; then
    AC_DEFINE([MONITOR_USE_SIGNALS], [1],
	[Include support for signals.])
    wrap_list="${wrap_list} signal sigaction sigprocmask"
fi

AM_CONDITIONAL([MONITOR_TEST_USE_SIGNALS],
    [test x$enable_signals = xyes])

if test "x$enable_pthreads" = xyes && test "x$enable_signals" = xyes ; then
    wrap_list="${wrap_list} pthread_sigmask"
fi

#
# Option: --with-mpi=/path/to/MPI/install/dir
#
AC_ARG_WITH([mpi],
    [AS_HELP_STRING([--with-mpi=PATH],
	[path to MPI install directory [default=no]])],
    [],
    [with_mpi=no])

if test "x$with_mpi" != xno ; then
    AC_CHECK_FILE(["${with_mpi}/include/mpi.h"], [], [with_mpi=no])
fi
if test "x$with_mpi" != xno ; then
    AC_DEFINE([MONITOR_USE_MPI], [1],
	[Include support for MPI.])
    mpi_cpp_flags="-I ${with_mpi}/include"
    wrap_list="${wrap_list} MPI_Init MPI_Finalize"
    wrap_list="${wrap_list} mpi_init_ mpi_finalize_"
    wrap_list="${wrap_list} mpi_init__ mpi_finalize__"
fi

AM_CONDITIONAL([MONITOR_TEST_USE_MPI],
    [test x$with_mpi != xno])

AC_OUTPUT

#
# Display a summary of the config options.
#
AC_MSG_NOTICE([monitor prefix: $prefix])
AC_MSG_NOTICE([monitor build: $build])
if test "x$host" != "x$build" ; then
    AC_MSG_NOTICE([monitor host: $host])
fi
if test "x$target" != x && test "x$target" != "x$build" ; then
    AC_MSG_NOTICE([monitor target: $target])
fi
AC_MSG_NOTICE([monitor CC CFLAGS: $CC $CFLAGS])
AC_MSG_NOTICE([monitor debug mode: $enable_debug])
AC_MSG_NOTICE([monitor link preload: $enable_link_preload])
AC_MSG_NOTICE([monitor link static: $enable_link_static])
AC_MSG_NOTICE([monitor use dlfcn: $enable_dlfcn])
AC_MSG_NOTICE([monitor use fork: $enable_fork])
AC_MSG_NOTICE([monitor use pthreads: $enable_pthreads])
AC_MSG_NOTICE([monitor use signals: $enable_signals])
AC_MSG_NOTICE([monitor with mpi: $with_mpi])
