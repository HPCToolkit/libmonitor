/*
 *  Libmonitor Fortran MPI override functions.
 *
 *  mpi_init --> mpi_init__
 *
 *  $Id$
 */

#define FORTRAN_NAME(name)  name ## __
#define FORTRAN_REAL_NAME(name)  __real_ ## name ## __

#include "mpi_fortran.h"
