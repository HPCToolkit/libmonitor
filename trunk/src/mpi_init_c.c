/*
 *  Override MPI_Init in C/C++.
 *
 *  $Id$
 */

#include "config.h"
#include "common.h"
#include "monitor.h"

typedef int mpi_init_fcn_t(int *, char ***);
#ifdef MONITOR_STATIC
extern mpi_init_fcn_t  __real_MPI_Init;
#endif
static mpi_init_fcn_t  *real_mpi_init = NULL;

int
MONITOR_WRAP_NAME(MPI_Init)(int *argc, char ***argv)
{
    int ret;

    MONITOR_DEBUG1("\n");
    MONITOR_GET_REAL_NAME_WRAP(real_mpi_init, MPI_Init);
    ret = (*real_mpi_init)(argc, argv);
    MONITOR_DEBUG1("calling monitor_init_mpi() ...\n");
    monitor_init_mpi(argc, argv);

    return (ret);
}
