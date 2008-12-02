/*
 *  Libmonitor common MPI functions.
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
#include "common.h"
#include "monitor.h"

/*
 *----------------------------------------------------------------------
 *  GLOBAL VARIABLES
 *----------------------------------------------------------------------
 */

static int mpi_size = -1;
static int mpi_rank = -1;

/*
 *----------------------------------------------------------------------
 *  MPI SUPPORT FUNCTIONS
 *----------------------------------------------------------------------
 */

/*
 *  Returns: The stored MPI_COMM_WORLD size and rank, or else -1 if
 *  they have not been set.
 *  
 */
int
monitor_mpi_comm_size(void)
{
    return (mpi_size);
}

int
monitor_mpi_comm_rank(void)
{
    return (mpi_rank);
}

/*
 *  Set in the C or Fortran MPI_Comm_rank() override function.
 *
 *  Note: we depend on the application using MPI_COMM_WORLD before any
 *  other communicator, so we only want to set size/rank on the first
 *  use of MPI_Comm_rank().
 */
void
monitor_set_mpi_size_rank(int size, int rank)
{
    static int first = 1;

    if (first) {
	mpi_size = size;
	mpi_rank = rank;
	first = 0;
    }
}
