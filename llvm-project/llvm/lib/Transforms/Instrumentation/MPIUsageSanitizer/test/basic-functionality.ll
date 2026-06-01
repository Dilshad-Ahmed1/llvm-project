; RUN: opt %basic-test -S < %s | FileCheck %s
; RUN: opt %optimization-test -S < %s | FileCheck %s --check-prefix=OPT

; Test basic MPI sanitizer functionality with simple MPI calls

target triple = "x86_64-unknown-linux-gnu"

; MPI function declarations
declare i32 @MPI_Init(i32*, i8***)
declare i32 @MPI_Finalize()
declare i32 @MPI_Send(i8*, i32, i32, i32, i32, i32)
declare i32 @MPI_Recv(i8*, i32, i32, i32, i32, i32, i32*)

; CHECK-LABEL: @simple_mpi_program
; CHECK: call{{.*}}@__mpi_sanitizer_pre_init
; CHECK: call i32 @MPI_Init
; CHECK: call{{.*}}@__mpi_sanitizer_post_init

; OPT-LABEL: @simple_mpi_program
; OPT: call{{.*}}@__mpi_sanitizer_pre_init
; OPT: call i32 @MPI_Init
; OPT: call{{.*}}@__mpi_sanitizer_post_init

define i32 @simple_mpi_program(i32 %argc, i8** %argv) {
entry:
  %argc.addr = alloca i32, align 4
  %argv.addr = alloca i8**, align 8
  %buffer = alloca [100 x i8], align 1
  
  store i32 %argc, i32* %argc.addr, align 4
  store i8** %argv, i8*** %argv.addr, align 8
  
  ; Initialize MPI
  %call1 = call i32 @MPI_Init(i32* %argc.addr, i8*** %argv.addr)
  
  ; Send operation
  %buffer_ptr = getelementptr inbounds [100 x i8], [100 x i8]* %buffer, i64 0, i64 0
  %call2 = call i32 @MPI_Send(i8* %buffer_ptr, i32 100, i32 0, i32 1, i32 42, i32 0)
  
  ; Receive operation
  %call3 = call i32 @MPI_Recv(i8* %buffer_ptr, i32 100, i32 0, i32 1, i32 42, i32 0, i32* null)
  
  ; Finalize MPI
  %call4 = call i32 @MPI_Finalize()
  
  ret i32 0
}

; CHECK: declare{{.*}}@__mpi_sanitizer_pre_init
; CHECK: declare{{.*}}@__mpi_sanitizer_post_init
; CHECK: declare{{.*}}@__mpi_sanitizer_pre_send
; CHECK: declare{{.*}}@__mpi_sanitizer_post_send
; CHECK: declare{{.*}}@__mpi_sanitizer_pre_recv
; CHECK: declare{{.*}}@__mpi_sanitizer_post_recv
; CHECK: declare{{.*}}@__mpi_sanitizer_pre_finalize
; CHECK: declare{{.*}}@__mpi_sanitizer_post_finalize