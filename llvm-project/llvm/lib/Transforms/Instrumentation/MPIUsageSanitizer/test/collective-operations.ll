; RUN: opt %basic-test -S < %s | FileCheck %s
; RUN: opt %optimization-test -S < %s | FileCheck %s --check-prefix=OPT

; Test MPI collective operations instrumentation

target triple = "x86_64-unknown-linux-gnu"

; MPI collective function declarations
declare i32 @MPI_Bcast(i8*, i32, i32, i32, i32)
declare i32 @MPI_Allreduce(i8*, i8*, i32, i32, i32, i32)
declare i32 @MPI_Gather(i8*, i32, i32, i8*, i32, i32, i32, i32)
declare i32 @MPI_Scatter(i8*, i32, i32, i8*, i32, i32, i32, i32)
declare i32 @MPI_Barrier(i32)

; CHECK-LABEL: @test_collective_operations
; CHECK: call{{.*}}@__mpi_sanitizer_pre_bcast
; CHECK: call i32 @MPI_Bcast
; CHECK: call{{.*}}@__mpi_sanitizer_post_bcast

; OPT-LABEL: @test_collective_operations
; OPT: call{{.*}}@__mpi_sanitizer_pre_bcast
; OPT: call i32 @MPI_Bcast
; OPT: call{{.*}}@__mpi_sanitizer_post_bcast

define i32 @test_collective_operations() {
entry:
  %send_buffer = alloca [100 x i32], align 4
  %recv_buffer = alloca [100 x i32], align 4
  %result_buffer = alloca [100 x i32], align 4
  
  ; Broadcast operation
  %send_ptr = getelementptr inbounds [100 x i32], [100 x i32]* %send_buffer, i64 0, i64 0
  %send_ptr_i8 = bitcast i32* %send_ptr to i8*
  %call1 = call i32 @MPI_Bcast(i8* %send_ptr_i8, i32 100, i32 0, i32 0, i32 0)
  
  ; Allreduce operation
  %recv_ptr = getelementptr inbounds [100 x i32], [100 x i32]* %recv_buffer, i64 0, i64 0
  %recv_ptr_i8 = bitcast i32* %recv_ptr to i8*
  %result_ptr = getelementptr inbounds [100 x i32], [100 x i32]* %result_buffer, i64 0, i64 0
  %result_ptr_i8 = bitcast i32* %result_ptr to i8*
  %call2 = call i32 @MPI_Allreduce(i8* %recv_ptr_i8, i8* %result_ptr_i8, i32 100, i32 0, i32 0, i32 0)
  
  ; Gather operation
  %call3 = call i32 @MPI_Gather(i8* %send_ptr_i8, i32 25, i32 0, i8* %recv_ptr_i8, i32 25, i32 0, i32 0, i32 0)
  
  ; Scatter operation
  %call4 = call i32 @MPI_Scatter(i8* %send_ptr_i8, i32 25, i32 0, i8* %recv_ptr_i8, i32 25, i32 0, i32 0, i32 0)
  
  ; Barrier operation
  %call5 = call i32 @MPI_Barrier(i32 0)
  
  ret i32 0
}

; CHECK: declare{{.*}}@__mpi_sanitizer_pre_bcast
; CHECK: declare{{.*}}@__mpi_sanitizer_post_bcast
; CHECK: declare{{.*}}@__mpi_sanitizer_pre_allreduce
; CHECK: declare{{.*}}@__mpi_sanitizer_post_allreduce
; CHECK: declare{{.*}}@__mpi_sanitizer_pre_gather
; CHECK: declare{{.*}}@__mpi_sanitizer_post_gather
; CHECK: declare{{.*}}@__mpi_sanitizer_pre_scatter
; CHECK: declare{{.*}}@__mpi_sanitizer_post_scatter
; CHECK: declare{{.*}}@__mpi_sanitizer_pre_barrier
; CHECK: declare{{.*}}@__mpi_sanitizer_post_barrier