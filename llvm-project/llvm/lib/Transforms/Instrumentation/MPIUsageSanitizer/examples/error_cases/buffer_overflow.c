/**
 * @file buffer_overflow.c
 * @brief Examples demonstrating buffer size mismatches and overflow conditions
 * 
 * This example shows common buffer-related errors in MPI programs and
 * demonstrates how the MPI Usage Sanitizer detects buffer overflows,
 * underflows, and size mismatches.
 * 
 * Expected instrumentation:
 * - Buffer bounds checking for send/receive operations
 * - Size mismatch detection between sender and receiver
 * - Buffer alignment validation
 * - Memory access pattern analysis
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SMALL_BUFFER_SIZE 100
#define LARGE_BUFFER_SIZE 1000

// Function to demonstrate buffer overflow in receive
void buffer_overflow_receive_example(int rank, int size) {
    if (size < 2) return;
    
    printf("Process %d: Buffer overflow receive example\n", rank);
    
    if (rank == 0) {
        // Sender: Send large amount of data
        int *large_send_buffer = (int*)malloc(LARGE_BUFFER_SIZE * sizeof(int));
        
        for (int i = 0; i < LARGE_BUFFER_SIZE; i++) {
            large_send_buffer[i] = i;
        }
        
        printf("Process %d: Sending %d integers to process 1\n", rank, LARGE_BUFFER_SIZE);
        
        // Send large buffer
        MPI_Send(large_send_buffer, LARGE_BUFFER_SIZE, MPI_INT, 1, 0, MPI_COMM_WORLD);
        
        free(large_send_buffer);
        
    } else if (rank == 1) {
        // Receiver: Try to receive into small buffer (DANGEROUS!)
        int *small_recv_buffer = (int*)malloc(SMALL_BUFFER_SIZE * sizeof(int));
        
        printf("Process %d: Attempting to receive into buffer of size %d\n", 
               rank, SMALL_BUFFER_SIZE);
        
        // MPI_Recv will be instrumented to detect:
        // - Buffer size vs. incoming message size mismatch
        // - Potential buffer overflow condition
        // - Memory bounds violation risk
        
        MPI_Status status;
        MPI_Recv(small_recv_buffer,     // DANGEROUS: Buffer too small!
                SMALL_BUFFER_SIZE,      // Count is smaller than sent data
                MPI_INT,
                0,
                0,
                MPI_COMM_WORLD,
                &status);
        
        // Check how much data was actually received
        int received_count;
        MPI_Get_count(&status, MPI_INT, &received_count);
        
        printf("Process %d: Received %d integers (buffer size was %d)\n",
               rank, received_count, SMALL_BUFFER_SIZE);
        
        free(small_recv_buffer);
    }
}

// Function to demonstrate buffer underflow (sending more than allocated)
void buffer_underflow_send_example(int rank, int size) {
    if (size < 2) return;
    
    printf("Process %d: Buffer underflow send example\n", rank);
    
    if (rank == 0) {
        // Allocate small buffer but try to send more data (DANGEROUS!)
        int *small_send_buffer = (int*)malloc(SMALL_BUFFER_SIZE * sizeof(int));
        
        for (int i = 0; i < SMALL_BUFFER_SIZE; i++) {
            small_send_buffer[i] = i * 10;
        }
        
        printf("Process %d: Allocated buffer for %d integers\n", rank, SMALL_BUFFER_SIZE);
        printf("Process %d: Attempting to send %d integers (MORE than allocated!)\n", 
               rank, LARGE_BUFFER_SIZE);
        
        // MPI_Send will be instrumented to detect:
        // - Send count exceeds allocated buffer size
        // - Potential read beyond buffer bounds
        // - Memory access violation risk
        
        MPI_Send(small_send_buffer,     // Buffer allocated for SMALL_BUFFER_SIZE
                LARGE_BUFFER_SIZE,      // DANGEROUS: Trying to send more!
                MPI_INT,
                1,
                0,
                MPI_COMM_WORLD);
        
        free(small_send_buffer);
        
    } else if (rank == 1) {
        // Receiver expects large buffer
        int *large_recv_buffer = (int*)malloc(LARGE_BUFFER_SIZE * sizeof(int));
        
        printf("Process %d: Waiting to receive %d integers\n", rank, LARGE_BUFFER_SIZE);
        
        MPI_Status status;
        MPI_Recv(large_recv_buffer,
                LARGE_BUFFER_SIZE,
                MPI_INT,
                0,
                0,
                MPI_COMM_WORLD,
                &status);
        
        int received_count;
        MPI_Get_count(&status, MPI_INT, &received_count);
        
        printf("Process %d: Actually received %d integers\n", rank, received_count);
        
        free(large_recv_buffer);
    }
}

// Function to demonstrate NULL pointer usage
void null_pointer_example(int rank, int size) {
    if (size < 2) return;
    
    printf("Process %d: NULL pointer example\n", rank);
    
    if (rank == 0) {
        int *null_buffer = NULL;  // DANGEROUS: NULL pointer
        
        printf("Process %d: Attempting to send from NULL buffer\n", rank);
        
        // MPI_Send will be instrumented to detect:
        // - NULL buffer pointer
        // - Invalid memory access attempt
        // - Segmentation fault risk
        
        MPI_Send(null_buffer,           // DANGEROUS: NULL pointer!
                10,
                MPI_INT,
                1,
                0,
                MPI_COMM_WORLD);
        
    } else if (rank == 1) {
        int recv_buffer[10];
        
        MPI_Status status;
        MPI_Recv(recv_buffer, 10, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    }
}

// Function to demonstrate uninitialized buffer usage
void uninitialized_buffer_example(int rank, int size) {
    if (size < 2) return;
    
    printf("Process %d: Uninitialized buffer example\n", rank);
    
    if (rank == 0) {
        // Allocate buffer but don't initialize (potential issue)
        int *uninitialized_buffer = (int*)malloc(SMALL_BUFFER_SIZE * sizeof(int));
        // Note: Not initializing the buffer - contains garbage values
        
        printf("Process %d: Sending uninitialized buffer data\n", rank);
        
        // MPI_Send will be instrumented to detect:
        // - Potentially uninitialized memory usage
        // - Non-deterministic program behavior
        // - Data integrity concerns
        
        MPI_Send(uninitialized_buffer,  // Contains garbage values
                SMALL_BUFFER_SIZE,
                MPI_INT,
                1,
                0,
                MPI_COMM_WORLD);
        
        free(uninitialized_buffer);
        
    } else if (rank == 1) {
        int recv_buffer[SMALL_BUFFER_SIZE];
        
        MPI_Status status;
        MPI_Recv(recv_buffer, SMALL_BUFFER_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        
        printf("Process %d: Received data (may contain garbage values)\n", rank);
        printf("Process %d: First few values: %d, %d, %d\n", 
               rank, recv_buffer[0], recv_buffer[1], recv_buffer[2]);
    }
}

// Function to demonstrate buffer alignment issues
void buffer_alignment_example(int rank, int size) {
    if (size < 2) return;
    
    printf("Process %d: Buffer alignment example\n", rank);
    
    if (rank == 0) {
        // Create misaligned buffer (for demonstration)
        char *raw_memory = (char*)malloc(SMALL_BUFFER_SIZE * sizeof(int) + 1);
        int *misaligned_buffer = (int*)(raw_memory + 1);  // Misalign by 1 byte
        
        // Initialize the buffer
        for (int i = 0; i < SMALL_BUFFER_SIZE; i++) {
            misaligned_buffer[i] = i * 100;
        }
        
        printf("Process %d: Sending from potentially misaligned buffer\n", rank);
        
        // MPI_Send will be instrumented to detect:
        // - Buffer alignment issues
        // - Performance implications of misalignment
        // - Potential hardware-specific problems
        
        MPI_Send(misaligned_buffer,     // Potentially misaligned
                SMALL_BUFFER_SIZE,
                MPI_INT,
                1,
                0,
                MPI_COMM_WORLD);
        
        free(raw_memory);
        
    } else if (rank == 1) {
        int recv_buffer[SMALL_BUFFER_SIZE];
        
        MPI_Status status;
        MPI_Recv(recv_buffer, SMALL_BUFFER_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        
        printf("Process %d: Received aligned data successfully\n", rank);
    }
}

// Function to demonstrate buffer reuse issues
void buffer_reuse_example(int rank, int size) {
    if (size < 2) return;
    
    printf("Process %d: Buffer reuse example\n", rank);
    
    if (rank == 0) {
        int *buffer = (int*)malloc(SMALL_BUFFER_SIZE * sizeof(int));
        MPI_Request request;
        
        // Initialize buffer
        for (int i = 0; i < SMALL_BUFFER_SIZE; i++) {
            buffer[i] = i;
        }
        
        // Start non-blocking send
        MPI_Isend(buffer, SMALL_BUFFER_SIZE, MPI_INT, 1, 0, MPI_COMM_WORLD, &request);
        
        printf("Process %d: Non-blocking send posted\n", rank);
        
        // DANGEROUS: Modify buffer while send is in progress
        printf("Process %d: Modifying buffer while send is active (DANGEROUS!)\n", rank);
        
        // MPI buffer modification will be instrumented to detect:
        // - Buffer modification during active non-blocking operation
        // - Data race condition
        // - Undefined behavior risk
        
        for (int i = 0; i < SMALL_BUFFER_SIZE; i++) {
            buffer[i] = i * 1000;  // DANGEROUS: Modifying active send buffer!
        }
        
        // Wait for send completion
        MPI_Status status;
        MPI_Wait(&request, &status);
        
        printf("Process %d: Send completed\n", rank);
        
        free(buffer);
        
    } else if (rank == 1) {
        int recv_buffer[SMALL_BUFFER_SIZE];
        
        MPI_Status status;
        MPI_Recv(recv_buffer, SMALL_BUFFER_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        
        printf("Process %d: Received data - first value: %d (may be corrupted)\n", 
               rank, recv_buffer[0]);
    }
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
        printf("=== MPI Buffer Error Detection Examples ===\n");
        printf("Running with %d processes\n", size);
        printf("Note: These examples demonstrate DANGEROUS patterns!\n\n");
    }
    
    // Example 1: Buffer overflow in receive (safe to run)
    if (rank == 0) printf("--- Example 1: Buffer Overflow (Receive) ---\n");
    buffer_overflow_receive_example(rank, size);
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Example 2: Buffer underflow in send (may cause crash - commented out)
    /*
    if (rank == 0) printf("\n--- Example 2: Buffer Underflow (Send) ---\n");
    buffer_underflow_send_example(rank, size);
    MPI_Barrier(MPI_COMM_WORLD);
    */
    
    // Example 3: NULL pointer usage (will cause crash - commented out)
    /*
    if (rank == 0) printf("\n--- Example 3: NULL Pointer Usage ---\n");
    null_pointer_example(rank, size);
    MPI_Barrier(MPI_COMM_WORLD);
    */
    
    // Example 4: Uninitialized buffer (safe to run)
    if (rank == 0) printf("\n--- Example 4: Uninitialized Buffer ---\n");
    uninitialized_buffer_example(rank, size);
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Example 5: Buffer alignment issues (safe to run)
    if (rank == 0) printf("\n--- Example 5: Buffer Alignment ---\n");
    buffer_alignment_example(rank, size);
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Example 6: Buffer reuse during non-blocking operations (safe to run)
    if (rank == 0) printf("\n--- Example 6: Buffer Reuse Issues ---\n");
    buffer_reuse_example(rank, size);
    MPI_Barrier(MPI_COMM_WORLD);
    
    if (rank == 0) {
        printf("\n=== Buffer Safety Tips ===\n");
        printf("1. Always ensure receive buffers are large enough\n");
        printf("2. Never send more data than allocated in buffer\n");
        printf("3. Initialize buffers before sending\n");
        printf("4. Don't modify buffers during non-blocking operations\n");
        printf("5. Use proper buffer alignment for performance\n");
        printf("6. Check return codes and use MPI_Get_count\n");
    }
    
    MPI_Finalize();
    return EXIT_SUCCESS;
}

/*
 * Expected Sanitizer Output (Buffer Overflow):
 * 
 * [MPI Sanitizer] Pre-call: MPI_Send at buffer_overflow.c:35
 * [MPI Sanitizer]   - Buffer: 0x7f8b4c000b20, size 4000 bytes
 * [MPI Sanitizer]   - Count: 1000, Datatype: MPI_INT (4 bytes)
 * [MPI Sanitizer]   - Total send size: 4000 bytes
 * [MPI Sanitizer] Post-call: MPI_Send returned MPI_SUCCESS
 * 
 * [MPI Sanitizer] Pre-call: MPI_Recv at buffer_overflow.c:47
 * [MPI Sanitizer] ERROR: Buffer overflow risk detected!
 * [MPI Sanitizer]   - Receive buffer: 0x7f8b4c001b20, capacity 400 bytes
 * [MPI Sanitizer]   - Requested count: 100 integers (400 bytes)
 * [MPI Sanitizer]   - Incoming message: 1000 integers (4000 bytes)
 * [MPI Sanitizer]   - OVERFLOW: Message is 10x larger than buffer!
 * [MPI Sanitizer] WARNING: Only partial data will be received
 * [MPI Sanitizer] RECOMMENDATION: Increase buffer size or use MPI_Probe
 * [MPI Sanitizer] Post-call: MPI_Recv returned MPI_SUCCESS
 * [MPI Sanitizer]   - Received: 100/1000 integers (10% of message)
 * [MPI Sanitizer]   - Remaining data: TRUNCATED
 * 
 * Expected Sanitizer Output (Buffer Underflow):
 * 
 * [MPI Sanitizer] Pre-call: MPI_Send at buffer_overflow.c:85
 * [MPI Sanitizer] ERROR: Buffer underflow detected!
 * [MPI Sanitizer]   - Send buffer: 0x7f8b4c000b20, allocated size 400 bytes
 * [MPI Sanitizer]   - Requested count: 1000 integers (4000 bytes)
 * [MPI Sanitizer]   - UNDERFLOW: Trying to send 10x more than allocated!
 * [MPI Sanitizer] CRITICAL: This will read beyond buffer bounds
 * [MPI Sanitizer] RECOMMENDATION: Fix buffer allocation or send count
 * [MPI Sanitizer] Post-call: MPI_Send - SEGMENTATION FAULT RISK
 * 
 * Expected Sanitizer Output (NULL Pointer):
 * 
 * [MPI Sanitizer] Pre-call: MPI_Send at buffer_overflow.c:115
 * [MPI Sanitizer] ERROR: NULL pointer detected!
 * [MPI Sanitizer]   - Send buffer: NULL (invalid)
 * [MPI Sanitizer]   - Count: 10 integers (40 bytes requested)
 * [MPI Sanitizer] CRITICAL: NULL pointer dereference will cause crash
 * [MPI Sanitizer] RECOMMENDATION: Allocate buffer before use
 * [MPI Sanitizer] Post-call: MPI_Send - SEGMENTATION FAULT
 * 
 * Expected Sanitizer Output (Uninitialized Buffer):
 * 
 * [MPI Sanitizer] Pre-call: MPI_Send at buffer_overflow.c:145
 * [MPI Sanitizer] WARNING: Potentially uninitialized buffer detected
 * [MPI Sanitizer]   - Buffer: 0x7f8b4c000b20, size 400 bytes
 * [MPI Sanitizer]   - Memory pattern analysis: HIGH entropy (likely uninitialized)
 * [MPI Sanitizer] RECOMMENDATION: Initialize buffer before sending
 * [MPI Sanitizer] Post-call: MPI_Send returned MPI_SUCCESS
 * [MPI Sanitizer]   - Data integrity: QUESTIONABLE (uninitialized data sent)
 * 
 * Expected Sanitizer Output (Buffer Alignment):
 * 
 * [MPI Sanitizer] Pre-call: MPI_Send at buffer_overflow.c:185
 * [MPI Sanitizer] WARNING: Buffer alignment issue detected
 * [MPI Sanitizer]   - Buffer address: 0x7f8b4c000b21 (misaligned)
 * [MPI Sanitizer]   - Expected alignment: 4 bytes (for MPI_INT)
 * [MPI Sanitizer]   - Actual alignment: 1 byte
 * [MPI Sanitizer] PERFORMANCE: Misalignment may reduce transfer speed
 * [MPI Sanitizer] RECOMMENDATION: Use properly aligned buffers
 * [MPI Sanitizer] Post-call: MPI_Send returned MPI_SUCCESS
 * 
 * Expected Sanitizer Output (Buffer Reuse):
 * 
 * [MPI Sanitizer] Pre-call: MPI_Isend at buffer_overflow.c:215
 * [MPI Sanitizer]   - Non-blocking send posted
 * [MPI Sanitizer]   - Buffer tracking: ENABLED for 0x7f8b4c000b20
 * [MPI Sanitizer] Post-call: MPI_Isend returned MPI_SUCCESS
 * 
 * [MPI Sanitizer] RUNTIME ERROR: Buffer modification during active operation!
 * [MPI Sanitizer]   - Buffer: 0x7f8b4c000b20 (active in MPI_Isend)
 * [MPI Sanitizer]   - Modification detected at: buffer_overflow.c:225
 * [MPI Sanitizer]   - Operation status: IN_PROGRESS
 * [MPI Sanitizer] CRITICAL: Data race condition - undefined behavior
 * [MPI Sanitizer] RECOMMENDATION: Wait for completion before modifying buffer
 * 
 * [MPI Sanitizer] Buffer Safety Summary:
 * [MPI Sanitizer]   - Buffer errors detected: 5
 * [MPI Sanitizer]   - Critical errors: 3
 * [MPI Sanitizer]   - Warnings issued: 2
 * [MPI Sanitizer]   - Segmentation fault risks: 2
 * [MPI Sanitizer]   - Data integrity issues: 2
 */