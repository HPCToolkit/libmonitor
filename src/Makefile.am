##
##  Libmonitor src/Makefile.am.
##
##  Copyright (c) 2007-2017, Rice University.
##  All rights reserved.
##
##  Redistribution and use in source and binary forms, with or without
##  modification, are permitted provided that the following conditions are
##  met:
##
##  * Redistributions of source code must retain the above copyright
##    notice, this list of conditions and the following disclaimer.
##
##  * Redistributions in binary form must reproduce the above copyright
##    notice, this list of conditions and the following disclaimer in the
##    documentation and/or other materials provided with the distribution.
##
##  * Neither the name of Rice University (RICE) nor the names of its
##    contributors may be used to endorse or promote products derived from
##    this software without specific prior written permission.
##
##  This software is provided by RICE and contributors "as is" and any
##  express or implied warranties, including, but not limited to, the
##  implied warranties of merchantability and fitness for a particular
##  purpose are disclaimed. In no event shall RICE or contributors be
##  liable for any direct, indirect, incidental, special, exemplary, or
##  consequential damages (including, but not limited to, procurement of
##  substitute goods or services; loss of use, data, or profits; or
##  business interruption) however caused and on any theory of liability,
##  whether in contract, strict liability, or tort (including negligence
##  or otherwise) arising in any way out of the use of this software, even
##  if advised of the possibility of such damage.
##
##  $Id$
##

# Automake does not allow per-file flags, only per-target (program or
# library).  So, we have to fake the fence cflags with a separate
# library and then add that to the real library.  See Section 27.8,
# "Per-Object Flags Emulation" of automake docs.  Blech.

MONITOR_MAIN_FILES = callback.c mpi.c utils.c
MONITOR_THREAD_FILES =

MONITOR_FENCE_FILES = main.c
MONITOR_THREAD_FENCE_FILES = pthread.c

MONITOR_SCRIPT_FILES = monitor-link monitor-run
MONITOR_MPI_FILES =  \
	mpi_init_c.c  mpi_init_thread_c.c  mpi_final_c.c  mpi_comm_c.c   \
	mpi_init_f0.c mpi_init_thread_f0.c mpi_final_f0.c mpi_comm_f0.c  \
	mpi_init_f1.c mpi_init_thread_f1.c mpi_final_f1.c mpi_comm_f1.c  \
	mpi_init_f2.c mpi_init_thread_f2.c mpi_final_f2.c mpi_comm_f2.c

include_HEADERS = monitor.h
bin_SCRIPTS =
CLEANFILES = $(MONITOR_SCRIPT_FILES)

MONITOR_CFLAGS = @MONITOR_CFLAGS@
FENCE_CFLAGS = @FENCE_CFLAGS@

#------------------------------------------------------------

if MONITOR_TEST_LINK_PRELOAD
    lib_LTLIBRARIES = libmonitor.la
    libmonitor_la_SOURCES = $(MONITOR_MAIN_FILES)
    libmonitor_la_CPPFLAGS = -DMONITOR_DYNAMIC
    libmonitor_la_CFLAGS = $(MONITOR_CFLAGS)
    libmonitor_la_LIBADD = libfence.la
    bin_SCRIPTS += monitor-run

    noinst_LTLIBRARIES = libfence.la
    libfence_la_SOURCES = $(MONITOR_FENCE_FILES)
    libfence_la_CPPFLAGS = -DMONITOR_DYNAMIC
    libfence_la_CFLAGS = $(FENCE_CFLAGS)

if MONITOR_TEST_USE_GNU_ATOMIC_OPS
    libmonitor_la_CPPFLAGS += -DUSE_GNU_ATOMIC_OPS
    libfence_la_CPPFLAGS += -DUSE_GNU_ATOMIC_OPS
endif

if MONITOR_TEST_USE_DLOPEN
    libmonitor_la_SOURCES += dlopen.c
endif

if MONITOR_TEST_USE_FORK
    libmonitor_la_SOURCES += fork.c
endif

if MONITOR_TEST_USE_PTHREADS
    libmonitor_la_SOURCES += $(MONITOR_THREAD_FILES)
    libfence_la_SOURCES += $(MONITOR_THREAD_FENCE_FILES)
endif

if MONITOR_TEST_USE_SIGNALS
    libmonitor_la_SOURCES += signal.c
endif

if MONITOR_TEST_USE_MPI
    libmonitor_la_SOURCES += $(MONITOR_MPI_FILES) pmpi.c
endif
endif

#------------------------------------------------------------

if MONITOR_TEST_LINK_STATIC
    lib_LIBRARIES = libmonitor_wrap.a
    libmonitor_wrap_a_SOURCES = $(MONITOR_MAIN_FILES)
    libmonitor_wrap_a_CPPFLAGS = -DMONITOR_STATIC
    libmonitor_wrap_a_CFLAGS = $(MONITOR_CFLAGS)
    libmonitor_wrap_a_LIBADD = libfence_wrap_a-main.o
    bin_SCRIPTS += monitor-link

    noinst_LIBRARIES = libfence_wrap.a
    libfence_wrap_a_SOURCES = $(MONITOR_FENCE_FILES)
    libfence_wrap_a_CPPFLAGS = -DMONITOR_STATIC
    libfence_wrap_a_CFLAGS = $(FENCE_CFLAGS)

if MONITOR_TEST_USE_GNU_ATOMIC_OPS
    libmonitor_wrap_a_CPPFLAGS += -DUSE_GNU_ATOMIC_OPS
    libfence_wrap_a_CPPFLAGS += -DUSE_GNU_ATOMIC_OPS
endif

if MONITOR_TEST_USE_DLOPEN
    libmonitor_wrap_a_SOURCES += dlopen.c
endif

if MONITOR_TEST_USE_FORK
    libmonitor_wrap_a_SOURCES += fork.c
endif

if MONITOR_TEST_USE_PTHREADS
    libmonitor_wrap_a_SOURCES += $(MONITOR_THREAD_FILES)
    libmonitor_wrap_a_LIBADD += libfence_wrap_a-pthread.o
    libfence_wrap_a_SOURCES += $(MONITOR_THREAD_FENCE_FILES)
endif

if MONITOR_TEST_USE_SIGNALS
    libmonitor_wrap_a_SOURCES += signal.c
endif

if MONITOR_TEST_USE_MPI
    libmonitor_wrap_a_SOURCES += $(MONITOR_MPI_FILES)
endif
endif
