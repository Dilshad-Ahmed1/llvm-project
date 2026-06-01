/**
 * @file bandwidth_test.c
 * @brief MPI bandwidth measurement and performance monitoring example
 * 
 * This example demonstrates how the MPI Usage Sanitizer instruments
 * communication operations for performance monitoring, bandwidth analysis,
 * and bottleneck detection.
 * 
 * Expected instrumentation:
 * - Communication timing and bandwidth calculation
 * - Message size impact analysis
 * - Network utilization monitoring
 * - Performance bottleneck identification
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MIN_MESSAGE_SIZE 1024        // 1 KB
#define MAX_MESSAGE_SIZE (16*1024*1024)  // 16 MB
#define NUM_ITERATIONS 10
#define WARMUP_ITERATIONS 3

// Structure to hold performance results
typedef struct {
    int message_size;
    double avg_time;
    double min_time;
    double max_time;
    double bandwidth_mbps;
    double latency_us;
} PerformanceResult;

// Function to perform bandwidth test between two processes
void bandwidth_test_p2p(int rank, int size, PerformanceResult *results, int *num_results) {
    if (size < 2) return;
    
    int partner = (rank == 0) ? 1 : 0;
    *num_results = 0;
    
    printf("Process %d: Starting point-to-point bandwidth test with process %d\n", 
           rank, partner);
    
    // Test different message sizes
    for (int msg_size = MIN_MESSAGE_SIZE; msg_size <= MAX_MESSAGE_SIZE; msg_size *= 2) {
        char *send_buffer = (char*)malloc(msg_size);
        char *recv_buffer = (char*)malloc(msg_size);
        
        if (!send_buffer || !recv_buffer) {
            fprintf(stderr, "Process %d: Memory allocation failed for size %d\n", 
                    rank, msg_size);
            continue;
        }
        
        // Initialize send buffer
        memset(send_buffer, rank + 1, msg_size);
        
        double times[NUM_ITERATIONS];
        double total_time = 0.0;
        double min_time = 1e9, max_time = 0.0;
        
        // Warmup iterations
        for (int i = 0; i < WARMUP_ITERATIONS; i++) {
            if (rank == 0) {
                MPI_Send(send_buffer, msg_size, MPI_CHAR, partner, 0, MPI_COMM_WORLD);
                MPI_Recv(recv_buffer, msg_size, MPI_CHAR, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else if (rank == 1) {
                MPI_Recv(recv_buffer, msg_size, MPI_CHAR, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(send_buffer, msg_size, MPI_CHAR, partner, 0, MPI_COMM_WORLD);
            }
        }
        
        MPI_Barrier(MPI_COMM_WORLD);
        
        // Actual measurement iterations
        for (int i = 0; i < NUM_ITERATIONS; i++) {
            double start_time = MPI_Wtime();
            
            // MPI operations will be instrumented to collect:
            // - Individual operation timing
            // - Message size and bandwidth calculations
            // - Network utilization metrics
            // - Communication pattern analysis
            
            if (rank == 0) {
                MPI_Send(send_buffer, msg_size, MPI_CHAR, partner, i, MPI_COMM_WORLD);
                MPI_Recv(recv_buffer, msg_size, MPI_CHAR, partner, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else if (rank == 1) {
                MPI_Recv(recv_buffer, msg_size, MPI_CHAR, partner, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(send_buffer, msg_size, MPI_CHAR, partner, i, MPI_COMM_WORLD);
            }
            
            double end_time = MPI_Wtime();
            times[i] = end_time - start_time;
            
            total_time += times[i];
            if (times[i] < min_time) min_time = times[i];
            if (times[i] > max_time) max_time = times[i];
        }
        
        // Calculate statistics
        double avg_time = total_time / NUM_ITERATIONS;
        double bytes_transferred = 2.0 * msg_size;  // Round-trip
        double bandwidth_mbps = (bytes_transferred / (1024.0 * 1024.0)) / avg_time;
        double latency_us = (avg_time * 1000000.0) / 2.0;  // One-way latency
        
        // Store results
        if (*num_results < 20) {  // Assuming max 20 message sizes
            results[*num_results].message_size = msg_size;
            results[*num_results].avg_time = avg_time;
            results[*num_results].min_time = min_time;
            results[*num_results].max_time = max_time;
            results[*num_results].bandwidth_mbps = bandwidth_mbps;
            results[*num_results].latency_us = latency_us;
            (*num_results)++;
        }
        
        if (rank == 0) {
            printf("Message size: %8d bytes, Bandwidth: %8.2f MB/s, Latency: %8.2f μs\n",
                   msg_size, bandwidth_mbps, latency_us);
        }
        
        free(send_buffer);
        free(recv_buffer);
        
        MPI_Barrier(MPI_COMM_WORLD);
    }
}

// Function to test collective operation bandwidth
void bandwidth_test_collective(int rank, int size, PerformanceResult *results, int *num_results) {
    printf("Process %d: Starting collective bandwidth test\n", rank);
    
    *num_results = 0;
    
    // Test MPI_Allreduce with different data sizes
    for (int count = 1024; count <= 1024*1024; count *= 4) {
        double *data = (double*)malloc(count * sizeof(double));
        double *result = (double*)malloc(count * sizeof(double));
        
        if (!data || !result) {
            fprintf(stderr, "Process %d: Memory allocation failed for count %d\n", 
                    rank, count);
            continue;
        }
        
        // Initialize data
        for (int i = 0; i < count; i++) {
            data[i] = rank * 1000.0 + i;
        }
        
        double times[NUM_ITERATIONS];
        double total_time = 0.0;
        
        // Warmup
        for (int i = 0; i < WARMUP_ITERATIONS; i++) {
            MPI_Allreduce(data, result, count, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        }
        
        MPI_Barrier(MPI_COMM_WORLD);
        
        // Measurement iterations
        for (int i = 0; i < NUM_ITERATIONS; i++) {
            double start_time = MPI_Wtime();
            
            // MPI_Allreduce will be instrumented to collect:
            // - Collective operation timing
            // - Data volume and reduction bandwidth
            // - Synchronization overhead analysis
            // - Scalability metrics
            
            MPI_Allreduce(data, result, count, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
            
            double end_time = MPI_Wtime();
            times[i] = end_time - start_time;
            total_time += times[i];
        }
        
        double avg_time = total_time / NUM_ITERATIONS;
        double data_size_mb = (count * sizeof(double) * size) / (1024.0 * 1024.0);
        double effective_bandwidth = data_size_mb / avg_time;
        
        if (rank == 0) {
            printf("Allreduce count: %8d, Data: %6.2f MB, Bandwidth: %8.2f MB/s, Time: %8.4f s\n",
                   count, data_size_mb, effective_bandwidth, avg_time);
        }
        
        // Store results
        if (*num_results < 20) {
            results[*num_results].message_size = count * sizeof(double);
            results[*num_results].avg_time = avg_time;
            results[*num_results].bandwidth_mbps = effective_bandwidth;
            (*num_results)++;
        }
        
        free(data);
        free(result);
        
        MPI_Barrier(MPI_COMM_WORLD);
    }
}

// Function to test communication patterns and their impact
void communication_pattern_analysis(int rank, int size) {
    if (size < 4) return;
    
    printf("Process %d: Communication pattern analysis\n", rank);
    
    const int msg_size = 64 * 1024;  // 64 KB messages
    char *send_buffer = (char*)malloc(msg_size);
    char *recv_buffer = (char*)malloc(msg_size);
    
    memset(send_buffer, rank, msg_size);
    
    // Pattern 1: All-to-one communication
    if (rank == 0) {
        printf("\n--- Pattern 1: All-to-one communication ---\n");
    }
    
    double start_time = MPI_Wtime();
    
    if (rank == 0) {
        // Root receives from all other processes
        for (int src = 1; src < size; src++) {
            MPI_Recv(recv_buffer, msg_size, MPI_CHAR, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    } else {
        // All other processes send to root
        MPI_Send(send_buffer, msg_size, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }
    
    double pattern1_time = MPI_Wtime() - start_time;
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Pattern 2: Ring communication
    if (rank == 0) {
        printf("--- Pattern 2: Ring communication ---\n");
    }
    
    start_time = MPI_Wtime();
    
    int next_rank = (rank + 1) % size;
    int prev_rank = (rank - 1 + size) % size;
    
    // Use non-blocking operations to avoid deadlock
    MPI_Request send_req, recv_req;
    MPI_Isend(send_buffer, msg_size, MPI_CHAR, next_rank, 1, MPI_COMM_WORLD, &send_req);
    MPI_Irecv(recv_buffer, msg_size, MPI_CHAR, prev_rank, 1, MPI_COMM_WORLD, &recv_req);
    
    MPI_Wait(&send_req, MPI_STATUS_IGNORE);
    MPI_Wait(&recv_req, MPI_STATUS_IGNORE);
    
    double pattern2_time = MPI_Wtime() - start_time;
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Pattern 3: All-to-all communication
    if (rank == 0) {
        printf("--- Pattern 3: All-to-all communication ---\n");
    }
    
    start_time = MPI_Wtime();
    
    // Simple all-to-all using MPI_Alltoall
    char *send_data = (char*)malloc(msg_size * size);
    char *recv_data = (char*)malloc(msg_size * size);
    
    // Initialize send data
    for (int i = 0; i < size; i++) {
        memset(send_data + i * msg_size, rank * 10 + i, msg_size);
    }
    
    MPI_Alltoall(send_data, msg_size, MPI_CHAR, recv_data, msg_size, MPI_CHAR, MPI_COMM_WORLD);
    
    double pattern3_time = MPI_Wtime() - start_time;
    
    // Report pattern performance
    if (rank == 0) {
        printf("Pattern performance comparison:\n");
        printf("All-to-one:  %8.4f seconds\n", pattern1_time);
        printf("Ring:        %8.4f seconds\n", pattern2_time);
        printf("All-to-all:  %8.4f seconds\n", pattern3_time);
        
        double data_mb = (msg_size * (size - 1)) / (1024.0 * 1024.0);
        printf("Data volume: %8.2f MB per pattern\n", data_mb);
    }
    
    free(send_buffer);
    free(recv_buffer);
    free(send_data);
    free(recv_data);
}

int main(int argc, char *argv[]) {
    int rank, size;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (rank == 0) {
        printf("=== MPI Bandwidth and Performance Test ===\n");
        printf("Number of processes: %d\n", size);
        printf("Message size range: %d bytes to %d MB\n", 
               MIN_MESSAGE_SIZE, MAX_MESSAGE_SIZE / (1024*1024));
        printf("Iterations per test: %d\n\n", NUM_ITERATIONS);
    }
    
    PerformanceResult p2p_results[20];
    PerformanceResult collective_results[20];
    int num_p2p_results, num_collective_results;
    
    // Test 1: Point-to-point bandwidth
    if (size >= 2) {
        if (rank == 0) printf("=== Point-to-Point Bandwidth Test ===\n");
        bandwidth_test_p2p(rank, size, p2p_results, &num_p2p_results);
        MPI_Barrier(MPI_COMM_WORLD);
    }
    
    // Test 2: Collective bandwidth
    if (rank == 0) printf("\n=== Collective Operation Bandwidth Test ===\n");
    bandwidth_test_collective(rank, size, collective_results, &num_collective_results);
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Test 3: Communication pattern analysis
    if (size >= 4) {
        if (rank == 0) printf("\n=== Communication Pattern Analysis ===\n");
        communication_pattern_analysis(rank, size);
        MPI_Barrier(MPI_COMM_WORLD);
    }
    
    // Performance summary
    if (rank == 0) {
        printf("\n=== Performance Summary ===\n");
        
        if (num_p2p_results > 0) {
            printf("Point-to-point peak bandwidth: %.2f MB/s\n", 
                   p2p_results[num_p2p_results-1].bandwidth_mbps);
            printf("Point-to-point minimum latency: %.2f μs\n", 
                   p2p_results[0].latency_us);
        }
        
        if (num_collective_results > 0) {
            printf("Collective peak bandwidth: %.2f MB/s\n", 
                   collective_results[num_collective_results-1].bandwidth_mbps);
        }
        
        printf("\nPerformance monitoring enabled by MPI Usage Sanitizer\n");
        printf("Check sanitizer output for detailed timing analysis\n");
    }
    
    MPI_Finalize();
    return EXIT_SUCCESS;
}

/*
 * Expected Sanitizer Output (Performance Monitoring):
 * 
 * [MPI Sanitizer] Performance Monitor: Bandwidth test started
 * [MPI Sanitizer]   - Test type: Point-to-point
 * [MPI Sanitizer]   - Participants: Process 0 ↔ Process 1
 * [MPI Sanitizer]   - Message size range: 1 KB - 16 MB
 * 
 * [MPI Sanitizer] Pre-call: MPI_Send at bandwidth_test.c:85
 * [MPI Sanitizer]   - Performance timing: ENABLED
 * [MPI Sanitizer]   - Message size: 1024 bytes
 * [MPI Sanitizer]   - Bandwidth calculation: ACTIVE
 * [MPI Sanitizer] Post-call: MPI_Send returned MPI_SUCCESS
 * [MPI Sanitizer]   - Transfer time: 0.000045 seconds
 * [MPI Sanitizer]   - Effective bandwidth: 22.7 MB/s
 * [MPI Sanitizer]   - Network utilization: 15%
 * 
 * [MPI Sanitizer] Pre-call: MPI_Send at bandwidth_test.c:85 (16 MB)
 * [MPI Sanitizer]   - Large message detected: 16777216 bytes
 * [MPI Sanitizer]   - Expected high bandwidth transfer
 * [MPI Sanitizer] Post-call: MPI_Send returned MPI_SUCCESS
 * [MPI Sanitizer]   - Transfer time: 0.156789 seconds
 * [MPI Sanitizer]   - Effective bandwidth: 107.0 MB/s
 * [MPI Sanitizer]   - Network utilization: 89%
 * [MPI Sanitizer]   - Peak bandwidth achieved
 * 
 * [MPI Sanitizer] Pre-call: MPI_Allreduce at bandwidth_test.c:145
 * [MPI Sanitizer]   - Collective performance monitoring: ENABLED
 * [MPI Sanitizer]   - Operation: MPI_Allreduce (SUM)
 * [MPI Sanitizer]   - Data count: 262144 doubles (2.0 MB)
 * [MPI Sanitizer]   - Processes: 4
 * [MPI Sanitizer]   - Expected algorithm: Recursive doubling
 * [MPI Sanitizer] Post-call: MPI_Allreduce returned MPI_SUCCESS
 * [MPI Sanitizer]   - Collective time: 0.002345 seconds
 * [MPI Sanitizer]   - Effective bandwidth: 853.6 MB/s
 * [MPI Sanitizer]   - Synchronization overhead: 12%
 * [MPI Sanitizer]   - Algorithm efficiency: 94%
 * 
 * [MPI Sanitizer] Communication Pattern Analysis:
 * [MPI Sanitizer]   - All-to-one pattern detected
 * [MPI Sanitizer]     * Bottleneck: Process 0 (receiver)
 * [MPI Sanitizer]     * Serialization factor: 75%
 * [MPI Sanitizer]     * Recommendation: Use MPI_Gather for better performance
 * 
 * [MPI Sanitizer]   - Ring pattern detected
 * [MPI Sanitizer]     * Communication efficiency: 98%
 * [MPI Sanitizer]     * Load balance: EXCELLENT
 * [MPI Sanitizer]     * Scalability: GOOD
 * 
 * [MPI Sanitizer]   - All-to-all pattern detected
 * [MPI Sanitizer]     * Network congestion: MODERATE
 * [MPI Sanitizer]     * Bandwidth utilization: 67%
 * [MPI Sanitizer]     * Recommendation: Consider staged communication
 * 
 * [MPI Sanitizer] Performance Summary:
 * [MPI Sanitizer]   - Total MPI operations monitored: 156
 * [MPI Sanitizer]   - Peak point-to-point bandwidth: 107.0 MB/s
 * [MPI Sanitizer]   - Peak collective bandwidth: 853.6 MB/s
 * [MPI Sanitizer]   - Average network utilization: 45%
 * [MPI Sanitizer]   - Bottlenecks detected: 1 (all-to-one pattern)
 * [MPI Sanitizer]   - Performance recommendations: 2
 * [MPI Sanitizer]   - Total communication time: 2.456 seconds
 * [MPI Sanitizer]   - Total data transferred: 89.3 MB
 */