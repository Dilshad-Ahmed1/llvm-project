; REQUIRES: performance-tests
; RUN: opt %profiling-test -S < %s | FileCheck %s --check-prefix=PROFILING
; RUN: opt %optimization-test %profiling-test -S < %s | FileCheck %s --check-prefix=OPT-PROFILING

; Test performance validation and profiling integration

target triple = "x86_64-unknown-linux-gnu"

declare i32 @MPI_Send(i8*, i32, i32, i32, i32, i32)
declare i32 @MPI_Allreduce(i8*, i8*, i32, i32, i32, i32)

; PROFILING-LABEL: @performance_test_function
; PROFILING: call{{.*}}@__mpi_sanitizer_start_profiling
; PROFILING: call{{.*}}@__mpi_sanitizer_pre_send
; PROFILING: call i32 @MPI_Send
; PROFILING: call{{.*}}@__mpi_sanitizer_post_send
; PROFILING: call{{.*}}@__mpi_sanitizer_end_profiling

; OPT-PROFILING-LABEL: @performance_test_function
; OPT-PROFILING: call{{.*}}@__mpi_sanitizer_start_profiling
; OPT-PROFILING: call{{.*}}@__mpi_sanitizer_pre_send
; OPT-PROFILING: call i32 @MPI_Send
; OPT-PROFILING: call{{.*}}@__mpi_sanitizer_post_send
; OPT-PROFILING: call{{.*}}@__mpi_sanitizer_end_profiling

define i32 @performance_test_function() {
entry:
  %send_buffer = alloca [1000 x i32], align 4
  %recv_buffer = alloca [1000 x i32], align 4
  
  %send_ptr = getelementptr inbounds [1000 x i32], [1000 x i32]* %send_buffer, i64 0, i64 0
  %send_ptr_i8 = bitcast i32* %send_ptr to i8*
  %recv_ptr = getelementptr inbounds [1000 x i32], [1000 x i32]* %recv_buffer, i64 0, i64 0
  %recv_ptr_i8 = bitcast i32* %recv_ptr to i8*
  
  ; Performance-critical MPI operations
  br label %communication_loop
  
communication_loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %communication_loop ]
  
  ; Point-to-point communication
  %dest = and i32 %i, 3  ; Destination rank (0-3)
  %tag = add i32 %i, 100  ; Unique tag
  %call1 = call i32 @MPI_Send(i8* %send_ptr_i8, i32 1000, i32 0, i32 %dest, i32 %tag, i32 0)
  
  ; Collective operation every 10 iterations
  %mod = urem i32 %i, 10
  %is_collective = icmp eq i32 %mod, 0
  br i1 %is_collective, label %collective_op, label %continue_loop
  
collective_op:
  %call2 = call i32 @MPI_Allreduce(i8* %send_ptr_i8, i8* %recv_ptr_i8, i32 1000, i32 0, i32 0, i32 0)
  br label %continue_loop
  
continue_loop:
  %i.next = add i32 %i, 1
  %cond = icmp slt i32 %i.next, 50
  br i1 %cond, label %communication_loop, label %exit
  
exit:
  ret i32 0
}

; Test scalability with varying message sizes
; PROFILING-LABEL: @scalability_test_function
define i32 @scalability_test_function(i32 %message_size) {
entry:
  %max_buffer = alloca [10000 x i8], align 1
  %buffer_ptr = getelementptr inbounds [10000 x i8], [10000 x i8]* %max_buffer, i64 0, i64 0
  
  ; Test different message sizes
  %small_call = call i32 @MPI_Send(i8* %buffer_ptr, i32 100, i32 0, i32 1, i32 1, i32 0)
  %medium_call = call i32 @MPI_Send(i8* %buffer_ptr, i32 1000, i32 0, i32 1, i32 2, i32 0)
  %large_call = call i32 @MPI_Send(i8* %buffer_ptr, i32 %message_size, i32 0, i32 1, i32 3, i32 0)
  
  ret i32 0
}

; Test memory usage patterns
; PROFILING-LABEL: @memory_usage_test
define i32 @memory_usage_test() {
entry:
  ; Multiple buffers to test memory overhead
  %buffer1 = alloca [1000 x i8], align 1
  %buffer2 = alloca [2000 x i8], align 1
  %buffer3 = alloca [4000 x i8], align 1
  
  %ptr1 = getelementptr inbounds [1000 x i8], [1000 x i8]* %buffer1, i64 0, i64 0
  %ptr2 = getelementptr inbounds [2000 x i8], [2000 x i8]* %buffer2, i64 0, i64 0
  %ptr3 = getelementptr inbounds [4000 x i8], [4000 x i8]* %buffer3, i64 0, i64 0
  
  ; Concurrent MPI operations with different buffers
  %call1 = call i32 @MPI_Send(i8* %ptr1, i32 1000, i32 0, i32 1, i32 10, i32 0)
  %call2 = call i32 @MPI_Send(i8* %ptr2, i32 2000, i32 0, i32 2, i32 20, i32 0)
  %call3 = call i32 @MPI_Send(i8* %ptr3, i32 4000, i32 0, i32 3, i32 30, i32 0)
  
  ret i32 0
}

; PROFILING: declare{{.*}}@__mpi_sanitizer_start_profiling
; PROFILING: declare{{.*}}@__mpi_sanitizer_end_profiling
; PROFILING: declare{{.*}}@__mpi_sanitizer_record_performance_metric