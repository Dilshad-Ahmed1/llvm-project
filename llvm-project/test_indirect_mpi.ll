; ModuleID = 'test_indirect_mpi.c'
source_filename = "test_indirect_mpi.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

@__const.test_function_pointer_array.mpi_funcs = private unnamed_addr constant [2 x ptr] [ptr @MPI_Send, ptr @MPI_Isend], align 8

; Function Attrs: noinline nounwind optnone ssp uwtable(sync)
define void @test_direct_function_pointer() #0 {
  %1 = alloca ptr, align 8
  store ptr @MPI_Send, ptr %1, align 8
  %2 = load ptr, ptr %1, align 8
  %3 = call i32 %2(ptr noundef null, i32 noundef 0, i32 noundef 1, i32 noundef 0, i32 noundef 0, i32 noundef 0)
  ret void
}

declare i32 @MPI_Send(ptr noundef, i32 noundef, i32 noundef, i32 noundef, i32 noundef, i32 noundef) #1

; Function Attrs: noinline nounwind optnone ssp uwtable(sync)
define void @test_function_pointer_variable() #0 {
  %1 = alloca ptr, align 8
  store ptr @MPI_Send, ptr %1, align 8
  %2 = load ptr, ptr %1, align 8
  %3 = call i32 %2(ptr noundef null, i32 noundef 0, i32 noundef 1, i32 noundef 0, i32 noundef 0, i32 noundef 0)
  ret void
}

; Function Attrs: noinline nounwind optnone ssp uwtable(sync)
define void @test_function_pointer_array() #0 {
  %1 = alloca [2 x ptr], align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %1, ptr align 8 @__const.test_function_pointer_array.mpi_funcs, i64 16, i1 false)
  %2 = getelementptr inbounds [2 x ptr], ptr %1, i64 0, i64 0
  %3 = load ptr, ptr %2, align 8
  %4 = call i32 %3(ptr noundef null, i32 noundef 0, i32 noundef 1, i32 noundef 0, i32 noundef 0, i32 noundef 0)
  ret void
}

declare i32 @MPI_Isend(ptr noundef, i32 noundef, i32 noundef, i32 noundef, i32 noundef, i32 noundef) #1

; Function Attrs: argmemonly nocallback nofree nounwind willreturn
declare void @llvm.memcpy.p0.p0.i64(ptr noalias nocapture writeonly, ptr noalias nocapture readonly, i64, i1 immarg) #2

; Function Attrs: noinline nounwind optnone ssp uwtable(sync)
define void @test_conditional_function_pointer(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca ptr, align 8
  store i32 %0, ptr %2, align 4
  %4 = load i32, ptr %2, align 4
  %5 = icmp ne i32 %4, 0
  br i1 %5, label %6, label %7

6:                                                ; preds = %1
  store ptr @MPI_Send, ptr %3, align 8
  br label %8

7:                                                ; preds = %1
  store ptr @MPI_Isend, ptr %3, align 8
  br label %8

8:                                                ; preds = %7, %6
  %9 = load ptr, ptr %3, align 8
  %10 = call i32 %9(ptr noundef null, i32 noundef 0, i32 noundef 1, i32 noundef 0, i32 noundef 0, i32 noundef 0)
  ret void
}

; Function Attrs: noinline nounwind optnone ssp uwtable(sync)
define i32 @main() #0 {
  %1 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  %2 = call i32 @MPI_Init(ptr noundef null, ptr noundef null)
  call void @test_direct_function_pointer()
  call void @test_function_pointer_variable()
  call void @test_function_pointer_array()
  call void @test_conditional_function_pointer(i32 noundef 1)
  %3 = call i32 @MPI_Finalize()
  ret i32 0
}

declare i32 @MPI_Init(ptr noundef, ptr noundef) #1

declare i32 @MPI_Finalize() #1

attributes #0 = { noinline nounwind optnone ssp uwtable(sync) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "probe-stack"="__chkstk_darwin" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+crc,+crypto,+dotprod,+fp-armv8,+fp16fml,+fullfp16,+lse,+neon,+ras,+rcpc,+rdm,+sha2,+sha3,+sm4,+v8.1a,+v8.2a,+v8.3a,+v8.4a,+v8.5a,+v8a,+zcm,+zcz" }
attributes #1 = { "frame-pointer"="non-leaf" "no-trapping-math"="true" "probe-stack"="__chkstk_darwin" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+crc,+crypto,+dotprod,+fp-armv8,+fp16fml,+fullfp16,+lse,+neon,+ras,+rcpc,+rdm,+sha2,+sha3,+sm4,+v8.1a,+v8.2a,+v8.3a,+v8.4a,+v8.5a,+v8a,+zcm,+zcz" }
attributes #2 = { argmemonly nocallback nofree nounwind willreturn }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 2, !"SDK Version", [2 x i32] [i32 14, i32 4]}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 8, !"PIC Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 1}
!5 = !{!"Apple clang version 15.0.0 (clang-1500.3.9.4)"}
