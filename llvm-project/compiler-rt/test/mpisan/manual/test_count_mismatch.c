#include <stdio.h>
#include <mpi.h>

extern void __mpisan_init();
extern void __mpisan_send(const void*, int, int, int, int, int);
extern void __mpisan_recv(void*, int, int, int, int, int);

#define MPISAN_TYPE_INT 0x4c000405
#define COMM_ID 0

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    __mpisan_init();

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int sbuf[20] = {0};
    int rbuf[10] = {0};

    if (rank == 0) {
        // Send 20 ints, recv expects only 10 — count mismatch
        __mpisan_send(sbuf, 20, MPISAN_TYPE_INT, 1, 0, COMM_ID);
        __mpisan_recv(rbuf, 10, MPISAN_TYPE_INT, 1, 0, COMM_ID);

        MPI_Send(sbuf, 10, MPI_INT, 1, 0, MPI_COMM_WORLD);
    } else {
        MPI_Recv(rbuf, 10, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    MPI_Finalize();
    return 0;
}
