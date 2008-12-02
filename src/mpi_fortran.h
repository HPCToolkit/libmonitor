/*
 *  Libmonitor MPI Fortran override functions.
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
 *    mpi_init_
 *    mpi_init_thread_
 *    mpi_finalize_
 *    mpi_comm_rank_
 *
 *  Note: we want a single libmonitor to work with multiple MPI
 *  libraries.  This is a bit easier in Fortran than in C because
 *  MPI_Comm will always be int.  But, MPI_COMM_WORLD can still vary
 *  by implementation, so we wait for the application to call
 *  MPI_Comm_rank() and use its comm value.
 *
 *  Note: this file needs to be .h and not .c, or else Automake would
 *  try to compile it separately.
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

typedef void mpi_init_fcn_t(int *);
typedef void mpi_init_thread_fcn_t(int *, int *, int *);
typedef void mpi_finalize_fcn_t(int *);
typedef void mpi_comm_fcn_t(int *, int *, int *);

#ifdef MONITOR_STATIC
extern mpi_init_fcn_t      FORTRAN_REAL_NAME(mpi_init);
extern mpi_init_thread_fcn_t  FORTRAN_REAL_NAME(mpi_init_thread);
extern mpi_finalize_fcn_t  FORTRAN_REAL_NAME(mpi_finalize);
extern mpi_comm_fcn_t      FORTRAN_REAL_NAME(mpi_comm_rank);
extern mpi_comm_fcn_t      FORTRAN_NAME(mpi_comm_size);
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
    MONITOR_GET_REAL_NAME_WRAP(real_mpi_init, FORTRAN_NAME(mpi_init));
    MONITOR_GET_REAL_NAME_WRAP(real_mpi_init_thread, FORTRAN_NAME(mpi_init_thread));
    MONITOR_GET_REAL_NAME_WRAP(real_mpi_finalize, FORTRAN_NAME(mpi_finalize));
    MONITOR_GET_REAL_NAME_WRAP(real_mpi_comm_rank, FORTRAN_NAME(mpi_comm_rank));
    MONITOR_GET_REAL_NAME(real_mpi_comm_size, FORTRAN_NAME(mpi_comm_size));
}

/*
 *----------------------------------------------------------------------
 *  MPI OVERRIDE FUNCTIONS
 *----------------------------------------------------------------------
 */

void
MONITOR_WRAP_NAME(FORTRAN_NAME(mpi_init))(int *ierror)
{
    int argc;
    char **argv;

    MONITOR_DEBUG1("\n");
    monitor_mpi_init();
    (*real_mpi_init)(ierror);
    MONITOR_DEBUG1("calling monitor_init_mpi() ...\n");
    monitor_get_main_args(&argc, &argv, NULL);
    monitor_init_mpi(&argc, &argv);
}

void
MONITOR_WRAP_NAME(FORTRAN_NAME(mpi_init_thread))
		 (int *required, int *provided, int *ierror)
{
    int argc;
    char **argv;

    MONITOR_DEBUG1("\n");
    monitor_mpi_init();
    (*real_mpi_init_thread)(required, provided, ierror);
    MONITOR_DEBUG1("calling monitor_init_mpi() ...\n");
    monitor_get_main_args(&argc, &argv, NULL);
    monitor_init_mpi(&argc, &argv);
}

void
MONITOR_WRAP_NAME(FORTRAN_NAME(mpi_finalize))(int *ierror)
{
    monitor_mpi_init();
    MONITOR_DEBUG("size = %d, rank = %d\n",
		  monitor_mpi_comm_size(), monitor_mpi_comm_rank());
    MONITOR_DEBUG1("calling monitor_fini_mpi() ...\n");
    monitor_fini_mpi();
    (*real_mpi_finalize)(ierror);
}

void
MONITOR_WRAP_NAME(FORTRAN_NAME(mpi_comm_rank))
		 (int *comm, int *rank, int *ierror)
{
    int size = -1, ret;

    monitor_mpi_init();
    (*real_mpi_comm_size)(comm, &size, &ret);
    (*real_mpi_comm_rank)(comm, rank, ierror);
    MONITOR_DEBUG("setting size = %d, rank = %d\n", size, *rank);
    monitor_set_mpi_size_rank(size, *rank);
}
