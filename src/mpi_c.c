/*
 *  Libmonitor MPI C/C++ override functions.
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
 *    MPI_Init
 *    MPI_Init_thread
 *    MPI_Finalize
 *    MPI_Comm_rank
 *
 *  Note: we want a single libmonitor to work with multiple MPI
 *  libraries, but each library defines its own MPI_Comm and
 *  MPI_COMM_WORLD.  So, we avoid including <mpi.h>, we assume a
 *  uniform type for MPI_Comm (void *), and we wait for the
 *  application to call MPI_Comm_rank() and use its comm value.
 */

#include "config.h"
#ifdef MONITOR_DYNAMIC
#include <dlfcn.h>
#endif
#include <err.h>
#include <errno.h>
#include <stdio.h>

#include "common.h"
#include "monitor.h"

/*
 *----------------------------------------------------------------------
 *  GLOBAL VARIABLES
 *----------------------------------------------------------------------
 */

typedef int mpi_init_fcn_t(int *, char ***);
typedef int mpi_init_thread_fcn_t(int *, char ***, int, int *);
typedef int mpi_finalize_fcn_t(void);
typedef int mpi_comm_fcn_t(void *, int *);

#ifdef MONITOR_STATIC
extern mpi_init_fcn_t      __real_MPI_Init;
extern mpi_init_thread_fcn_t  __real_MPI_Init_thread;
extern mpi_finalize_fcn_t  __real_MPI_Finalize;
extern mpi_comm_fcn_t      __real_MPI_Comm_rank;
extern mpi_comm_fcn_t      MPI_Comm_size;
#endif

static mpi_init_fcn_t      *real_mpi_init = NULL;
static mpi_init_thread_fcn_t  *real_mpi_init_thread = NULL;
static mpi_finalize_fcn_t  *real_mpi_finalize = NULL;
static mpi_comm_fcn_t      *real_mpi_comm_rank = NULL;
static mpi_comm_fcn_t      *real_mpi_comm_size = NULL;

/*
 *----------------------------------------------------------------------
 *  INTERNAL HELPER FUNCTIONS
 *----------------------------------------------------------------------
 */

static void
monitor_mpi_init(void)
{
    MONITOR_RUN_ONCE(mpi_init);
    monitor_early_init();
    MONITOR_GET_REAL_NAME_WRAP(real_mpi_init, MPI_Init);
    MONITOR_GET_REAL_NAME_WRAP(real_mpi_init_thread, MPI_Init_thread);
    MONITOR_GET_REAL_NAME_WRAP(real_mpi_finalize, MPI_Finalize);
    MONITOR_GET_REAL_NAME_WRAP(real_mpi_comm_rank, MPI_Comm_rank);
    MONITOR_GET_REAL_NAME(real_mpi_comm_size, MPI_Comm_size);
}

/*
 *----------------------------------------------------------------------
 *  MPI OVERRIDE FUNCTIONS
 *----------------------------------------------------------------------
 */

int
MONITOR_WRAP_NAME(MPI_Init)(int *argc, char ***argv)
{
    int ret;

    MONITOR_DEBUG1("\n");
    monitor_mpi_init();
    ret = (*real_mpi_init)(argc, argv);
    MONITOR_DEBUG1("calling monitor_init_mpi() ...\n");
    monitor_init_mpi(argc, argv);

    return (ret);
}

int
MONITOR_WRAP_NAME(MPI_Init_thread)(int *argc, char ***argv,
				   int required, int *provided)
{
    int ret;

    MONITOR_DEBUG1("\n");
    monitor_mpi_init();
    ret = (*real_mpi_init_thread)(argc, argv, required, provided);
    MONITOR_DEBUG1("calling monitor_init_mpi() ...\n");
    monitor_init_mpi(argc, argv);

    return (ret);
}

int
MONITOR_WRAP_NAME(MPI_Finalize)(void)
{
    monitor_mpi_init();
    MONITOR_DEBUG("size = %d, rank = %d\n",
		  monitor_mpi_comm_size(), monitor_mpi_comm_rank());
    MONITOR_DEBUG1("calling monitor_fini_mpi() ...\n");
    monitor_fini_mpi();

    return (*real_mpi_finalize)();
}

int
MONITOR_WRAP_NAME(MPI_Comm_rank)(void *comm, int *rank)
{
    int size = -1, ret;

    monitor_mpi_init();
    ret = (*real_mpi_comm_size)(comm, &size);
    ret = (*real_mpi_comm_rank)(comm, rank);
    MONITOR_DEBUG("setting size = %d, rank = %d\n", size, *rank);
    monitor_set_mpi_size_rank(size, *rank);

    return (ret);
}
