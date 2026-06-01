/**
 * @file deadlock_example.c
 * @brief Examples that demonstrate potential deadlock scenarios
 * 
 * This example shows common MPI programming patterns that can lead to
 * deadlocks, and demonstrates how the MPI Usage Sanitizer detects and
 * warns about these dangerous patterns.
 * 
 * Expected instrumentation:
 * - Deadlock pattern detection in blocking operations
 * - Circular dependency analysis
 * - Timeout warnings for long-running operations
 * - Suggestions for safer communication patterns
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1000
#define LARGE_BUFFER_SIZE 100000  // Large enough to potentially cause buffering issues

// Function to demonstrate safe communication pattern
void safe_communication_pattern(int rank, int size) {
    int *send_buf = (int*)malloc(BUFFER_SIZE * sizeof(int));
    int *recv_buf = (int*)malloc(BUFFER_SIZE * sizeof(int));
    
    // Initialize send buffer
    for (int i = 0; i < BUFFER_SIZE; i++) {
        send_buf[i] = rank * 1000 + i;
    }
    
    printf("Process %d: Demonstrating SAFE communication pattern\n", rank);
    
    if (rank == 0) {
        // Process 0: Send first, then receive
        MPI_Send(send_buf, BUFFER_SIZE, MPI_INT, 1, 0, MPI_COMM_WORLD);
        printf("Process %d: Sent message to process 1\n", rank);
        
        MPI_Recv(recv_buf, BUFFER_SIZE, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Process %d: Received message from process 1\n", rank);
        
    } else if (rank == 1) {
        // Process 1: Receive first, then send
        MPI_Recv(recv_buf, BUFFER_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Process %d: Received message from process 0\n", rank);
        
        MPI_Send(send_buf, BUFFER_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD);
        printf("Process %d: Sent message to process 0\n", rank);
    }
    
    free(send_buf);
    free(recv_buf);
}

// Function to demonstrate deadlock-prone pattern
void deadlock_prone_pattern(int rank, int size) {
    int *send_buf = (int*)malloc(LARGE_BUFFER_SIZE * sizeof(int));
    int *recv_buf = (int*)malloc(LARGE_BUFFER_SIZE * sizeof(int));
    
    // Initialize send buffer
    for (int i = 0; i < LARGE_BUFFER_SIZE; i++) {
        send_buf[i] = rank * 10000 + i;
    }
    
    printf("Process %d: Demonstrating DEADLOCK-PRONE pattern\n", rank);
    
    if (rank == 0) {
        // DANGEROUS: Both processes try to send first
        // This can deadlock if the message is too large for system buffers
        
        // MPI_Send will be instrumented to detect:
        // - Potential circular dependency (both processes sending simultaneously)
        // - Large message size that may exceed system buffers
        // - Blocking operation that could cause indefinite wait
        
        printf("Process %d: Attempting to send large message to process 1...\n", rank);
        MPI_Send(send_buf, LARGE_BUFFER_SIZE, MPI_INT, 1, 0, MPI_COMM_WORLD);
        printf("Process %d: Large message sent (this may not print if deadlocked)\n", rank);
        
        printf("Process %d: Attempting to receive from process 1...\n", rank);
        MPI_Recv(recv_buf, LARGE_BUFFER_SIZE, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Process %d: Received large message from process 1\n", rank);
        
    } else if (rank == 1) {
        // DANGEROUS: Same pattern - both try to send first
        printf("Process %d: Attempting to send large message to process 0...\n", rank);
        MPI_Send(send_buf, LARGE_BUFFER_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD);
        printf("Process %d: Large message sent (this may not print if deadlocked)\n", rank);
        
        printf("Process %d: Attempting to receive from process 0...\n", rank);
        MPI_Recv(recv_buf, LARGE_BUFFER_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Process %d: Received large message from process 0\n", rank);
    }
    
    free(send_buf);
    free(recv_buf);
}

// Function to demonstrate collective deadlock
void collective_deadlock_pattern(int rank, int size) {
    int data = rank * 100;
    int result;
    
    printf("Process %d: Demonstrating collective operation deadlock\n", rank);
    
    if (rank == 0) {
        // DANGEROUS: Not all processes participate in collective
        printf("Process %d: Skipping MPI_Allreduce (DANGEROUS!)\n", rank);
        
        // This will cause a deadlock because other processes are waiting
        // in the collective operation while process 0 is not participating
        
        // Simulate some other work instead of participating
        for (int i = 0; i < 1000000; i++) {
            volatile double x = i * 0.001;
        }
        
        printf("Process %d: Finished non-collective work\n", rank);
        
    } else {
        // Other processes participate in collective
        // MPI_Allreduce will be instrumented to detect:
        // - Missing processes in collective operation
        // - Timeout in collective synchronization
        // - Inconsistent collective participation
        
        printf("Process %d: Participating in MPI_Allreduce\n", rank);
        MPI_Allreduce(&data, &result, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
        printf("Process %d: Allreduce completed with result %d\n", rank, result);
    }
}

// Function to demonstrate circular communication deadlock
void circular_deadlock_pattern(int rank, int size) {
    if (size < 3) {
        if (rank == 0) {
            printf("Circular deadlock example requires at least 3 processes\n");
        }
        return;
    }
    
    int send_data = rank * 1000;
    int recv_data;
    
    printf("Process %d: Demonstrating circular communication deadlock\n", rank);
    
    // DANGEROUS: Circular communication pattern
    // Each process sends to next and receives from previous
    // All using blocking operations simultaneously
    
    int next_rank = (rank + 1) % size;
    int prev_rank = (rank - 1 + size) % size;
    
    printf("Process %d: Sending to %d, receiving from %d\n", rank, next_rank, prev_rank);
    
    // This pattern can deadlock because all processes are trying to send
    // before any process is ready to receive
    
    // MPI_Send will be instrumented to detect:
    // - Circular dependency in communication pattern
    // - All processes in send state simultaneously
    // - Potential for system buffer exhaustion
    
    MPI_Send(&send_data, 1, MPI_INT, next_rank, 0, MPI_COMM_WORLD);
    printf("Process %d: Send completed\n", rank);
    
    MPI_Recv(&recv_data, 1, MPI_INT, prev_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("Process %d: Received %d from process %d\n", rank, recv_data, prev_rank);
}

// Function to demonstrate safe non-blocking alternative
void safe_nonblocking_pattern(int rank, int size) {
    int *send_buf = (int*)malloc(LARGE_BUFFER_SIZE * sizeof(int));
    int *recv_buf = (int*)malloc(LARGE_BUFFER_SIZE * sizeof(int));
    MPI_Request send_req, recv_req;
    MPI_Status status;
    
    // Initialize send buffer
    for (int i = 0; i < LARGE_BUFFER_SIZE; i++) {
        send_buf[i] = rank * 10000 + i;
    }
    
    printf("Process %d: Demonstrating SAFE non-blocking pattern\n", rank);
    
    if (rank == 0) {
        // Use non-blocking operations to avoid deadlock
        MPI_Isend(send_buf, LARGE_BUFFER_SIZE, MPI_INT, 1, 0, MPI_COMM_WORLD, &send_req);
        MPI_Irecv(recv_buf, LARGE_BUFFER_SIZE, MPI_INT, 1, 0, MPI_COMM_WORLD, &recv_req);
        
        printf("Process %d: Non-blocking operations posted\n", rank);
        
        // Wait for completion
        MPI_Wait(&send_req, &status);
        printf("Process %d: Send completed\n", rank);
        
        MPI_Wait(&recv_req, &status);
        printf("Process %d: Receive completed\n", rank);
        
    } else if (rank == 1) {
        // Same pattern for process 1
        MPI_Isend(send_buf, LARGE_BUFFER_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, &send_req);
        MPI_Irecv(recv_buf, LARGE_BUFFER_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, &recv_req);
        
        printf("Process %d: Non-blocking operations posted\n", rank);
        
        MPI_Wait(&send_req, &status);
        printf("Process %d: Send completed\n", rank);
        
        MPI_Wait(&recv_req, &status);
        printf("Process %d: Receive completed\n", rank);
    }
    
    free(send_buf);
    free(recv_buf);
}

int main(int argc, char *argv[]) {
    int rank, size;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (size < 2) {
        if (rank == 0) {
            printf("This example requires at least 2 processes\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    
    if (rank == 0) {
        printf("=== MPI Deadlock Detection Examples ===\n");
        printf("Running with %d processes\n", size);
        printf("Note: Some examples may intentionally deadlock!\n\n");
    }
    
    // Example 1: Safe communication pattern
    if (rank == 0) printf("--- Example 1: Safe Communication ---\n");
    safe_communication_pattern(rank, size);
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Example 2: Deadlock-prone pattern (commented out to avoid actual deadlock)
    /*
    if (rank == 0) printf("\n--- Example 2: Deadlock-Prone Pattern ---\n");
    deadlock_prone_pattern(rank, size);
    MPI_Barrier(MPI_COMM_WORLD);
    */
    
    // Example 3: Collective deadlock (commented out to avoid actual deadlock)
    /*
    if (rank == 0) printf("\n--- Example 3: Collective Deadlock ---\n");
    collective_deadlock_pattern(rank, size);
    MPI_Barrier(MPI_COMM_WORLD);
    */
    
    // Example 4: Circular deadlock (commented out to avoid actual deadlock)
    /*
    if (rank == 0) printf("\n--- Example 4: Circular Deadlock ---\n");
    circular_deadlock_pattern(rank, size);
    MPI_Barrier(MPI_COMM_WORLD);
    */
    
    // Example 5: Safe non-blocking alternative
    if (rank == 0) printf("\n--- Example 5: Safe Non-blocking Pattern ---\n");
    safe_nonblocking_pattern(rank, size);
    MPI_Barrier(MPI_COMM_WORLD);
    
    if (rank == 0) {
        printf("\n=== Deadlock Prevention Tips ===\n");
        printf("1. Use non-blocking operations for symmetric communication\n");
        printf("2. Ensure all processes participate in collective operations\n");
        printf("3. Avoid circular communication with blocking operations\n");
        printf("4. Consider message sizes and system buffer limits\n");
        printf("5. Use MPI_Sendrecv for bidirectional communication\n");
    }
    
    MPI_Finalize();
    return EXIT_SUCCESS;
}

/*
 * Expected Sanitizer Output (Deadlock-Prone Pattern):
 * 
 * [MPI Sanitizer] Pre-call: MPI_Send at deadlock_example.c:67
 * [MPI Sanitizer] WARNING: Potential deadlock detected!
 * [MPI Sanitizer]   - Large message size: 400000 bytes
 * [MPI Sanitizer]   - Blocking send operation
 * [MPI Sanitizer]   - Symmetric communication pattern detected
 * [MPI Sanitizer]   - Risk: Both processes may be sending simultaneously
 * [MPI Sanitizer] RECOMMENDATION: Use MPI_Isend/MPI_Irecv or MPI_Sendrecv
 * [MPI Sanitizer]   - Buffer: 0x7f8b4c000b20, size 400000 bytes
 * [MPI Sanitizer]   - Destination: 1, Tag: 0
 * [MPI Sanitizer] Post-call: MPI_Send - TIMEOUT WARNING after 5 seconds
 * [MPI Sanitizer] ERROR: Operation appears to be deadlocked!
 * [MPI Sanitizer]   - Consider using non-blocking operations
 * [MPI Sanitizer]   - Check for symmetric communication patterns
 * 
 * Expected Sanitizer Output (Collective Deadlock):
 * 
 * [MPI Sanitizer] Pre-call: MPI_Allreduce at deadlock_example.c:115
 * [MPI Sanitizer] WARNING: Collective operation timeout detected!
 * [MPI Sanitizer]   - Operation: MPI_Allreduce
 * [MPI Sanitizer]   - Waiting processes: 1, 2, 3 (missing: 0)
 * [MPI Sanitizer]   - Timeout: 10 seconds
 * [MPI Sanitizer] ERROR: Collective deadlock - not all processes participating
 * [MPI Sanitizer] RECOMMENDATION: Ensure all processes call collective operations
 * 
 * Expected Sanitizer Output (Circular Deadlock):
 * 
 * [MPI Sanitizer] Pre-call: MPI_Send at deadlock_example.c:155
 * [MPI Sanitizer] WARNING: Circular communication pattern detected!
 * [MPI Sanitizer]   - Process 0 → Process 1
 * [MPI Sanitizer]   - Process 1 → Process 2  
 * [MPI Sanitizer]   - Process 2 → Process 0
 * [MPI Sanitizer]   - All processes in send state: HIGH DEADLOCK RISK
 * [MPI Sanitizer] RECOMMENDATION: Use non-blocking operations or MPI_Sendrecv
 * [MPI Sanitizer] Post-call: MPI_Send - TIMEOUT WARNING after 5 seconds
 * 
 * Expected Sanitizer Output (Safe Non-blocking):
 * 
 * [MPI Sanitizer] Pre-call: MPI_Isend at deadlock_example.c:175
 * [MPI Sanitizer]   - Non-blocking operation: Safe for symmetric communication
 * [MPI Sanitizer]   - Large message: 400000 bytes (non-blocking recommended)
 * [MPI Sanitizer] Post-call: MPI_Isend returned MPI_SUCCESS
 * [MPI Sanitizer]   - Request posted successfully
 * 
 * [MPI Sanitizer] Pre-call: MPI_Irecv at deadlock_example.c:176
 * [MPI Sanitizer]   - Non-blocking receive: Complements non-blocking send
 * [MPI Sanitizer] Post-call: MPI_Irecv returned MPI_SUCCESS
 * 
 * [MPI Sanitizer] Deadlock Analysis Summary:
 * [MPI Sanitizer]   - Potential deadlocks detected: 3
 * [MPI Sanitizer]   - Safe patterns used: 2
 * [MPI Sanitizer]   - Recommendations provided: 3
 * [MPI Sanitizer]   - Critical warnings: 2
 */