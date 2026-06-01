/**
 * @file gather_scatter.c
 * @brief Examples of MPI gather and scatter collective operations
 * 
 * This example demonstrates MPI_Gather, MPI_Scatter, MPI_Allgather, and
 * MPI_Alltoall operations, showing how the sanitizer validates collective
 * data distribution patterns.
 * 
 * Expected instrumentation:
 * - Root process buffer validation for gather operations
 * - Data distribution consistency checking
 * - Buffer size calculations and validation
 * - Collective synchronization verification
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_SIZE 10
#define ROOT_PROCESS 0

void print_array(const char *label, int *array, int size, int rank) {
    printf("Process %d %s: [", rank, label);
    for (int i = 0; i < size; i++) {
        printf("%d", array[i]);
        if (i < size - 1) printf(", ");
    }
    printf("]\n");
}

int main(int argc, char *argv[]) {
    int rank, size;
    int *local_data, *gathered_data, *scattered_data, *allgather_data;
    int *send_counts, *recv_counts, *send_displs, *recv_displs;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Allocate local data
    local_data = (int*)malloc(DATA_SIZE * sizeof(int));
    scattered_data = (int*)malloc(DATA_SIZE * sizeof(int));
    allgather_data = (int*)malloc(DATA_SIZE * size * sizeof(int));
    
    // Initialize local data with rank-specific values
    for (int i = 0; i < DATA_SIZE; i++) {
        local_data[i] = rank * DATA_SIZE + i;
    }
    
    print_array("local data", local_data, DATA_SIZE, rank);
    
    if (rank == 0) {
        printf("\n=== MPI Gather/Scatter Examples ===\n");
        printf("Number of processes: %d\n", size);
        printf("Data size per process: %d integers\n\n", DATA_SIZE);
    }
    
    // Example 1: MPI_Gather - collect all data at root
    if (rank == ROOT_PROCESS) {
        gathered_data = (int*)malloc(DATA_SIZE * size * sizeof(int));
        printf("1. Gathering data from all processes to root...\n");
    }
    
    // MPI_Gather will be instrumented to validate:
    // - Send buffer is valid on all processes
    // - Receive buffer is valid and large enough on root process
    // - Send count is consistent across all processes
    // - Receive count matches send count * number of processes
    // - Datatype consistency across all processes
    // - Root process is valid and consistent
    
    double start_time = MPI_Wtime();
    
    int gather_result = MPI_Gather(local_data,              // sendbuf
                                  DATA_SIZE,                // sendcount
                                  MPI_INT,                  // sendtype
                                  gathered_data,            // recvbuf (only valid on root)
                                  DATA_SIZE,                // recvcount
                                  MPI_INT,                  // recvtype
                                  ROOT_PROCESS,             // root
                                  MPI_COMM_WORLD);          // communicator
    
    double gather_time = MPI_Wtime() - start_time;
    
    if (gather_result != MPI_SUCCESS) {
        fprintf(stderr, "Process %d: MPI_Gather failed with code %d\n", 
                rank, gather_result);
        MPI_Abort(MPI_COMM_WORLD, gather_result);
    }
    
    if (rank == ROOT_PROCESS) {
        printf("Gather completed in %f seconds\n", gather_time);
        print_array("gathered data", gathered_data, DATA_SIZE * size, rank);
        printf("\n");
    }
    
    // Example 2: MPI_Scatter - distribute data from root
    int *scatter_source = NULL;
    if (rank == ROOT_PROCESS) {
        scatter_source = (int*)malloc(DATA_SIZE * size * sizeof(int));
        // Initialize scatter data
        for (int i = 0; i < DATA_SIZE * size; i++) {
            scatter_source[i] = 1000 + i;
        }
        printf("2. Scattering data from root to all processes...\n");
        print_array("scatter source", scatter_source, DATA_SIZE * size, rank);
    }
    
    // MPI_Scatter will be instrumented to validate:
    // - Send buffer is valid and large enough on root process
    // - Receive buffer is valid on all processes
    // - Send count * number of processes matches total data
    // - Receive count is consistent across all processes
    // - Data distribution is correct
    
    start_time = MPI_Wtime();
    
    int scatter_result = MPI_Scatter(scatter_source,        // sendbuf (only valid on root)
                                    DATA_SIZE,              // sendcount
                                    MPI_INT,                // sendtype
                                    scattered_data,         // recvbuf
                                    DATA_SIZE,              // recvcount
                                    MPI_INT,                // recvtype
                                    ROOT_PROCESS,           // root
                                    MPI_COMM_WORLD);        // communicator
    
    double scatter_time = MPI_Wtime() - start_time;
    
    if (scatter_result == MPI_SUCCESS) {
        print_array("scattered data", scattered_data, DATA_SIZE, rank);
        printf("Process %d: Scatter completed in %f seconds\n", rank, scatter_time);
    }
    
    // Example 3: MPI_Allgather - gather data to all processes
    if (rank == 0) {
        printf("\n3. All-gathering data to all processes...\n");
    }
    
    start_time = MPI_Wtime();
    
    int allgather_result = MPI_Allgather(local_data,           // sendbuf
                                        DATA_SIZE,             // sendcount
                                        MPI_INT,               // sendtype
                                        allgather_data,        // recvbuf
                                        DATA_SIZE,             // recvcount
                                        MPI_INT,               // recvtype
                                        MPI_COMM_WORLD);       // communicator
    
    double allgather_time = MPI_Wtime() - start_time;
    
    if (allgather_result == MPI_SUCCESS) {
        print_array("allgather result", allgather_data, DATA_SIZE * size, rank);
        printf("Process %d: Allgather completed in %f seconds\n", rank, allgather_time);
    }
    
    // Example 4: MPI_Alltoall - all-to-all exchange
    int *alltoall_send = (int*)malloc(size * sizeof(int));
    int *alltoall_recv = (int*)malloc(size * sizeof(int));
    
    // Initialize send data - each process sends its rank to all others
    for (int i = 0; i < size; i++) {
        alltoall_send[i] = rank * 100 + i;
    }
    
    if (rank == 0) {
        printf("\n4. All-to-all data exchange...\n");
    }
    
    print_array("alltoall send", alltoall_send, size, rank);
    
    start_time = MPI_Wtime();
    
    int alltoall_result = MPI_Alltoall(alltoall_send,          // sendbuf
                                      1,                       // sendcount
                                      MPI_INT,                 // sendtype
                                      alltoall_recv,           // recvbuf
                                      1,                       // recvcount
                                      MPI_INT,                 // recvtype
                                      MPI_COMM_WORLD);         // communicator
    
    double alltoall_time = MPI_Wtime() - start_time;
    
    if (alltoall_result == MPI_SUCCESS) {
        print_array("alltoall recv", alltoall_recv, size, rank);
        printf("Process %d: Alltoall completed in %f seconds\n", rank, alltoall_time);
    }
    
    // Example 5: Variable-sized gather (MPI_Gatherv)
    send_counts = (int*)malloc(size * sizeof(int));
    recv_counts = (int*)malloc(size * sizeof(int));
    send_displs = (int*)malloc(size * sizeof(int));
    recv_displs = (int*)malloc(size * sizeof(int));
    
    // Each process sends a different amount of data
    int my_send_count = rank + 1;
    int *variable_send_data = (int*)malloc(my_send_count * sizeof(int));
    
    for (int i = 0; i < my_send_count; i++) {
        variable_send_data[i] = rank * 1000 + i;
    }
    
    // Gather the send counts from all processes
    MPI_Allgather(&my_send_count, 1, MPI_INT, recv_counts, 1, MPI_INT, MPI_COMM_WORLD);
    
    int total_recv_count = 0;
    for (int i = 0; i < size; i++) {
        recv_displs[i] = total_recv_count;
        total_recv_count += recv_counts[i];
    }
    
    int *variable_recv_data = NULL;
    if (rank == ROOT_PROCESS) {
        variable_recv_data = (int*)malloc(total_recv_count * sizeof(int));
        printf("\n5. Variable-sized gather (Gatherv)...\n");
        printf("Total data to receive: %d integers\n", total_recv_count);
    }
    
    start_time = MPI_Wtime();
    
    int gatherv_result = MPI_Gatherv(variable_send_data,       // sendbuf
                                    my_send_count,             // sendcount
                                    MPI_INT,                   // sendtype
                                    variable_recv_data,        // recvbuf
                                    recv_counts,               // recvcounts
                                    recv_displs,               // displs
                                    MPI_INT,                   // recvtype
                                    ROOT_PROCESS,              // root
                                    MPI_COMM_WORLD);           // communicator
    
    double gatherv_time = MPI_Wtime() - start_time;
    
    if (gatherv_result == MPI_SUCCESS) {
        printf("Process %d sent %d integers\n", rank, my_send_count);
        if (rank == ROOT_PROCESS) {
            print_array("gatherv result", variable_recv_data, total_recv_count, rank);
            printf("Gatherv completed in %f seconds\n", gatherv_time);
        }
    }
    
    // Performance summary
    if (rank == ROOT_PROCESS) {
        printf("\n=== Performance Summary ===\n");
        printf("Gather time:    %f seconds\n", gather_time);
        printf("Scatter time:   %f seconds\n", scatter_time);
        printf("Allgather time: %f seconds\n", allgather_time);
        printf("Alltoall time:  %f seconds\n", alltoall_time);
        printf("Gatherv time:   %f seconds\n", gatherv_time);
    }
    
    // Cleanup
    free(local_data);
    free(scattered_data);
    free(allgather_data);
    free(alltoall_send);
    free(alltoall_recv);
    free(variable_send_data);
    free(send_counts);
    free(recv_counts);
    free(send_displs);
    free(recv_displs);
    
    if (rank == ROOT_PROCESS) {
        free(gathered_data);
        free(scatter_source);
        free(variable_recv_data);
    }
    
    MPI_Finalize();
    return EXIT_SUCCESS;
}

/*
 * Expected Sanitizer Output (Process 0 - Root):
 * 
 * [MPI Sanitizer] Pre-call: MPI_Gather at gather_scatter.c:58
 * [MPI Sanitizer]   - Collective operation: MPI_Gather detected
 * [MPI Sanitizer]   - Send buffer: 0x7f8b4c000b20, size 40 bytes
 * [MPI Sanitizer]   - Recv buffer (root): 0x7f8b4c001b20, size 160 bytes
 * [MPI Sanitizer]   - Send count: 10, Recv count: 10 per process
 * [MPI Sanitizer]   - Total expected data: 40 integers (160 bytes)
 * [MPI Sanitizer]   - Root process: 0 (this process)
 * [MPI Sanitizer]   - Datatype: MPI_INT (4 bytes)
 * [MPI Sanitizer] Post-call: MPI_Gather returned MPI_SUCCESS
 * [MPI Sanitizer]   - Data gathered from 4 processes successfully
 * 
 * [MPI Sanitizer] Pre-call: MPI_Scatter at gather_scatter.c:85
 * [MPI Sanitizer]   - Collective operation: MPI_Scatter detected
 * [MPI Sanitizer]   - Send buffer (root): 0x7f8b4c002b20, size 160 bytes
 * [MPI Sanitizer]   - Recv buffer: 0x7f8b4c000c20, size 40 bytes
 * [MPI Sanitizer]   - Send count: 10 per process, Recv count: 10
 * [MPI Sanitizer]   - Data distribution validation: PASSED
 * [MPI Sanitizer] Post-call: MPI_Scatter returned MPI_SUCCESS
 * 
 * [MPI Sanitizer] Pre-call: MPI_Allgather at gather_scatter.c:103
 * [MPI Sanitizer]   - Collective operation: MPI_Allgather detected
 * [MPI Sanitizer]   - Send buffer: 0x7f8b4c000b20, size 40 bytes
 * [MPI Sanitizer]   - Recv buffer: 0x7f8b4c003b20, size 160 bytes
 * [MPI Sanitizer]   - All processes will receive complete dataset
 * [MPI Sanitizer] Post-call: MPI_Allgather returned MPI_SUCCESS
 * 
 * [MPI Sanitizer] Pre-call: MPI_Alltoall at gather_scatter.c:119
 * [MPI Sanitizer]   - Collective operation: MPI_Alltoall detected
 * [MPI Sanitizer]   - Send buffer: 0x7f8b4c004b20, size 16 bytes
 * [MPI Sanitizer]   - Recv buffer: 0x7f8b4c005b20, size 16 bytes
 * [MPI Sanitizer]   - Exchange pattern: each process sends 1 int to each other
 * [MPI Sanitizer] Post-call: MPI_Alltoall returned MPI_SUCCESS
 * 
 * [MPI Sanitizer] Pre-call: MPI_Gatherv at gather_scatter.c:155
 * [MPI Sanitizer]   - Collective operation: MPI_Gatherv detected
 * [MPI Sanitizer]   - Variable-sized gather operation
 * [MPI Sanitizer]   - Send count: 1, Recv counts: [1, 2, 3, 4]
 * [MPI Sanitizer]   - Displacement array: [0, 1, 3, 6]
 * [MPI Sanitizer]   - Total receive size: 10 integers (40 bytes)
 * [MPI Sanitizer] Post-call: MPI_Gatherv returned MPI_SUCCESS
 * 
 * Expected Sanitizer Output (Process 1 - Non-root):
 * 
 * [MPI Sanitizer] Pre-call: MPI_Gather at gather_scatter.c:58
 * [MPI Sanitizer]   - Send buffer: 0x7f8b4c000b20, size 40 bytes
 * [MPI Sanitizer]   - Recv buffer: NULL (non-root process)
 * [MPI Sanitizer]   - Participating in collective as non-root
 * [MPI Sanitizer] Post-call: MPI_Gather returned MPI_SUCCESS
 * 
 * [MPI Sanitizer] Performance Summary for Process 0:
 * [MPI Sanitizer]   - Total collective operations: 5
 * [MPI Sanitizer]   - Total collective time: 0.005432 seconds
 * [MPI Sanitizer]   - Data volume processed: 0.8 KB
 * [MPI Sanitizer]   - Average collective latency: 1.09 milliseconds
 */