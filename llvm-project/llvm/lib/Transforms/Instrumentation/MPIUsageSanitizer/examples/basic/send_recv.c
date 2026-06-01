/**
 * @file send_recv.c
 * @brief Basic point-to-point communication example
 * 
 * This example demonstrates simple send/receive operations and shows how the
 * MPI Usage Sanitizer instruments point-to-point communication calls.
 * 
 * Expected instrumentation:
 * - Buffer validation for send/receive operations
 * - Message size and datatype consistency checking
 * - Tag and communicator validation
 * - Deadlock detection for blocking operations
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MESSAGE_SIZE 100
#define TAG 42

int main(int argc, char *argv[]) {
    int rank, size;
    char send_buffer[MESSAGE_SIZE];
    char recv_buffer[MESSAGE_SIZE];
    MPI_Status status;
    
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
        // Process 0 sends a message to process 1
        snprintf(send_buffer, MESSAGE_SIZE, "Hello from process %d!", rank);
        
        // MPI_Send will be instrumented to validate:
        // - Buffer pointer is valid and accessible
        // - Count matches buffer size
        // - Datatype is appropriate for buffer
        // - Destination rank is valid
        // - Tag is within valid range
        // - Communicator is valid
        printf("Process %d sending: '%s'\n", rank, send_buffer);
        
        int send_result = MPI_Send(send_buffer, 
                                  strlen(send_buffer) + 1,  // Include null terminator
                                  MPI_CHAR, 
                                  1,                         // destination rank
                                  TAG, 
                                  MPI_COMM_WORLD);
        
        if (send_result != MPI_SUCCESS) {
            fprintf(stderr, "MPI_Send failed with error code %d\n", send_result);
        }
        
        // Now receive a reply from process 1
        int recv_result = MPI_Recv(recv_buffer, 
                                  MESSAGE_SIZE, 
                                  MPI_CHAR, 
                                  1,                         // source rank
                                  TAG, 
                                  MPI_COMM_WORLD, 
                                  &status);
        
        if (recv_result == MPI_SUCCESS) {
            printf("Process %d received: '%s'\n", rank, recv_buffer);
            
            // Validate message metadata
            int received_count;
            MPI_Get_count(&status, MPI_CHAR, &received_count);
            printf("Process %d received %d characters from process %d with tag %d\n",
                   rank, received_count, status.MPI_SOURCE, status.MPI_TAG);
        } else {
            fprintf(stderr, "MPI_Recv failed with error code %d\n", recv_result);
        }
        
    } else if (rank == 1) {
        // Process 1 receives a message from process 0
        
        // MPI_Recv will be instrumented to validate:
        // - Buffer pointer is valid and has sufficient space
        // - Count is positive and reasonable
        // - Datatype matches expected data
        // - Source rank is valid (or MPI_ANY_SOURCE)
        // - Tag matches sender or is MPI_ANY_TAG
        // - Status pointer is valid
        int recv_result = MPI_Recv(recv_buffer, 
                                  MESSAGE_SIZE, 
                                  MPI_CHAR, 
                                  0,                         // source rank
                                  TAG, 
                                  MPI_COMM_WORLD, 
                                  &status);
        
        if (recv_result == MPI_SUCCESS) {
            printf("Process %d received: '%s'\n", rank, recv_buffer);
            
            // Send a reply back to process 0
            snprintf(send_buffer, MESSAGE_SIZE, "Reply from process %d!", rank);
            printf("Process %d sending reply: '%s'\n", rank, send_buffer);
            
            int send_result = MPI_Send(send_buffer, 
                                      strlen(send_buffer) + 1, 
                                      MPI_CHAR, 
                                      0,                     // destination rank
                                      TAG, 
                                      MPI_COMM_WORLD);
            
            if (send_result != MPI_SUCCESS) {
                fprintf(stderr, "MPI_Send failed with error code %d\n", send_result);
            }
        } else {
            fprintf(stderr, "MPI_Recv failed with error code %d\n", recv_result);
        }
    }
    
    // Synchronize before finalization
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    
    return EXIT_SUCCESS;
}

/*
 * Expected Sanitizer Output (Process 0):
 * 
 * [MPI Sanitizer] Pre-call: MPI_Send at send_recv.c:52
 * [MPI Sanitizer]   - Buffer validation: 0x7fff5fbff8a0, size 100 bytes
 * [MPI Sanitizer]   - Count: 22, Datatype: MPI_CHAR (1 byte)
 * [MPI Sanitizer]   - Total message size: 22 bytes
 * [MPI Sanitizer]   - Destination rank: 1 (valid in communicator)
 * [MPI Sanitizer]   - Tag: 42 (within valid range)
 * [MPI Sanitizer]   - Communicator: MPI_COMM_WORLD (valid)
 * [MPI Sanitizer] Post-call: MPI_Send returned MPI_SUCCESS
 * [MPI Sanitizer]   - Message sent successfully
 * 
 * [MPI Sanitizer] Pre-call: MPI_Recv at send_recv.c:62
 * [MPI Sanitizer]   - Buffer validation: 0x7fff5fbff7a0, capacity 100 bytes
 * [MPI Sanitizer]   - Count: 100, Datatype: MPI_CHAR (1 byte)
 * [MPI Sanitizer]   - Maximum message size: 100 bytes
 * [MPI Sanitizer]   - Source rank: 1 (valid in communicator)
 * [MPI Sanitizer]   - Tag: 42 (within valid range)
 * [MPI Sanitizer]   - Status pointer: 0x7fff5fbff790 (valid)
 * [MPI Sanitizer] Post-call: MPI_Recv returned MPI_SUCCESS
 * [MPI Sanitizer]   - Message received: 23 bytes from rank 1, tag 42
 * [MPI Sanitizer]   - Buffer usage: 23/100 bytes (23%)
 * 
 * Expected Sanitizer Output (Process 1):
 * 
 * [MPI Sanitizer] Pre-call: MPI_Recv at send_recv.c:82
 * [MPI Sanitizer]   - Buffer validation: 0x7fff5fbff7a0, capacity 100 bytes
 * [MPI Sanitizer]   - Count: 100, Datatype: MPI_CHAR (1 byte)
 * [MPI Sanitizer]   - Source rank: 0 (valid in communicator)
 * [MPI Sanitizer]   - Tag: 42 (within valid range)
 * [MPI Sanitizer] Post-call: MPI_Recv returned MPI_SUCCESS
 * [MPI Sanitizer]   - Message received: 22 bytes from rank 0, tag 42
 * 
 * [MPI Sanitizer] Pre-call: MPI_Send at send_recv.c:95
 * [MPI Sanitizer]   - Buffer validation: 0x7fff5fbff8a0, size 100 bytes
 * [MPI Sanitizer]   - Count: 23, Datatype: MPI_CHAR (1 byte)
 * [MPI Sanitizer]   - Destination rank: 0 (valid in communicator)
 * [MPI Sanitizer] Post-call: MPI_Send returned MPI_SUCCESS
 */