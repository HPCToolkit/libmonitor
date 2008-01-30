/*
 *  Libmonitor MPI C override functions.
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

typedef int mpi_init_fcn_t(int *, char ***);
typedef int mpi_finalize_fcn_t(void);
typedef int mpi_comm_fcn_t(MPI_Comm, int *);

#ifdef MONITOR_STATIC
extern mpi_init_fcn_t      __real_MPI_Init;
extern mpi_finalize_fcn_t  __real_MPI_Finalize;
extern mpi_comm_fcn_t      __real_MPI_Comm_size;
extern mpi_comm_fcn_t      __real_MPI_Comm_rank;
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
    MONITOR_GET_REAL_NAME_WRAP(real_mpi_finalize, MPI_Finalize);
    MONITOR_GET_REAL_NAME(real_mpi_comm_size, MPI_Comm_size);
    MONITOR_GET_REAL_NAME(real_mpi_comm_rank, MPI_Comm_rank);
}

/*
 *----------------------------------------------------------------------
 *  MPI OVERRIDE FUNCTIONS
 *----------------------------------------------------------------------
 */

/*
 *  Override MPI_Init() and MPI_Finalize() in C.
 */
int
MONITOR_WRAP_NAME(MPI_Init)(int *argc, char ***argv)
{
    int size = -1, rank = -1;
    int ret;

    MONITOR_DEBUG1("\n");
    monitor_mpi_init();
    ret = (*real_mpi_init)(argc, argv);
    if ((*real_mpi_comm_size)(MPI_COMM_WORLD, &size) == MPI_SUCCESS &&
	(*real_mpi_comm_rank)(MPI_COMM_WORLD, &rank) == MPI_SUCCESS) {
	monitor_set_mpi_size_rank(size, rank);
    }
    MONITOR_DEBUG("size = %d, rank = %d\n", size, rank);
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
