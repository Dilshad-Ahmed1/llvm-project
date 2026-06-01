!
! @file hello_world.f90
! @brief Basic Fortran MPI Hello World program
! 
! This example demonstrates MPI usage in Fortran and shows how the
! MPI Usage Sanitizer handles Fortran name mangling and parameter
! passing conventions.
! 
! Expected instrumentation:
! - Fortran name mangling detection and handling
! - Fortran-specific parameter passing validation
! - INTEGER vs C int compatibility checking
! - Error code handling in Fortran style
!

program hello_world_fortran
    use mpi
    implicit none
    
    integer :: rank, size, ierr
    integer :: provided
    character(len=100) :: processor_name
    integer :: name_length
    
    ! Initialize MPI environment
    ! The sanitizer will instrument this call to validate:
    ! - Fortran name mangling (mpi_init_ or similar)
    ! - INTEGER parameter compatibility with C int
    ! - Error code handling in Fortran convention
    call MPI_INIT(ierr)
    if (ierr /= MPI_SUCCESS) then
        print *, 'MPI_INIT failed with error code:', ierr
        stop 1
    end if
    
    ! Get process rank and total number of processes
    ! These calls will be instrumented to validate:
    ! - Fortran INTEGER vs C int parameter passing
    ! - Communicator handle compatibility
    ! - Output parameter validation
    call MPI_COMM_RANK(MPI_COMM_WORLD, rank, ierr)
    if (ierr /= MPI_SUCCESS) then
        print *, 'MPI_COMM_RANK failed with error code:', ierr
        call MPI_ABORT(MPI_COMM_WORLD, ierr, ierr)
    end if
    
    call MPI_COMM_SIZE(MPI_COMM_WORLD, size, ierr)
    if (ierr /= MPI_SUCCESS) then
        print *, 'MPI_COMM_SIZE failed with error code:', ierr
        call MPI_ABORT(MPI_COMM_WORLD, ierr, ierr)
    end if
    
    ! Get processor name (Fortran string handling)
    call MPI_GET_PROCESSOR_NAME(processor_name, name_length, ierr)
    if (ierr == MPI_SUCCESS) then
        print '(A,I0,A,I0,A,A)', 'Hello from Fortran process ', rank, &
              ' of ', size, ' on ', processor_name(1:name_length)
    else
        print '(A,I0,A,I0)', 'Hello from Fortran process ', rank, ' of ', size
    end if
    
    ! Demonstrate Fortran array operations
    if (size >= 2) then
        call fortran_array_communication(rank, size)
    end if
    
    ! Synchronize all processes before finalization
    call MPI_BARRIER(MPI_COMM_WORLD, ierr)
    if (ierr /= MPI_SUCCESS) then
        print *, 'MPI_BARRIER failed with error code:', ierr
    end if
    
    ! Finalize MPI environment
    call MPI_FINALIZE(ierr)
    if (ierr /= MPI_SUCCESS) then
        print *, 'MPI_FINALIZE failed with error code:', ierr
        stop 1
    end if
    
end program hello_world_fortran

! Subroutine to demonstrate Fortran array communication
subroutine fortran_array_communication(rank, size)
    use mpi
    implicit none
    
    integer, intent(in) :: rank, size
    integer, parameter :: ARRAY_SIZE = 10
    integer :: send_array(ARRAY_SIZE)
    integer :: recv_array(ARRAY_SIZE)
    integer :: ierr, i
    integer :: status(MPI_STATUS_SIZE)
    
    ! Initialize send array with Fortran 1-based indexing
    do i = 1, ARRAY_SIZE
        send_array(i) = rank * 100 + i
    end do
    
    if (rank == 0) then
        print *, 'Fortran process 0: Sending array to process 1'
        print '(A,10I4)', 'Send array: ', (send_array(i), i=1,ARRAY_SIZE)
        
        ! Send array to process 1
        ! The sanitizer will instrument this to validate:
        ! - Fortran array descriptor handling
        ! - Array bounds and size validation
        ! - Fortran vs C array layout compatibility
        call MPI_SEND(send_array, ARRAY_SIZE, MPI_INTEGER, 1, 42, &
                     MPI_COMM_WORLD, ierr)
        
        if (ierr /= MPI_SUCCESS) then
            print *, 'MPI_SEND failed with error code:', ierr
        end if
        
        ! Receive array back from process 1
        call MPI_RECV(recv_array, ARRAY_SIZE, MPI_INTEGER, 1, 43, &
                     MPI_COMM_WORLD, status, ierr)
        
        if (ierr == MPI_SUCCESS) then
            print '(A,10I4)', 'Recv array: ', (recv_array(i), i=1,ARRAY_SIZE)
        else
            print *, 'MPI_RECV failed with error code:', ierr
        end if
        
    else if (rank == 1) then
        ! Receive array from process 0
        call MPI_RECV(recv_array, ARRAY_SIZE, MPI_INTEGER, 0, 42, &
                     MPI_COMM_WORLD, status, ierr)
        
        if (ierr == MPI_SUCCESS) then
            print *, 'Fortran process 1: Received array from process 0'
            print '(A,10I4)', 'Recv array: ', (recv_array(i), i=1,ARRAY_SIZE)
            
            ! Modify array and send back
            do i = 1, ARRAY_SIZE
                recv_array(i) = recv_array(i) + 1000
            end do
            
            print '(A,10I4)', 'Modified:   ', (recv_array(i), i=1,ARRAY_SIZE)
            
            call MPI_SEND(recv_array, ARRAY_SIZE, MPI_INTEGER, 0, 43, &
                         MPI_COMM_WORLD, ierr)
            
            if (ierr /= MPI_SUCCESS) then
                print *, 'MPI_SEND failed with error code:', ierr
            end if
        else
            print *, 'MPI_RECV failed with error code:', ierr
        end if
    end if
    
end subroutine fortran_array_communication

!
! Expected Sanitizer Output (Fortran-specific):
! 
! [MPI Sanitizer] Pre-call: mpi_init_ at hello_world.f90:25
! [MPI Sanitizer]   - Fortran binding detected
! [MPI Sanitizer]   - Name mangling: mpi_init_ (GNU Fortran convention)
! [MPI Sanitizer]   - Parameter: ierr (INTEGER, passed by reference)
! [MPI Sanitizer]   - Fortran error handling: ENABLED
! [MPI Sanitizer] Post-call: mpi_init_ returned MPI_SUCCESS
! [MPI Sanitizer]   - Fortran INTEGER ierr set to 0
! 
! [MPI Sanitizer] Pre-call: mpi_comm_rank_ at hello_world.f90:33
! [MPI Sanitizer]   - Fortran binding: MPI_COMM_RANK
! [MPI Sanitizer]   - Communicator: MPI_COMM_WORLD (Fortran INTEGER handle)
! [MPI Sanitizer]   - Output parameter: rank (INTEGER, by reference)
! [MPI Sanitizer]   - Error parameter: ierr (INTEGER, by reference)
! [MPI Sanitizer] Post-call: mpi_comm_rank_ returned MPI_SUCCESS
! [MPI Sanitizer]   - Fortran rank variable updated: 0
! 
! [MPI Sanitizer] Pre-call: mpi_comm_size_ at hello_world.f90:39
! [MPI Sanitizer]   - Fortran binding: MPI_COMM_SIZE
! [MPI Sanitizer] Post-call: mpi_comm_size_ returned MPI_SUCCESS
! [MPI Sanitizer]   - Fortran size variable updated: 4
! 
! [MPI Sanitizer] Pre-call: mpi_get_processor_name_ at hello_world.f90:46
! [MPI Sanitizer]   - Fortran string parameter detected
! [MPI Sanitizer]   - String buffer: CHARACTER(100), length 100
! [MPI Sanitizer]   - Length parameter: name_length (INTEGER, by reference)
! [MPI Sanitizer]   - Fortran string handling: ENABLED
! [MPI Sanitizer] Post-call: mpi_get_processor_name_ returned MPI_SUCCESS
! [MPI Sanitizer]   - Processor name: "compute-node-01" (length: 14)
! [MPI Sanitizer]   - Fortran string properly null-padded
! 
! [MPI Sanitizer] Pre-call: mpi_send_ at hello_world.f90:85
! [MPI Sanitizer]   - Fortran array communication detected
! [MPI Sanitizer]   - Array: send_array, INTEGER(10), 1-based indexing
! [MPI Sanitizer]   - Array descriptor analysis: PASSED
! [MPI Sanitizer]   - Count: 10 (matches array size)
! [MPI Sanitizer]   - Datatype: MPI_INTEGER (Fortran INTEGER)
! [MPI Sanitizer]   - Destination: 1, Tag: 42
! [MPI Sanitizer] Post-call: mpi_send_ returned MPI_SUCCESS
! [MPI Sanitizer]   - Fortran array data sent successfully
! 
! [MPI Sanitizer] Pre-call: mpi_recv_ at hello_world.f90:93
! [MPI Sanitizer]   - Fortran array receive detected
! [MPI Sanitizer]   - Array: recv_array, INTEGER(10), 1-based indexing
! [MPI Sanitizer]   - Status array: INTEGER(MPI_STATUS_SIZE)
! [MPI Sanitizer] Post-call: mpi_recv_ returned MPI_SUCCESS
! [MPI Sanitizer]   - Message received: 10 integers from rank 1, tag 43
! [MPI Sanitizer]   - Fortran status array updated
! 
! [MPI Sanitizer] Fortran Binding Summary:
! [MPI Sanitizer]   - Name mangling convention: GNU Fortran (trailing underscore)
! [MPI Sanitizer]   - Parameter passing: By reference (Fortran standard)
! [MPI Sanitizer]   - String handling: Fortran CHARACTER type
! [MPI Sanitizer]   - Array handling: 1-based indexing, descriptor analysis
! [MPI Sanitizer]   - Error handling: Fortran INTEGER error codes
! [MPI Sanitizer]   - Compatibility: C/Fortran interoperability verified
!