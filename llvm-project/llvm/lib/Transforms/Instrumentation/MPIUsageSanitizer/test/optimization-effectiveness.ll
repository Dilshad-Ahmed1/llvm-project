; REQUIRES: mpi-sanitizer-optimization
; RUN: opt %basic-test -S < %s | FileCheck %s --check-prefix=BASELINE
; RUN: opt %optimization-test -S < %s | FileCheck %s --check-prefix=OPTIMIZED

; Test that optimization reduces instrumentation overhead for hot paths

target triple = "x86_64-unknown-linux-gnu"

declare i32 @MPI_Send(i8*, i32, i32, i32, i32, i32)
declare i32 @MPI_Recv(i8*, i32, i32, i32, i32, i32, i32*)

; Hot path function with many MPI calls
; BASELINE-LABEL: @hot_path_function
; BASELINE: call{{.*}}@__mpi_sanitizer_pre_send
; BASELINE: call i32 @MPI_Send
; BASELINE: call{{.*}}@__mpi_sanitizer_post_send

; OPTIMIZED-LABEL: @hot_path_function
; Optimized version should have fewer or more efficient hooks
; OPTIMIZED: call{{.*}}@__mpi_sanitizer_pre_send
; OPTIMIZED: call i32 @MPI_Send
; OPTIMIZED: call{{.*}}@__mpi_sanitizer_post_send

define i32 @hot_path_function() {
entry:
  %buffer = alloca [1000 x i8], align 1
  %buffer_ptr = getelementptr inbounds [1000 x i8], [1000 x i8]* %buffer, i64 0, i64 0
  
  ; Loop with many MPI calls (hot path)
  br label %loop
  
loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  
  ; Send operation in loop
  %call1 = call i32 @MPI_Send(i8* %buffer_ptr, i32 100, i32 0, i32 1, i32 %i, i32 0)
  
  ; Receive operation in loop
  %call2 = call i32 @MPI_Recv(i8* %buffer_ptr, i32 100, i32 0, i32 1, i32 %i, i32 0, i32* null)
  
  %i.next = add i32 %i, 1
  %cond = icmp slt i32 %i.next, 100
  br i1 %cond, label %loop, label %exit
  
exit:
  ret i32 0
}

; Cold path function with few MPI calls
; Both versions should have similar instrumentation
; BASELINE-LABEL: @cold_path_function
; OPTIMIZED-LABEL: @cold_path_function

define i32 @cold_path_function() {
entry:
  %buffer = alloca [100 x i8], align 1
  %buffer_ptr = getelementptr inbounds [100 x i8], [100 x i8]* %buffer, i64 0, i64 0
  
  ; Single MPI call (cold path)
  %call = call i32 @MPI_Send(i8* %buffer_ptr, i32 100, i32 0, i32 1, i32 42, i32 0)
  
  ret i32 0
}

; Test function with complex control flow
; BASELINE-LABEL: @complex_control_flow
; OPTIMIZED-LABEL: @complex_control_flow

define i32 @complex_control_flow(i32 %condition) {
entry:
  %buffer = alloca [100 x i8], align 1
  %buffer_ptr = getelementptr inbounds [100 x i8], [100 x i8]* %buffer, i64 0, i64 0
  
  %cmp = icmp sgt i32 %condition, 0
  br i1 %cmp, label %then_branch, label %else_branch
  
then_branch:
  %call1 = call i32 @MPI_Send(i8* %buffer_ptr, i32 50, i32 0, i32 1, i32 1, i32 0)
  br label %merge
  
else_branch:
  %call2 = call i32 @MPI_Recv(i8* %buffer_ptr, i32 50, i32 0, i32 1, i32 2, i32 0, i32* null)
  br label %merge
  
merge:
  %result = phi i32 [ %call1, %then_branch ], [ %call2, %else_branch ]
  ret i32 %result
}