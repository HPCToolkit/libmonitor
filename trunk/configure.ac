dnl
dnl  Libmonitor top-level configure.ac.
dnl
dnl  Copyright (c) 2007-2012, Rice University.
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

AC_INIT([libmonitor], [0.x], [hpctoolkit-forum@rice.edu])
AC_COPYRIGHT([libmonitor is Copyright (c) 2007-2012, Rice University.
See the file LICENSE for details.])

AC_CONFIG_AUX_DIR([config])
AM_INIT_AUTOMAKE([foreign -Wall -Werror no-dist])
AM_MAINTAINER_MODE
AC_PREREQ(2.67)
AM_INIT_AUTOMAKE(1.11.1)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_CONFIG_FILES([src/monitor-link], [chmod +x src/monitor-link])
AC_CONFIG_FILES([src/monitor-run],  [chmod +x src/monitor-run])

#
# Disable building libmonitor.a, since nothing uses it.  The static
# case uses libmonitor_wrap.a, not libmonitor.a.  This needs to come
# before PROG_LIBTOOL.
#
enable_static=no

AC_PROG_CC
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AC_SUBST([wrap_list], ["main exit _exit"])

#------------------------------------------------------------
# CPU type
#------------------------------------------------------------

case "$host_cpu" in
    [*[pP][pP][cC]*] | [*[pP][oO][wW][eE][rR][pP][cC]*] )
	host_cpu_ppc=yes
	;;
esac

#------------------------------------------------------------
# CFLAGS and Fence Labels
#------------------------------------------------------------

# On ia64, gcc at -O2 or higher will duplicate code after a branch to
# make long pipelines.  This breaks our fence labels because it
# duplicates the symbol.  Here, we test for this and try lowering the
# optimization level if needed.

AC_LANG_PUSH([C])

orig_cflags="$CFLAGS"
cflags_noopt=
warn_cflags_opt=no

for opt in $CFLAGS
do
    case "$opt" in
	-O* ) ;;
	* ) cflags_noopt="$cflags_noopt $opt" ;;
    esac
done

for opt in orig -O1 none fail
do
    case "$opt" in
	orig )
	    CFLAGS="$orig_cflags"
	    warn_cflags_opt=no
	    ;;
	none )
	    CFLAGS="$cflags_noopt"
	    warn_cflags_opt=yes
	    ;;
	fail )
	    AC_MSG_ERROR([unable to find CFLAGS that work with asm fence labels])
	    ;;
	* )
	    CFLAGS="$cflags_noopt $opt"
	    warn_cflags_opt=yes
	    ;;
    esac

    AC_MSG_CHECKING([if asm fence labels work with CFLAGS='$CFLAGS'])

    AC_COMPILE_IFELSE([
extern void fence;
int baz(void);
int foo(int x)
{
  int ret = 0;
  if (x != 0) {
    ret = baz();
  }
  asm (".globl fence");
  asm ("fence:");
  return ret;
}
], [ans=yes], [ans=no])

    AC_MSG_RESULT([$ans])
    if test "$ans" = yes ; then
	break
    fi
done

AC_LANG_POP

#------------------------------------------------------------
# GNU atomic ops
#------------------------------------------------------------

# Test if CC supports GNU atomic ops: __sync_val_compare_and_swap()

AC_MSG_CHECKING([if CC supports GNU atomic ops])

AC_LANG_PUSH([C])

AC_LINK_IFELSE([
#include <stdio.h>
int main(int argc, char **argv)
{
    long x = (long) argc;
    x = __sync_val_compare_and_swap(&x, 2L, 3L);
    printf("%ld", x);
    return 0;
}
],  [use_gnu_atomic_ops=yes],
    [use_gnu_atomic_ops=no])

AC_MSG_RESULT([$use_gnu_atomic_ops])

AC_LANG_POP([C])

AM_CONDITIONAL([MONITOR_TEST_USE_GNU_ATOMIC_OPS],
    [test x$use_gnu_atomic_ops = xyes])

#------------------------------------------------------------
# SVN Revision
#------------------------------------------------------------

# If this directory is an svn sandbox, then use svn info (and store
# the result in a file), else expect the answer to be in the file.
# Note: svn update changes the revision number, so svn info must take
# precedence over the file.

version_file="${srcdir}/.svn-revision"

AC_MSG_CHECKING([for svn revision])

svn_revision=`svn info $srcdir 2>/dev/null | grep -e '^Revision' | awk '{ print $2 }'`
if test "x$svn_revision" != x ; then
    rm -f "$version_file"
    echo "$svn_revision" > "$version_file"
elif test -f "$version_file" ; then
    svn_revision=`cat $version_file`
else
    svn_revision=0
fi

AC_MSG_RESULT([$svn_revision])

if test "$svn_revision" = 0 ; then
    AC_MSG_WARN([unable to find svn revision])
fi

AC_SUBST([svn_revision])
AC_DEFINE_UNQUOTED([SVN_REVISION], [$svn_revision], [SVN rev number.])

#
# Option: --enable-debug=no
#
AC_ARG_ENABLE([dummy-blank-line], [AS_HELP_STRING([], [])])
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
    wrap_list="${wrap_list} fork vfork system"
    wrap_list="${wrap_list} execl execlp execle execv execvp execve"
fi

AM_CONDITIONAL([MONITOR_TEST_USE_FORK],
    [test x$enable_fork = xyes])

#
# Option: --enable-mpi=yes
#
# Note: we try to support MPI in a generic way that is independent of
# the MPI implementation, so this option is just yes/no and doesn't
# require the <mpi.h> header.
#
AC_ARG_ENABLE([mpi],
    [AS_HELP_STRING([--enable-mpi],
        [include support for MPI [default=yes]])],
    [],
    [enable_mpi=yes])

if test "x$enable_mpi" = xyes ; then
    AC_DEFINE([MONITOR_USE_MPI], [1], [Include support for MPI.])
    mpi_c="MPI_Init MPI_Init_thread MPI_Finalize MPI_Comm_rank"
    mpi_f0="mpi_init mpi_init_thread mpi_finalize mpi_comm_rank"
    mpi_f1="mpi_init_ mpi_init_thread_ mpi_finalize_ mpi_comm_rank_"
    mpi_f2="mpi_init__ mpi_init_thread__ mpi_finalize__ mpi_comm_rank__"
    wrap_list="${wrap_list} ${mpi_c} ${mpi_f0} ${mpi_f1} ${mpi_f2}"
fi

AM_CONDITIONAL([MONITOR_TEST_USE_MPI], [test x$enable_mpi = xyes])

#
# Option: --enable-pthreads=yes
# Requires libpthread.
#
AC_ARG_ENABLE([pthreads],
    [AS_HELP_STRING([--enable-pthreads],
	[include support for pthreads [default=yes]])],
    [],
    [enable_pthreads=yes])

if test "x$enable_pthreads" = xyes ; then
    AC_SEARCH_LIBS([pthread_create], [pthread], [], [enable_pthreads=no])
fi

if test "x$enable_pthreads" = xyes ; then
    AC_DEFINE([MONITOR_USE_PTHREADS], [1],
	[Include support for pthreads.])
    wrap_list="${wrap_list} pthread_create pthread_exit"
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

#------------------------------------------------------------
# Option: --enable-start-main=TYPE
#------------------------------------------------------------

AC_MSG_CHECKING([type of __libc_start_main function])
AC_ARG_ENABLE([start-main],
    [AS_HELP_STRING([--enable-start-main=TYPE],
        [type of __libc_start_main function (ppc, x86)])],
    [],
    [enable_start_main=x86
     if test "x$host_cpu_ppc" = xyes ; then
	 enable_start_main=ppc
     fi])

case "$enable_start_main" in
    ppc )
	AC_DEFINE([MONITOR_START_MAIN_PPC], [1],
	    [Libc start main type is ppc.])
	;;
    x86 )
	;;
    * )
	AC_MSG_ERROR([invalid start main type (ppc or x86): $enable_start_main])
	;;
esac

AC_MSG_RESULT([$enable_start_main])

AC_OUTPUT

#------------------------------------------------------------
# Display a summary of the config options.
#------------------------------------------------------------

AC_MSG_NOTICE([-------------------])
AC_MSG_NOTICE([Libmonitor summary])
AC_MSG_NOTICE([-------------------])

if test "$warn_cflags_opt" = yes ; then
    AC_MSG_WARN([Modified -O flags in CFLAGS to support asm fence labels.])
    AC_MSG_WARN([Probably the compiler is duplicating code on ia64.])
fi

AC_MSG_NOTICE([svn revision: $svn_revision])
AC_MSG_NOTICE([prefix: $prefix])
AC_MSG_NOTICE([build: $build])
AC_MSG_NOTICE([host:  $host])
AC_MSG_NOTICE([CC CFLAGS: $CC $CFLAGS])
AC_MSG_NOTICE([enable debug: $enable_debug])
AC_MSG_NOTICE([enable link preload: $enable_link_preload])
AC_MSG_NOTICE([enable link static: $enable_link_static])
AC_MSG_NOTICE([enable dlfcn: $enable_dlfcn])
AC_MSG_NOTICE([enable fork: $enable_fork])
AC_MSG_NOTICE([enable mpi: $enable_mpi])
AC_MSG_NOTICE([enable pthreads: $enable_pthreads])
AC_MSG_NOTICE([enable signals: $enable_signals])
AC_MSG_NOTICE([start main type: $enable_start_main])
