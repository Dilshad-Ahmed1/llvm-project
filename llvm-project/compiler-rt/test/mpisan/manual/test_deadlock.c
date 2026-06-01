#include <stdio.h>
#include <mpi.h>

extern void __mpisan_init();
extern void __mpisan_set_wait_for(int waiter, int provider);
extern void __mpisan_clear_wait_for(int waiter);

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    __mpisan_init();

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        // Build a deadlock cycle in the wait-for graph:
        //   rank 0 is waiting for rank 1
        //   rank 1 is waiting for rank 0
        // This creates cycle: 0 -> 1 -> 0
        //
        // __mpisan_set_wait_for(waiter, provider) sets the edge and
        // calls CheckForDeadlock(waiter) which walks the graph.
        // Setting edge 0->1 first, then 1->0 closes the cycle.

        __mpisan_set_wait_for(0, 1);  // rank 0 waits for rank 1
        __mpisan_set_wait_for(1, 0);  // rank 1 waits for rank 0 — cycle!

        // Clean up so MPI can finish
        __mpisan_clear_wait_for(0);
        __mpisan_clear_wait_for(1);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}
