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
 */

#include "config.h"
#ifdef MONITOR_DYNAMIC
#include <dlfcn.h>
#endif
#include <err.h>
#include <errno.h>
#include <mpi.h>
#include <stdio.h>

#include "common.h"
#include "monitor.h"

/*
 *----------------------------------------------------------------------
 *  GLOBAL VARIABLES
 *----------------------------------------------------------------------
 */

typedef int  mpi_init_fcn_t(int *, char ***);
typedef void mpi_finalize_fcn_t(int *);
typedef void mpi_comm_fcn_t(int *, int *, int *);

#ifdef MONITOR_STATIC
extern mpi_init_fcn_t      __real_MPI_Init;
extern mpi_finalize_fcn_t  FORTRAN_REAL_NAME(mpi_finalize);
extern mpi_comm_fcn_t      FORTRAN_NAME(mpi_comm_size);
extern mpi_comm_fcn_t      FORTRAN_NAME(mpi_comm_rank);
#endif

static mpi_init_fcn_t      *real_mpi_init = NULL;
static mpi_finalize_fcn_t  *real_mpi_finalize = NULL;
static mpi_comm_fcn_t      *real_mpi_comm_size = NULL;
static mpi_comm_fcn_t      *real_mpi_comm_rank = NULL;

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
    MONITOR_GET_REAL_NAME_WRAP(real_mpi_finalize, FORTRAN_NAME(mpi_finalize));
    MONITOR_GET_REAL_NAME(real_mpi_comm_size, FORTRAN_NAME(mpi_comm_size));
    MONITOR_GET_REAL_NAME(real_mpi_comm_rank, FORTRAN_NAME(mpi_comm_rank));
}

/*
 *----------------------------------------------------------------------
 *  MPI OVERRIDE FUNCTIONS
 *----------------------------------------------------------------------
 */

/*
 *  Override MPI_INIT() and MPI_FINALIZE() in Fortran.
 *
 *  FIXME: This uses the C MPI_Init but the Fortran mpi_finalize and
 *  mpi_comm_size and rank, which is what mpif90 does on ada.  Other
 *  systems will likely treat this differently.
 */
void
MONITOR_WRAP_NAME(FORTRAN_NAME(mpi_init))(int *ierror)
{
    int comm_world = MPI_COMM_WORLD;
    int size = -1, rank = -1;
    int argc, ret_size, ret_rank;
    char **argv;

    MONITOR_DEBUG1("\n");
    monitor_mpi_init();
    monitor_get_main_args(&argc, &argv, NULL);
    *ierror = (*real_mpi_init)(&argc, &argv);
    (*real_mpi_comm_size)(&comm_world, &size, &ret_size);
    (*real_mpi_comm_rank)(&comm_world, &rank, &ret_rank);
    if (ret_size == MPI_SUCCESS && ret_rank == MPI_SUCCESS)
	monitor_set_mpi_size_rank(size, rank);
    MONITOR_DEBUG("size = %d, rank = %d\n", size, rank);
    MONITOR_DEBUG1("calling monitor_init_mpi() ...\n");
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
