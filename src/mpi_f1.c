/*
 *  Libmonitor Fortran MPI override functions.
 *
 *  mpi_init --> mpi_init_
 *
 *  $Id$
 */

#define FORTRAN_NAME(name)  name ## _
#define FORTRAN_REAL_NAME(name)  __real_ ## name ## _

#include "mpi_fortran.h"
