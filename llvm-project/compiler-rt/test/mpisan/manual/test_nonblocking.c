#include <stdio.h>
#include <mpi.h>

extern void __mpisan_init();
extern void __mpisan_isend(const void*, int, int, int, int, int,
                           unsigned long long);
extern void __mpisan_irecv(void*, int, int, int, int, int,
                           unsigned long long);
extern void __mpisan_wait(unsigned long long);

#define MPISAN_TYPE_INT    0x4c000405
#define MPISAN_TYPE_DOUBLE 0x4c00080b
#define COMM_ID 0

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    __mpisan_init();

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int    ibuf[10] = {0};
    double dbuf[10] = {0.0};

    if (rank == 0) {
        // On rank 0: post an isend of INT with request_id=1,
        // then post an irecv of DOUBLE with the same request_id=1.
        // When wait is called, the irecv matches the isend and
        // detects the INT vs DOUBLE mismatch.
        __mpisan_isend(ibuf, 10, MPISAN_TYPE_INT,    1, 0, COMM_ID, 1ULL);
        __mpisan_irecv(dbuf, 10, MPISAN_TYPE_DOUBLE, 1, 0, COMM_ID, 2ULL);

        // Wait on the irecv — this triggers matching against the isend
        __mpisan_wait(2ULL);

        MPI_Send(ibuf, 10, MPI_INT, 1, 0, MPI_COMM_WORLD);
    } else {
        MPI_Recv(dbuf, 10, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    MPI_Finalize();
    return 0;
}
