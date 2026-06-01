; RUN: opt < %s -passes=mpi-sanitizer -mpi-sanitizer-level=lightweight -S | FileCheck %s --check-prefix=LIGHTWEIGHT
; RUN: opt < %s -passes=mpi-sanitizer -mpi-sanitizer-level=full -S | FileCheck %s --check-prefix=FULL
; RUN: opt < %s -passes=mpi-sanitizer -mpi-sanitizer-disable-types=environment -S | FileCheck %s --check-prefix=DISABLE_ENV

; Test policy controls for MPI operation categories and instrumentation modes

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function declarations for MPI functions
declare i32 @MPI_Init(ptr, ptr)
declare i32 @MPI_Send(ptr, i32, i32, i32, i32, i32)
declare i32 @MPI_Recv(ptr, i32, i32, i32, i32, i32, ptr)
declare i32 @MPI_Bcast(ptr, i32, i32, i32, i32)
declare i32 @MPI_Barrier(i32)
declare i32 @MPI_Finalize()
declare i32 @MPI_Type_create_struct(i32, ptr, ptr, ptr, ptr)

; Test function with various MPI operations
define i32 @test_policy_controls() {
entry:
  %argc = alloca i32, align 4
  %argv = alloca ptr, align 8
  %buffer = alloca [100 x i32], align 16
  %status = alloca [6 x i32], align 16
  %comm = alloca i32, align 4
  %newtype = alloca i32, align 4
  
  ; Environment operation - should be skipped in lightweight mode
  ; LIGHTWEIGHT-NOT: call void @__mpi_sanitizer_pre_call
  ; FULL: call void @__mpi_sanitizer_pre_call(ptr @.str, ptr %{{.*}}, i32 2, ptr @.str.{{.*}})
  ; DISABLE_ENV-NOT: call void @__mpi_sanitizer_pre_call
  %init_result = call i32 @MPI_Init(ptr %argc, ptr %argv)
  
  ; Point-to-point operation - should be instrumented in both modes
  ; LIGHTWEIGHT: call void @__mpi_sanitizer_pre_call(ptr @.str.{{.*}}, ptr %{{.*}}, i32 6, ptr @.str.{{.*}})
  ; FULL: call void @__mpi_sanitizer_pre_call(ptr @.str.{{.*}}, ptr %{{.*}}, i32 6, ptr @.str.{{.*}})
  %send_result = call i32 @MPI_Send(ptr %buffer, i32 10, i32 1, i32 1, i32 0, i32 0)
  
  ; Point-to-point operation - should be instrumented in both modes
  ; LIGHTWEIGHT: call void @__mpi_sanitizer_pre_call(ptr @.str.{{.*}}, ptr %{{.*}}, i32 7, ptr @.str.{{.*}})
  ; FULL: call void @__mpi_sanitizer_pre_call(ptr @.str.{{.*}}, ptr %{{.*}}, i32 7, ptr @.str.{{.*}})
  %recv_result = call i32 @MPI_Recv(ptr %buffer, i32 10, i32 1, i32 0, i32 0, i32 0, ptr %status)
  
  ; Collective operation - should be instrumented in both modes (critical collective)
  ; LIGHTWEIGHT: call void @__mpi_sanitizer_pre_call(ptr @.str.{{.*}}, ptr %{{.*}}, i32 5, ptr @.str.{{.*}})
  ; FULL: call void @__mpi_sanitizer_pre_call(ptr @.str.{{.*}}, ptr %{{.*}}, i32 5, ptr @.str.{{.*}})
  %bcast_result = call i32 @MPI_Bcast(ptr %buffer, i32 10, i32 1, i32 0, i32 0)
  
  ; Collective operation - should be instrumented in both modes (synchronization)
  ; LIGHTWEIGHT: call void @__mpi_sanitizer_pre_call(ptr @.str.{{.*}}, ptr %{{.*}}, i32 1, ptr @.str.{{.*}})
  ; FULL: call void @__mpi_sanitizer_pre_call(ptr @.str.{{.*}}, ptr %{{.*}}, i32 1, ptr @.str.{{.*}})
  %barrier_result = call i32 @MPI_Barrier(i32 0)
  
  ; Datatype operation - should be skipped in lightweight mode
  ; LIGHTWEIGHT-NOT: call void @__mpi_sanitizer_pre_call
  ; FULL: call void @__mpi_sanitizer_pre_call(ptr @.str.{{.*}}, ptr %{{.*}}, i32 5, ptr @.str.{{.*}})
  %type_result = call i32 @MPI_Type_create_struct(i32 1, ptr null, ptr null, ptr null, ptr %newtype)
  
  ; Environment operation - should be skipped in lightweight mode
  ; LIGHTWEIGHT-NOT: call void @__mpi_sanitizer_pre_call
  ; FULL: call void @__mpi_sanitizer_pre_call(ptr @.str.{{.*}}, ptr %{{.*}}, i32 0, ptr @.str.{{.*}})
  ; DISABLE_ENV-NOT: call void @__mpi_sanitizer_pre_call
  %finalize_result = call i32 @MPI_Finalize()
  
  ret i32 0
}

; Test function for performance mode
define i32 @test_performance_mode() {
entry:
  %buffer = alloca [100 x i32], align 16
  %comm = alloca i32, align 4
  
  ; In performance mode, only operations that benefit from performance monitoring should be instrumented
  %bcast_result = call i32 @MPI_Bcast(ptr %buffer, i32 10, i32 1, i32 0, i32 0)
  %send_result = call i32 @MPI_Send(ptr %buffer, i32 10, i32 1, i32 1, i32 0, i32 0)
  
  ret i32 0
}