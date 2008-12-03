/*
 *  Override MPI_Finalize in C/C++.
 *
 *  $Id$
 */

#include "config.h"
#include "common.h"
#include "monitor.h"

typedef int mpi_finalize_fcn_t(void);
#ifdef MONITOR_STATIC
extern mpi_finalize_fcn_t  __real_MPI_Finalize;
#endif
static mpi_finalize_fcn_t  *real_mpi_finalize = NULL;

int
MONITOR_WRAP_NAME(MPI_Finalize)(void)
{
    MONITOR_GET_REAL_NAME_WRAP(real_mpi_finalize, MPI_Finalize);
    MONITOR_DEBUG("size = %d, rank = %d\n",
		  monitor_mpi_comm_size(), monitor_mpi_comm_rank());
    MONITOR_DEBUG1("calling monitor_fini_mpi() ...\n");
    monitor_fini_mpi();

    return (*real_mpi_finalize)();
}
