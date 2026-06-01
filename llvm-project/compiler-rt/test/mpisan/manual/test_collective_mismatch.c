#include <stdio.h>
#include <mpi.h>

extern void __mpisan_init();
extern void __mpisan_collective_enter(int, int, int, int, int, int);
extern void __mpisan_collective_exit(int, int);

// Collective op constants from mpisan_internal.h
#define MPISAN_COLL_BARRIER   1
#define MPISAN_COLL_BCAST     2
#define COMM_ID 0

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    __mpisan_init();

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        // Simulate two ranks entering different collectives on the same
        // communicator within the same process — first entry sets BARRIER,
        // second entry with BCAST triggers the mismatch.
        __mpisan_collective_enter(COMM_ID, MPISAN_COLL_BARRIER, -1, -1, -1, 2);
        __mpisan_collective_enter(COMM_ID, MPISAN_COLL_BCAST,   -1, -1, -1, 2);
        __mpisan_collective_exit(COMM_ID, MPISAN_COLL_BARRIER);
    }

    // Real MPI barrier so both ranks finish cleanly
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Finalize();
    return 0;
}
