/**
 * @file nonblocking_comm.c
 * @brief Non-blocking MPI communication examples
 * 
 * This example demonstrates MPI_Isend, MPI_Irecv, MPI_Wait, and MPI_Test
 * operations, showing how the sanitizer tracks non-blocking requests and
 * validates proper completion handling.
 * 
 * Expected instrumentation:
 * - Request handle validation and tracking
 * - Buffer lifetime analysis for non-blocking operations
 * - Completion status verification
 * - Outstanding request detection
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1000
#define NUM_MESSAGES 5
#define TAG_BASE 100

void simulate_work(double seconds) {
    double start = MPI_Wtime();
    while (MPI_Wtime() - start < seconds) {
        // Simulate computational work
        volatile double x = 0.0;
        for (int i = 0; i < 1000; i++) {
            x += i * 0.001;
        }
    }
}

int main(int argc, char *argv[]) {
    int rank, size;
    int *send_buffers[NUM_MESSAGES];
    int *recv_buffers[NUM_MESSAGES];
    MPI_Request send_requests[NUM_MESSAGES];
    MPI_Request recv_requests[NUM_MESSAGES];
    MPI_Status statuses[NUM_MESSAGES];
    
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
    
    // Allocate buffers for multiple messages
    for (int i = 0; i < NUM_MESSAGES; i++) {
        send_buffers[i] = (int*)malloc(BUFFER_SIZE * sizeof(int));
        recv_buffers[i] = (int*)malloc(BUFFER_SIZE * sizeof(int));
        
        if (!send_buffers[i] || !recv_buffers[i]) {
            fprintf(stderr, "Process %d: Memory allocation failed\n", rank);
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
        
        // Initialize send buffers
        for (int j = 0; j < BUFFER_SIZE; j++) {
            send_buffers[i][j] = rank * 10000 + i * 1000 + j;
        }
        
        // Initialize receive buffers to detect successful receives
        memset(recv_buffers[i], -1, BUFFER_SIZE * sizeof(int));
    }
    
    printf("Process %d starting non-blocking communication examples\n", rank);
    
    // Example 1: Basic non-blocking send/receive pair
    if (rank == 0) {
        printf("\n=== Example 1: Basic Non-blocking Communication ===\n");
        
        // Post non-blocking send
        // MPI_Isend will be instrumented to validate:
        // - Buffer is valid and will remain valid until completion
        // - Request handle is valid for tracking
        // - Parameters are consistent with blocking version
        // - Buffer lifetime tracking is enabled
        
        double start_time = MPI_Wtime();
        
        int isend_result = MPI_Isend(send_buffers[0],        // buf
                                    BUFFER_SIZE,             // count
                                    MPI_INT,                 // datatype
                                    1,                       // dest
                                    TAG_BASE,                // tag
                                    MPI_COMM_WORLD,          // comm
                                    &send_requests[0]);      // request
        
        if (isend_result != MPI_SUCCESS) {
            fprintf(stderr, "Process %d: MPI_Isend failed with code %d\n", 
                    rank, isend_result);
        } else {
            printf("Process %d: Non-blocking send posted\n", rank);
        }
        
        // Simulate some computation while message is in transit
        printf("Process %d: Doing computation while send is in progress...\n", rank);
        simulate_work(0.1);  // 100ms of work
        
        // Wait for send completion
        // MPI_Wait will be instrumented to validate:
        // - Request handle is valid and outstanding
        // - Status information is properly filled
        // - Request is properly freed after completion
        
        int wait_result = MPI_Wait(&send_requests[0], &statuses[0]);
        
        double send_time = MPI_Wtime() - start_time;
        
        if (wait_result == MPI_SUCCESS) {
            printf("Process %d: Send completed in %f seconds\n", rank, send_time);
        }
        
    } else if (rank == 1) {
        // Post non-blocking receive
        
        double start_time = MPI_Wtime();
        
        int irecv_result = MPI_Irecv(recv_buffers[0],        // buf
                                    BUFFER_SIZE,             // count
                                    MPI_INT,                 // datatype
                                    0,                       // source
                                    TAG_BASE,                // tag
                                    MPI_COMM_WORLD,          // comm
                                    &recv_requests[0]);      // request
        
        if (irecv_result != MPI_SUCCESS) {
            fprintf(stderr, "Process %d: MPI_Irecv failed with code %d\n", 
                    rank, irecv_result);
        } else {
            printf("Process %d: Non-blocking receive posted\n", rank);
        }
        
        // Simulate computation
        printf("Process %d: Doing computation while receive is posted...\n", rank);
        simulate_work(0.05);  // 50ms of work
        
        // Wait for receive completion
        int wait_result = MPI_Wait(&recv_requests[0], &statuses[0]);
        
        double recv_time = MPI_Wtime() - start_time;
        
        if (wait_result == MPI_SUCCESS) {
            printf("Process %d: Receive completed in %f seconds\n", rank, recv_time);
            
            // Verify received data
            int errors = 0;
            for (int i = 0; i < 10; i++) {  // Check first 10 elements
                int expected = 0 * 10000 + 0 * 1000 + i;  // From rank 0, message 0
                if (recv_buffers[0][i] != expected) {
                    errors++;
                }
            }
            printf("Process %d: Data verification - %d errors in first 10 elements\n", 
                   rank, errors);
        }
    }
    
    MPI_Barrier(MPI_COMM_WORLD);  // Synchronize before next example
    
    // Example 2: Multiple outstanding requests
    if (rank == 0) {
        printf("\n=== Example 2: Multiple Outstanding Requests ===\n");
        
        // Post multiple non-blocking sends
        for (int i = 0; i < NUM_MESSAGES; i++) {
            int dest = (i % (size - 1)) + 1;  // Round-robin to other processes
            
            int result = MPI_Isend(send_buffers[i],
                                  BUFFER_SIZE,
                                  MPI_INT,
                                  dest,
                                  TAG_BASE + i,
                                  MPI_COMM_WORLD,
                                  &send_requests[i]);
            
            if (result == MPI_SUCCESS) {
                printf("Process %d: Posted send %d to process %d\n", rank, i, dest);
            }
        }
        
        // Use MPI_Testall to check completion status
        int all_complete = 0;
        int test_count = 0;
        
        while (!all_complete) {
            // MPI_Testall will be instrumented to validate:
            // - All request handles in array are valid
            // - Status array is properly sized
            // - Completion detection is accurate
            
            int testall_result = MPI_Testall(NUM_MESSAGES,
                                           send_requests,
                                           &all_complete,
                                           statuses);
            
            if (testall_result != MPI_SUCCESS) {
                fprintf(stderr, "Process %d: MPI_Testall failed\n", rank);
                break;
            }
            
            test_count++;
            if (!all_complete) {
                simulate_work(0.01);  // 10ms between tests
            }
        }
        
        printf("Process %d: All sends completed after %d tests\n", rank, test_count);
        
    } else {
        // Other processes receive messages
        int my_receives = 0;
        
        // Calculate how many messages this process should receive
        for (int i = 0; i < NUM_MESSAGES; i++) {
            int dest = (i % (size - 1)) + 1;
            if (dest == rank) {
                my_receives++;
            }
        }
        
        printf("Process %d: Expecting %d messages\n", rank, my_receives);
        
        // Post receives for expected messages
        int recv_index = 0;
        for (int i = 0; i < NUM_MESSAGES; i++) {
            int dest = (i % (size - 1)) + 1;
            if (dest == rank) {
                int result = MPI_Irecv(recv_buffers[recv_index],
                                      BUFFER_SIZE,
                                      MPI_INT,
                                      0,  // From process 0
                                      TAG_BASE + i,
                                      MPI_COMM_WORLD,
                                      &recv_requests[recv_index]);
                
                if (result == MPI_SUCCESS) {
                    printf("Process %d: Posted receive %d for tag %d\n", 
                           rank, recv_index, TAG_BASE + i);
                }
                recv_index++;
            }
        }
        
        // Wait for all receives to complete
        if (my_receives > 0) {
            int waitall_result = MPI_Waitall(my_receives,
                                           recv_requests,
                                           statuses);
            
            if (waitall_result == MPI_SUCCESS) {
                printf("Process %d: All %d receives completed\n", rank, my_receives);
            }
        }
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Example 3: Request cancellation (advanced)
    if (rank == 0 && size >= 3) {
        printf("\n=== Example 3: Request Cancellation ===\n");
        
        // Post a send that we'll cancel
        MPI_Request cancel_request;
        int cancel_result = MPI_Isend(send_buffers[0],
                                     BUFFER_SIZE,
                                     MPI_INT,
                                     2,  // To process 2
                                     TAG_BASE + 999,
                                     MPI_COMM_WORLD,
                                     &cancel_request);
        
        if (cancel_result == MPI_SUCCESS) {
            printf("Process %d: Posted send for cancellation test\n", rank);
            
            // Immediately cancel the request
            // MPI_Cancel will be instrumented to validate:
            // - Request handle is valid and outstanding
            // - Cancellation is properly handled
            // - Request state is updated correctly
            
            int cancel_op_result = MPI_Cancel(&cancel_request);
            
            if (cancel_op_result == MPI_SUCCESS) {
                printf("Process %d: Cancel operation completed\n", rank);
                
                // Still need to wait/test to free the request
                MPI_Status cancel_status;
                MPI_Wait(&cancel_request, &cancel_status);
                
                int cancelled;
                MPI_Test_cancelled(&cancel_status, &cancelled);
                
                printf("Process %d: Request was %s\n", 
                       rank, cancelled ? "cancelled" : "completed normally");
            }
        }
    }
    
    // Performance summary
    if (rank == 0) {
        printf("\n=== Performance Notes ===\n");
        printf("Non-blocking operations allow computation/communication overlap\n");
        printf("Multiple outstanding requests can improve bandwidth utilization\n");
        printf("Proper request management is critical for correctness\n");
    }
    
    // Cleanup
    for (int i = 0; i < NUM_MESSAGES; i++) {
        free(send_buffers[i]);
        free(recv_buffers[i]);
    }
    
    MPI_Finalize();
    return EXIT_SUCCESS;
}

/*
 * Expected Sanitizer Output (Process 0):
 * 
 * [MPI Sanitizer] Pre-call: MPI_Isend at nonblocking_comm.c:75
 * [MPI Sanitizer]   - Non-blocking operation: MPI_Isend detected
 * [MPI Sanitizer]   - Buffer: 0x7f8b4c000b20, size 4000 bytes
 * [MPI Sanitizer]   - Buffer lifetime tracking: ENABLED
 * [MPI Sanitizer]   - Count: 1000, Datatype: MPI_INT (4 bytes)
 * [MPI Sanitizer]   - Destination: 1 (valid in communicator)
 * [MPI Sanitizer]   - Tag: 100 (within valid range)
 * [MPI Sanitizer]   - Request handle: 0x7fff5fbff790 (valid)
 * [MPI Sanitizer] Post-call: MPI_Isend returned MPI_SUCCESS
 * [MPI Sanitizer]   - Request registered for tracking
 * [MPI Sanitizer]   - Outstanding requests: 1
 * 
 * [MPI Sanitizer] Pre-call: MPI_Wait at nonblocking_comm.c:95
 * [MPI Sanitizer]   - Request completion: MPI_Wait detected
 * [MPI Sanitizer]   - Request handle: 0x7fff5fbff790 (valid, outstanding)
 * [MPI Sanitizer]   - Status pointer: 0x7fff5fbff780 (valid)
 * [MPI Sanitizer] Post-call: MPI_Wait returned MPI_SUCCESS
 * [MPI Sanitizer]   - Request completed successfully
 * [MPI Sanitizer]   - Buffer lifetime tracking: DISABLED
 * [MPI Sanitizer]   - Outstanding requests: 0
 * 
 * [MPI Sanitizer] Pre-call: MPI_Isend at nonblocking_comm.c:125 (multiple)
 * [MPI Sanitizer]   - Multiple non-blocking sends detected
 * [MPI Sanitizer]   - Request 0: dest=1, tag=100
 * [MPI Sanitizer]   - Request 1: dest=2, tag=101
 * [MPI Sanitizer]   - Request 2: dest=3, tag=102
 * [MPI Sanitizer]   - Request 3: dest=1, tag=103
 * [MPI Sanitizer]   - Request 4: dest=2, tag=104
 * [MPI Sanitizer] Post-call: All MPI_Isend calls successful
 * [MPI Sanitizer]   - Outstanding requests: 5
 * 
 * [MPI Sanitizer] Pre-call: MPI_Testall at nonblocking_comm.c:145
 * [MPI Sanitizer]   - Request testing: MPI_Testall detected
 * [MPI Sanitizer]   - Request array: 5 handles (all valid)
 * [MPI Sanitizer]   - Completion flag pointer: 0x7fff5fbff78c (valid)
 * [MPI Sanitizer] Post-call: MPI_Testall returned MPI_SUCCESS
 * [MPI Sanitizer]   - Completion status: NOT_COMPLETE (iteration 1)
 * 
 * [MPI Sanitizer] Pre-call: MPI_Testall at nonblocking_comm.c:145
 * [MPI Sanitizer] Post-call: MPI_Testall returned MPI_SUCCESS
 * [MPI Sanitizer]   - Completion status: COMPLETE (iteration 15)
 * [MPI Sanitizer]   - All requests completed successfully
 * [MPI Sanitizer]   - Outstanding requests: 0
 * 
 * [MPI Sanitizer] Pre-call: MPI_Cancel at nonblocking_comm.c:215
 * [MPI Sanitizer]   - Request cancellation: MPI_Cancel detected
 * [MPI Sanitizer]   - Request handle: 0x7fff5fbff788 (valid, outstanding)
 * [MPI Sanitizer] Post-call: MPI_Cancel returned MPI_SUCCESS
 * [MPI Sanitizer]   - Cancellation request submitted
 * [MPI Sanitizer]   - Note: Request still needs Wait/Test to complete
 * 
 * Expected Sanitizer Output (Process 1):
 * 
 * [MPI Sanitizer] Pre-call: MPI_Irecv at nonblocking_comm.c:110
 * [MPI Sanitizer]   - Non-blocking operation: MPI_Irecv detected
 * [MPI Sanitizer]   - Buffer: 0x7f8b4c001b20, capacity 4000 bytes
 * [MPI Sanitizer]   - Count: 1000, Datatype: MPI_INT (4 bytes)
 * [MPI Sanitizer]   - Source: 0 (valid in communicator)
 * [MPI Sanitizer]   - Tag: 100 (within valid range)
 * [MPI Sanitizer] Post-call: MPI_Irecv returned MPI_SUCCESS
 * [MPI Sanitizer]   - Receive request posted successfully
 * [MPI Sanitizer]   - Outstanding requests: 1
 * 
 * [MPI Sanitizer] Pre-call: MPI_Wait at nonblocking_comm.c:125
 * [MPI Sanitizer] Post-call: MPI_Wait returned MPI_SUCCESS
 * [MPI Sanitizer]   - Message received: 4000 bytes from rank 0, tag 100
 * [MPI Sanitizer]   - Buffer utilization: 100% (4000/4000 bytes)
 * [MPI Sanitizer]   - Outstanding requests: 0
 * 
 * [MPI Sanitizer] Performance Summary for Process 0:
 * [MPI Sanitizer]   - Total non-blocking operations: 7
 * [MPI Sanitizer]   - Peak outstanding requests: 5
 * [MPI Sanitizer]   - Average request lifetime: 0.045 seconds
 * [MPI Sanitizer]   - Communication/computation overlap: 67%
 */