#include <stdio.h>
#include <mpi.h>

extern void __mpisan_init();
extern void __mpisan_send(const void*, int, int, int, int, int);
extern void __mpisan_recv(void*, int, int, int, int, int);

// Must match MPISAN_TYPE_* in mpisan_internal.h
#define MPISAN_TYPE_INT    0x4c000405
#define MPISAN_TYPE_DOUBLE 0x4c00080b

// Use a fixed communicator ID — the runtime just needs consistent values
// for matching sends to recvs within the same process.
#define COMM_WORLD_ID 0

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    __mpisan_init();

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int    ibuf[10] = {0};
    double dbuf[10] = {0};

    if (rank == 0) {
        // Simulate rank 0 sending INT to rank 1, then immediately
        // "receiving" with DOUBLE type — both hooks on the same rank
        // so the pending ops table is shared and the mismatch is caught.
        __mpisan_send(ibuf, 10, MPISAN_TYPE_INT,    1, 0, COMM_WORLD_ID);
        __mpisan_recv(dbuf, 10, MPISAN_TYPE_DOUBLE, 1, 0, COMM_WORLD_ID);

        // Do the actual MPI communication
        MPI_Send(ibuf, 10, MPI_INT, 1, 0, MPI_COMM_WORLD);
    } else {
        MPI_Recv(dbuf, 10, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    MPI_Finalize();
    return 0;
}
