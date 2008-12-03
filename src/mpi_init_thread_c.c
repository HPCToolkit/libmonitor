/*
 *  Override MPI_Init_thread in C/C++.
 *
 *  $Id$
 */

#include "config.h"
#include "common.h"
#include "monitor.h"

typedef int mpi_init_thread_fcn_t(int *, char ***, int, int *);
#ifdef MONITOR_STATIC
extern mpi_init_thread_fcn_t  __real_MPI_Init_thread;
#endif
static mpi_init_thread_fcn_t  *real_mpi_init_thread = NULL;

int
MONITOR_WRAP_NAME(MPI_Init_thread)(int *argc, char ***argv,
				   int required, int *provided)
{
    int ret;

    MONITOR_DEBUG1("\n");
    MONITOR_GET_REAL_NAME_WRAP(real_mpi_init_thread, MPI_Init_thread);
    ret = (*real_mpi_init_thread)(argc, argv, required, provided);
    MONITOR_DEBUG1("calling monitor_init_mpi() ...\n");
    monitor_init_mpi(argc, argv);

    return (ret);
}
