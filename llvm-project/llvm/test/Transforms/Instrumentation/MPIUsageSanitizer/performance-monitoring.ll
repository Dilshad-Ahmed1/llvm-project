; RUN: opt < %s -passes=mpi-sanitizer -mpi-sanitizer-enable-performance=true -mpi-sanitizer-enable-comm-volume=true -mpi-sanitizer-enable-comm-pattern=true -S | FileCheck %s

; Test performance monitoring hook insertion for MPI operations

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; MPI function declarations
declare i32 @MPI_Send(i8*, i32, i32, i32, i32, i8*)
declare i32 @MPI_Recv(i8*, i32, i32, i32, i32, i8*, i8*)
declare i32 @MPI_Bcast(i8*, i32, i32, i32, i8*)
declare i32 @MPI_Allreduce(i8*, i8*, i32, i32, i32, i8*)

; CHECK: declare void @__mpi_sanitizer_performance_begin(i8*, i8*)
; CHECK: declare void @__mpi_sanitizer_performance_end(i8*, i8*)
; CHECK: declare void @__mpi_sanitizer_comm_volume(i8*, i64, i8*)
; CHECK: declare void @__mpi_sanitizer_comm_pattern(i8*, i32, i32, i32, i8*)
; CHECK: declare void @__mpi_sanitizer_collective_timing(i8*, i32, double*)

define void @test_point_to_point_performance() {
entry:
  %buffer = alloca [100 x i32], align 4
  %buffer_ptr = bitcast [100 x i32]* %buffer to i8*
  
  ; Point-to-point operation should get performance and communication pattern hooks
  ; CHECK: call void @__mpi_sanitizer_performance_begin(i8* {{.*}}, i8* {{.*}})
  ; CHECK: call void @__mpi_sanitizer_comm_pattern(i8* {{.*}}, i32 -1, i32 1, i32 0, i8* {{.*}})
  %result = call i32 @MPI_Send(i8* %buffer_ptr, i32 100, i32 1, i32 1, i32 0, i8* null)
  ; CHECK: call void @__mpi_sanitizer_performance_end(i8* {{.*}}, i8* {{.*}})
  ; CHECK: call void @__mpi_sanitizer_comm_volume(i8* {{.*}}, i64 400, i8* {{.*}})
  
  ret void
}

define void @test_collective_performance() {
entry:
  %buffer = alloca [1000 x double], align 8
  %buffer_ptr = bitcast [1000 x double]* %buffer to i8*
  
  ; Collective operation should get performance and collective timing hooks
  ; CHECK: call void @__mpi_sanitizer_performance_begin(i8* {{.*}}, i8* {{.*}})
  ; CHECK: call void @__mpi_sanitizer_collective_timing(i8* {{.*}}, i32 -1, double* {{.*}})
  %result = call i32 @MPI_Bcast(i8* %buffer_ptr, i32 1000, i32 2, i32 0, i8* null)
  ; CHECK: call void @__mpi_sanitizer_performance_end(i8* {{.*}}, i8* {{.*}})
  ; CHECK: call void @__mpi_sanitizer_comm_volume(i8* {{.*}}, i64 4000, i8* {{.*}})
  
  ret void
}

define void @test_multiple_operations() {
entry:
  %send_buffer = alloca [50 x i32], align 4
  %recv_buffer = alloca [50 x i32], align 4
  %send_ptr = bitcast [50 x i32]* %send_buffer to i8*
  %recv_ptr = bitcast [50 x i32]* %recv_buffer to i8*
  %status = alloca [8 x i32], align 4
  %status_ptr = bitcast [8 x i32]* %status to i8*
  
  ; Multiple MPI operations should each get their own performance hooks
  ; CHECK: call void @__mpi_sanitizer_performance_begin(i8* {{.*}}, i8* {{.*}})
  ; CHECK: call void @__mpi_sanitizer_comm_pattern(i8* {{.*}}, i32 -1, i32 0, i32 1, i8* {{.*}})
  %send_result = call i32 @MPI_Send(i8* %send_ptr, i32 50, i32 1, i32 0, i32 1, i8* null)
  ; CHECK: call void @__mpi_sanitizer_performance_end(i8* {{.*}}, i8* {{.*}})
  ; CHECK: call void @__mpi_sanitizer_comm_volume(i8* {{.*}}, i64 200, i8* {{.*}})
  
  ; CHECK: call void @__mpi_sanitizer_performance_begin(i8* {{.*}}, i8* {{.*}})
  ; CHECK: call void @__mpi_sanitizer_comm_pattern(i8* {{.*}}, i32 -1, i32 0, i32 1, i8* {{.*}})
  %recv_result = call i32 @MPI_Recv(i8* %recv_ptr, i32 50, i32 1, i32 0, i32 1, i8* null, i8* %status_ptr)
  ; CHECK: call void @__mpi_sanitizer_performance_end(i8* {{.*}}, i8* {{.*}})
  ; CHECK: call void @__mpi_sanitizer_comm_volume(i8* {{.*}}, i64 200, i8* {{.*}})
  
  ret void
}

define void @test_allreduce_collective() {
entry:
  %send_buffer = alloca [256 x double], align 8
  %recv_buffer = alloca [256 x double], align 8
  %send_ptr = bitcast [256 x double]* %send_buffer to i8*
  %recv_ptr = bitcast [256 x double]* %recv_buffer to i8*
  
  ; Allreduce should get collective timing and performance hooks
  ; CHECK: call void @__mpi_sanitizer_performance_begin(i8* {{.*}}, i8* {{.*}})
  ; CHECK: call void @__mpi_sanitizer_collective_timing(i8* {{.*}}, i32 -1, double* {{.*}})
  %result = call i32 @MPI_Allreduce(i8* %send_ptr, i8* %recv_ptr, i32 256, i32 2, i32 1, i8* null)
  ; CHECK: call void @__mpi_sanitizer_performance_end(i8* {{.*}}, i8* {{.*}})
  ; CHECK: call void @__mpi_sanitizer_comm_volume(i8* {{.*}}, i64 1024, i8* {{.*}})
  
  ret void
}

; Test that function names are correctly passed to hooks
; CHECK: @.str{{.*}} = private unnamed_addr constant [9 x i8] c"MPI_Send\00"
; CHECK: @.str{{.*}} = private unnamed_addr constant [9 x i8] c"MPI_Recv\00"
; CHECK: @.str{{.*}} = private unnamed_addr constant [10 x i8] c"MPI_Bcast\00"
; CHECK: @.str{{.*}} = private unnamed_addr constant [13 x i8] c"MPI_Allreduce\00"

; Test that operation types are correctly identified
; CHECK: @.str{{.*}} = private unnamed_addr constant [15 x i8] c"point-to-point\00"
; CHECK: @.str{{.*}} = private unnamed_addr constant [10 x i8] c"collective\00"
; CHECK: @.str{{.*}} = private unnamed_addr constant [10 x i8] c"reduction\00"
; CHECK: @.str{{.*}} = private unnamed_addr constant [10 x i8] c"broadcast\00"