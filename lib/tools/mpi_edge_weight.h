#ifndef MPI_EDGE_WEIGHT_H
#define MPI_EDGE_WEIGHT_H

#include <mpi.h>
#include "definitions.h"

inline MPI_Datatype mpi_edge_weight_type() {
#if defined(EDGE_WEIGHT_DOUBLE)
        return MPI_DOUBLE;
#elif defined(MODE64BITEDGES)
        return MPI_LONG_LONG_INT;
#else
        return MPI_INT;
#endif
}

#endif
