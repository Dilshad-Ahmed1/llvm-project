/**
 * @file broadcast.c
 * @brief Basic collective operation example using MPI_Bcast
 * 
 * This example demonstrates collective communication and shows how the
 * MPI Usage Sanitizer instruments collective operations for consistency
 * and correctness validation.
 * 
 * Expected instrumentation:
 * - Collective operation synchronization validation
 * - Buffer and parameter consistency across all processes
 * - Root process validation
 * - Communicator consistency checking
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 256
#define ROOT_PROCESS 0

int main(int argc, char *argv[]) {
    int rank, size;
    char broadcast_buffer[BUFFER_SIZE];
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (rank == ROOT_PROCESS) {
        // Root process initializes the data to broadcast
        snprintf(broadcast_buffer, BUFFER_SIZE, 
                "Broadcast message from root process %d to %d processes", 
                rank, size);
        printf("Root process %d broadcasting: '%s'\n", rank, broadcast_buffer);
    } else {
        // Non-root processes initialize buffer to detect successful broadcast
        memset(broadcast_buffer, 0, BUFFER_SIZE);
        printf("Process %d waiting for broadcast...\n", rank);
    }
    
    // Broadcast operation - all processes must call this collectively
    // MPI_Bcast will be instrumented to validate:
    // - All processes call with same parameters (count, datatype, root, comm)
    // - Buffer is valid and accessible on all processes
    // - Root process is valid and consistent across all processes
    // - Communicator is the same on all processes
    // - No processes are missing from the collective operation
    
    double start_time = MPI_Wtime();  // Performance monitoring
    
    int bcast_result = MPI_Bcast(broadcast_buffer,      // buffer
                                BUFFER_SIZE,            // count
                                MPI_CHAR,               // datatype
                                ROOT_PROCESS,           // root
                                MPI_COMM_WORLD);        // communicator
    
    double end_time = MPI_Wtime();
    double bcast_time = end_time - start_time;
    
    if (bcast_result != MPI_SUCCESS) {
        fprintf(stderr, "Process %d: MPI_Bcast failed with error code %d\n", 
                rank, bcast_result);
        MPI_Abort(MPI_COMM_WORLD, bcast_result);
    }
    
    // Verify that all processes received the broadcast data
    printf("Process %d received broadcast: '%s'\n", rank, broadcast_buffer);
    printf("Process %d: Broadcast completed in %f seconds\n", rank, bcast_time);
    
    // Demonstrate multiple broadcasts with different data types
    int int_data = 42;
    double double_data = 3.14159;
    
    if (rank == ROOT_PROCESS) {
        printf("\nRoot process broadcasting integer: %d\n", int_data);
    }
    
    // Broadcast integer data
    MPI_Bcast(&int_data, 1, MPI_INT, ROOT_PROCESS, MPI_COMM_WORLD);
    printf("Process %d received integer: %d\n", rank, int_data);
    
    if (rank == ROOT_PROCESS) {
        printf("Root process broadcasting double: %f\n", double_data);
    }
    
    // Broadcast double data
    MPI_Bcast(&double_data, 1, MPI_DOUBLE, ROOT_PROCESS, MPI_COMM_WORLD);
    printf("Process %d received double: %f\n", rank, double_data);
    
    // Synchronize all processes before measuring collective performance
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Performance test: Multiple small broadcasts
    if (rank == ROOT_PROCESS) {
        printf("\nPerforming broadcast performance test...\n");
    }
    
    start_time = MPI_Wtime();
    
    for (int i = 0; i < 100; i++) {
        int test_data = i;
        MPI_Bcast(&test_data, 1, MPI_INT, ROOT_PROCESS, MPI_COMM_WORLD);
    }
    
    end_time = MPI_Wtime();
    double total_time = end_time - start_time;
    
    if (rank == ROOT_PROCESS) {
        printf("100 broadcasts completed in %f seconds\n", total_time);
        printf("Average time per broadcast: %f microseconds\n", 
               (total_time * 1000000.0) / 100.0);
    }
    
    MPI_Finalize();
    return EXIT_SUCCESS;
}

/*
 * Expected Sanitizer Output (All Processes):
 * 
 * [MPI Sanitizer] Pre-call: MPI_Bcast at broadcast.c:47
 * [MPI Sanitizer]   - Collective operation detected
 * [MPI Sanitizer]   - Buffer validation: 0x7fff5fbff8a0, size 256 bytes
 * [MPI Sanitizer]   - Count: 256, Datatype: MPI_CHAR (1 byte)
 * [MPI Sanitizer]   - Root process: 0 (valid in communicator)
 * [MPI Sanitizer]   - Communicator: MPI_COMM_WORLD (4 processes)
 * [MPI Sanitizer]   - Checking collective consistency...
 * [MPI Sanitizer] Post-call: MPI_Bcast returned MPI_SUCCESS
 * [MPI Sanitizer]   - Collective operation completed successfully
 * [MPI Sanitizer]   - Data consistency verified across all processes
 * [MPI Sanitizer]   - Performance: 0.000123 seconds
 * 
 * [MPI Sanitizer] Pre-call: MPI_Bcast at broadcast.c:72 (integer)
 * [MPI Sanitizer]   - Collective operation detected
 * [MPI Sanitizer]   - Buffer validation: 0x7fff5fbff79c, size 4 bytes
 * [MPI Sanitizer]   - Count: 1, Datatype: MPI_INT (4 bytes)
 * [MPI Sanitizer]   - Root process: 0 (consistent with previous calls)
 * [MPI Sanitizer] Post-call: MPI_Bcast returned MPI_SUCCESS
 * 
 * [MPI Sanitizer] Pre-call: MPI_Bcast at broadcast.c:80 (double)
 * [MPI Sanitizer]   - Collective operation detected
 * [MPI Sanitizer]   - Buffer validation: 0x7fff5fbff798, size 8 bytes
 * [MPI Sanitizer]   - Count: 1, Datatype: MPI_DOUBLE (8 bytes)
 * [MPI Sanitizer] Post-call: MPI_Bcast returned MPI_SUCCESS
 * 
 * [MPI Sanitizer] Performance Summary for Process 0:
 * [MPI Sanitizer]   - Total MPI_Bcast calls: 102
 * [MPI Sanitizer]   - Total collective time: 0.012345 seconds
 * [MPI Sanitizer]   - Average latency: 121.0 microseconds
 * [MPI Sanitizer]   - Data volume broadcast: 1.3 KB
 */