/**
 * @file allreduce_patterns.c
 * @brief Comprehensive MPI_Allreduce examples demonstrating various usage patterns
 * 
 * This example shows different MPI_Allreduce operations and demonstrates how the
 * MPI Usage Sanitizer instruments collective reduction operations for correctness
 * and performance monitoring.
 * 
 * Expected instrumentation:
 * - Operation consistency validation across all processes
 * - Buffer alignment and size validation
 * - Datatype and operation compatibility checking
 * - Performance monitoring for collective operations
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define ARRAY_SIZE 1000

// Function to initialize test data
void initialize_data(double *data, int size, int rank) {
    for (int i = 0; i < size; i++) {
        data[i] = (double)(rank + 1) * (i + 1) * 0.1;
    }
}

// Function to print array statistics
void print_statistics(const char *label, double *data, int size, int rank) {
    double sum = 0.0, min_val = data[0], max_val = data[0];
    
    for (int i = 0; i < size; i++) {
        sum += data[i];
        if (data[i] < min_val) min_val = data[i];
        if (data[i] > max_val) max_val = data[i];
    }
    
    printf("Process %d %s: sum=%.2f, min=%.2f, max=%.2f, avg=%.2f\n",
           rank, label, sum, min_val, max_val, sum / size);
}

int main(int argc, char *argv[]) {
    int rank, size;
    double *local_data, *global_sum, *global_max, *global_min;
    int local_count, global_count_sum;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Allocate arrays for different reduction operations
    local_data = (double*)malloc(ARRAY_SIZE * sizeof(double));
    global_sum = (double*)malloc(ARRAY_SIZE * sizeof(double));
    global_max = (double*)malloc(ARRAY_SIZE * sizeof(double));
    global_min = (double*)malloc(ARRAY_SIZE * sizeof(double));
    
    if (!local_data || !global_sum || !global_max || !global_min) {
        fprintf(stderr, "Process %d: Memory allocation failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    
    // Initialize local data
    initialize_data(local_data, ARRAY_SIZE, rank);
    local_count = ARRAY_SIZE + rank;  // Different count per process for testing
    
    if (rank == 0) {
        printf("Starting MPI_Allreduce examples with %d processes\n", size);
        printf("Array size: %d elements per process\n", ARRAY_SIZE);
    }
    
    print_statistics("local data", local_data, ARRAY_SIZE, rank);
    
    // Example 1: Sum reduction
    // MPI_Allreduce will be instrumented to validate:
    // - Send and receive buffers are different and valid
    // - Count is consistent across all processes
    // - Datatype matches buffer element type
    // - Operation (MPI_SUM) is valid for the datatype
    // - Communicator is consistent across all processes
    
    double start_time = MPI_Wtime();
    
    int result = MPI_Allreduce(local_data,           // sendbuf
                              global_sum,            // recvbuf
                              ARRAY_SIZE,            // count
                              MPI_DOUBLE,            // datatype
                              MPI_SUM,               // operation
                              MPI_COMM_WORLD);       // communicator
    
    double sum_time = MPI_Wtime() - start_time;
    
    if (result != MPI_SUCCESS) {
        fprintf(stderr, "Process %d: MPI_Allreduce (SUM) failed with code %d\n", 
                rank, result);
        MPI_Abort(MPI_COMM_WORLD, result);
    }
    
    print_statistics("global sum", global_sum, ARRAY_SIZE, rank);
    printf("Process %d: Sum reduction completed in %f seconds\n", rank, sum_time);
    
    // Example 2: Maximum reduction
    start_time = MPI_Wtime();
    
    result = MPI_Allreduce(local_data, global_max, ARRAY_SIZE, 
                          MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    
    double max_time = MPI_Wtime() - start_time;
    
    if (result == MPI_SUCCESS) {
        print_statistics("global max", global_max, ARRAY_SIZE, rank);
        printf("Process %d: Max reduction completed in %f seconds\n", rank, max_time);
    }
    
    // Example 3: Minimum reduction
    start_time = MPI_Wtime();
    
    result = MPI_Allreduce(local_data, global_min, ARRAY_SIZE, 
                          MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
    
    double min_time = MPI_Wtime() - start_time;
    
    if (result == MPI_SUCCESS) {
        print_statistics("global min", global_min, ARRAY_SIZE, rank);
        printf("Process %d: Min reduction completed in %f seconds\n", rank, min_time);
    }
    
    // Example 4: Integer reduction with different operation
    result = MPI_Allreduce(&local_count,            // sendbuf
                          &global_count_sum,         // recvbuf
                          1,                         // count
                          MPI_INT,                   // datatype
                          MPI_SUM,                   // operation
                          MPI_COMM_WORLD);           // communicator
    
    if (result == MPI_SUCCESS) {
        printf("Process %d: Local count = %d, Global count sum = %d\n", 
               rank, local_count, global_count_sum);
    }
    
    // Example 5: In-place reduction (advanced pattern)
    // Note: This reuses the local_data buffer for both input and output
    double *temp_data = (double*)malloc(ARRAY_SIZE * sizeof(double));
    memcpy(temp_data, local_data, ARRAY_SIZE * sizeof(double));
    
    start_time = MPI_Wtime();
    
    result = MPI_Allreduce(MPI_IN_PLACE,            // sendbuf (in-place)
                          temp_data,                 // recvbuf
                          ARRAY_SIZE,                // count
                          MPI_DOUBLE,                // datatype
                          MPI_SUM,                   // operation
                          MPI_COMM_WORLD);           // communicator
    
    double inplace_time = MPI_Wtime() - start_time;
    
    if (result == MPI_SUCCESS) {
        print_statistics("in-place sum", temp_data, ARRAY_SIZE, rank);
        printf("Process %d: In-place reduction completed in %f seconds\n", 
               rank, inplace_time);
    }
    
    // Example 6: Custom operation (logical AND for demonstration)
    int local_flag = (rank % 2 == 0) ? 1 : 0;  // Even ranks set flag
    int global_flag;
    
    result = MPI_Allreduce(&local_flag, &global_flag, 1, 
                          MPI_INT, MPI_LAND, MPI_COMM_WORLD);
    
    if (result == MPI_SUCCESS) {
        printf("Process %d: Local flag = %d, Global AND result = %d\n", 
               rank, local_flag, global_flag);
    }
    
    // Performance summary
    if (rank == 0) {
        printf("\nPerformance Summary:\n");
        printf("Sum reduction:      %f seconds\n", sum_time);
        printf("Max reduction:      %f seconds\n", max_time);
        printf("Min reduction:      %f seconds\n", min_time);
        printf("In-place reduction: %f seconds\n", inplace_time);
        
        double total_data = ARRAY_SIZE * sizeof(double) * size;
        printf("Total data processed: %.2f KB per reduction\n", total_data / 1024.0);
    }
    
    // Cleanup
    free(local_data);
    free(global_sum);
    free(global_max);
    free(global_min);
    free(temp_data);
    
    MPI_Finalize();
    return EXIT_SUCCESS;
}

/*
 * Expected Sanitizer Output (Process 0):
 * 
 * [MPI Sanitizer] Pre-call: MPI_Allreduce at allreduce_patterns.c:73
 * [MPI Sanitizer]   - Collective operation: MPI_Allreduce detected
 * [MPI Sanitizer]   - Send buffer: 0x7f8b4c000b20, size 8000 bytes
 * [MPI Sanitizer]   - Recv buffer: 0x7f8b4c002b20, size 8000 bytes
 * [MPI Sanitizer]   - Buffer overlap check: PASSED (no overlap)
 * [MPI Sanitizer]   - Count: 1000, Datatype: MPI_DOUBLE (8 bytes)
 * [MPI Sanitizer]   - Operation: MPI_SUM (valid for MPI_DOUBLE)
 * [MPI Sanitizer]   - Communicator: MPI_COMM_WORLD (4 processes)
 * [MPI Sanitizer]   - Collective consistency check: PASSED
 * [MPI Sanitizer] Post-call: MPI_Allreduce returned MPI_SUCCESS
 * [MPI Sanitizer]   - Reduction completed successfully
 * [MPI Sanitizer]   - Performance: 0.001234 seconds, 6.5 MB/s effective bandwidth
 * 
 * [MPI Sanitizer] Pre-call: MPI_Allreduce at allreduce_patterns.c:87 (MAX)
 * [MPI Sanitizer]   - Operation: MPI_MAX (valid for MPI_DOUBLE)
 * [MPI Sanitizer]   - Collective consistency check: PASSED
 * [MPI Sanitizer] Post-call: MPI_Allreduce returned MPI_SUCCESS
 * 
 * [MPI Sanitizer] Pre-call: MPI_Allreduce at allreduce_patterns.c:96 (MIN)
 * [MPI Sanitizer]   - Operation: MPI_MIN (valid for MPI_DOUBLE)
 * [MPI Sanitizer] Post-call: MPI_Allreduce returned MPI_SUCCESS
 * 
 * [MPI Sanitizer] Pre-call: MPI_Allreduce at allreduce_patterns.c:105 (INT)
 * [MPI Sanitizer]   - Count: 1, Datatype: MPI_INT (4 bytes)
 * [MPI Sanitizer]   - Operation: MPI_SUM (valid for MPI_INT)
 * [MPI Sanitizer] Post-call: MPI_Allreduce returned MPI_SUCCESS
 * 
 * [MPI Sanitizer] Pre-call: MPI_Allreduce at allreduce_patterns.c:119 (IN_PLACE)
 * [MPI Sanitizer]   - In-place operation detected
 * [MPI Sanitizer]   - Send buffer: MPI_IN_PLACE
 * [MPI Sanitizer]   - Recv buffer: 0x7f8b4c004b20, size 8000 bytes
 * [MPI Sanitizer]   - In-place validation: PASSED
 * [MPI Sanitizer] Post-call: MPI_Allreduce returned MPI_SUCCESS
 * 
 * [MPI Sanitizer] Pre-call: MPI_Allreduce at allreduce_patterns.c:133 (LAND)
 * [MPI Sanitizer]   - Operation: MPI_LAND (logical AND, valid for MPI_INT)
 * [MPI Sanitizer] Post-call: MPI_Allreduce returned MPI_SUCCESS
 * 
 * [MPI Sanitizer] Performance Summary for Process 0:
 * [MPI Sanitizer]   - Total MPI_Allreduce calls: 6
 * [MPI Sanitizer]   - Total reduction time: 0.006789 seconds
 * [MPI Sanitizer]   - Average latency: 1.13 milliseconds
 * [MPI Sanitizer]   - Total data reduced: 40.0 KB
 * [MPI Sanitizer]   - Effective bandwidth: 5.9 MB/s
 */