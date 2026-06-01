/**
 * @file hello_world.c
 * @brief Basic MPI Hello World program demonstrating initialization and finalization
 * 
 * This example shows the simplest MPI program pattern and demonstrates how the
 * MPI Usage Sanitizer instruments MPI_Init and MPI_Finalize calls.
 * 
 * Expected instrumentation:
 * - Pre-call hook before MPI_Init with parameter validation
 * - Post-call hook after MPI_Init with return code checking
 * - Pre-call hook before MPI_Finalize
 * - Post-call hook after MPI_Finalize with cleanup validation
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int rank, size;
    int provided;
    
    // Initialize MPI environment
    // Sanitizer will instrument this call to validate:
    // - argc/argv parameters are valid
    // - MPI is not already initialized
    // - Return code indicates success
    int init_result = MPI_Init(&argc, &argv);
    if (init_result != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Init failed with error code %d\n", init_result);
        return EXIT_FAILURE;
    }
    
    // Get process rank and total number of processes
    // These calls will be instrumented to validate:
    // - MPI is properly initialized
    // - Output parameters are valid pointers
    // - Communicator (MPI_COMM_WORLD) is valid
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Print hello message from each process
    printf("Hello from process %d of %d\n", rank, size);
    
    // Synchronize all processes before finalization
    // Sanitizer will instrument to detect potential deadlocks
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Finalize MPI environment
    // Sanitizer will instrument this call to validate:
    // - MPI is still initialized
    // - No outstanding requests or operations
    // - Proper cleanup of resources
    int finalize_result = MPI_Finalize();
    if (finalize_result != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Finalize failed with error code %d\n", finalize_result);
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

/*
 * Expected Sanitizer Output:
 * 
 * [MPI Sanitizer] Pre-call: MPI_Init at hello_world.c:25
 * [MPI Sanitizer]   - Validating argc/argv parameters
 * [MPI Sanitizer]   - Checking MPI initialization state
 * [MPI Sanitizer] Post-call: MPI_Init returned MPI_SUCCESS
 * [MPI Sanitizer]   - MPI environment successfully initialized
 * 
 * [MPI Sanitizer] Pre-call: MPI_Comm_rank at hello_world.c:32
 * [MPI Sanitizer]   - Validating communicator: MPI_COMM_WORLD
 * [MPI Sanitizer]   - Validating output parameter: rank
 * [MPI Sanitizer] Post-call: MPI_Comm_rank returned MPI_SUCCESS
 * 
 * [MPI Sanitizer] Pre-call: MPI_Comm_size at hello_world.c:33
 * [MPI Sanitizer]   - Validating communicator: MPI_COMM_WORLD
 * [MPI Sanitizer]   - Validating output parameter: size
 * [MPI Sanitizer] Post-call: MPI_Comm_size returned MPI_SUCCESS
 * 
 * [MPI Sanitizer] Pre-call: MPI_Barrier at hello_world.c:39
 * [MPI Sanitizer]   - Validating communicator: MPI_COMM_WORLD
 * [MPI Sanitizer]   - Checking for potential deadlock conditions
 * [MPI Sanitizer] Post-call: MPI_Barrier returned MPI_SUCCESS
 * [MPI Sanitizer]   - Synchronization completed successfully
 * 
 * [MPI Sanitizer] Pre-call: MPI_Finalize at hello_world.c:46
 * [MPI Sanitizer]   - Checking for outstanding requests
 * [MPI Sanitizer]   - Validating MPI state for finalization
 * [MPI Sanitizer] Post-call: MPI_Finalize returned MPI_SUCCESS
 * [MPI Sanitizer]   - MPI environment successfully finalized
 */