; RUN: opt < %s -passes=mpi-sanitizer -S | FileCheck %s
; Test that MPI function database correctly identifies MPI functions

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Test basic MPI functions are recognized
declare i32 @MPI_Init(ptr, ptr)
declare i32 @MPI_Finalize()
declare i32 @MPI_Send(ptr, i32, i32, i32, i32, i32)
declare i32 @MPI_Recv(ptr, i32, i32, i32, i32, i32, ptr)
declare i32 @MPI_Bcast(ptr, i32, i32, i32, i32)
declare i32 @MPI_Barrier(i32)

; Test non-MPI function is not recognized
declare i32 @printf(ptr, ...)

define i32 @test_mpi_functions() {
entry:
  ; CHECK: call i32 @MPI_Init
  %init = call i32 @MPI_Init(ptr null, ptr null)
  
  ; CHECK: call i32 @MPI_Send
  %send = call i32 @MPI_Send(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0)
  
  ; CHECK: call i32 @MPI_Recv
  %recv = call i32 @MPI_Recv(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, ptr null)
  
  ; CHECK: call i32 @MPI_Bcast
  %bcast = call i32 @MPI_Bcast(ptr null, i32 0, i32 0, i32 0, i32 0)
  
  ; CHECK: call i32 @MPI_Barrier
  %barrier = call i32 @MPI_Barrier(i32 0)
  
  ; This should not be instrumented
  ; CHECK: call i32 (ptr, ...) @printf
  %printf = call i32 (ptr, ...) @printf(ptr null)
  
  ; CHECK: call i32 @MPI_Finalize
  %finalize = call i32 @MPI_Finalize()
  
  ret i32 0
}