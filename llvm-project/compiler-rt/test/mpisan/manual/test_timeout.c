#include <mpi.h>

// internal_usleep equivalent — use nanosleep via syscall
// Since we can't include <unistd.h> in the runtime, we use it in the test.
#include <unistd.h>

extern void __mpisan_init();
extern void __mpisan_isend(const void*, int, int, int, int, int,
                           unsigned long long);
extern void __mpisan_wait(unsigned long long);

#define MPISAN_TYPE_INT 0x4c000405
#define COMM_ID 0

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    __mpisan_init();

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int buf[10] = {0};

    if (rank == 0) {
        // Post a non-blocking send
        __mpisan_isend(buf, 10, MPISAN_TYPE_INT, 1, 0, COMM_ID, 42ULL);

        // Sleep 10ms — with deadlock_timeout_us=1000 (1ms), this exceeds it
        usleep(10000);

        // Now call wait — elapsed will be ~10ms > 1ms timeout
        __mpisan_wait(42ULL);

        MPI_Send(buf, 10, MPI_INT, 1, 0, MPI_COMM_WORLD);
    } else {
        MPI_Recv(buf, 10, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    MPI_Finalize();
    return 0;
}
