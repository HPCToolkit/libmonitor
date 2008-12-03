/*
 *  Override mpi_finalize_ in Fortran.
 *
 *  $Id$
 */

#include "config.h"
#include "common.h"
#include "monitor.h"

typedef void mpi_finalize_fcn_t(int *);
#ifdef MONITOR_STATIC
extern mpi_finalize_fcn_t  __real_mpi_finalize_;
#endif
static mpi_finalize_fcn_t  *real_mpi_finalize = NULL;

void
MONITOR_WRAP_NAME(mpi_finalize_)(int *ierror)
{
    MONITOR_GET_REAL_NAME_WRAP(real_mpi_finalize, mpi_finalize_);
    MONITOR_DEBUG("size = %d, rank = %d\n",
		  monitor_mpi_comm_size(), monitor_mpi_comm_rank());
    MONITOR_DEBUG1("calling monitor_fini_mpi() ...\n");
    monitor_fini_mpi();
    (*real_mpi_finalize)(ierror);
}
