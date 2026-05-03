; ModuleID = 'LLVMDialectModule'
source_filename = "LLVMDialectModule"

@"{'mcmc': False, 'num_burnin': 0, 'kernel_name': None}" = internal constant [54 x i8] c"{'mcmc': False, 'num_burnin': 0, 'kernel_name': None}\00"
@LightningSimulator = internal constant [19 x i8] c"LightningSimulator\00"
@"/workspaces/MQSS-Passes-Suite/build/_deps/venv/lib/python3.11/site-packages/pennylane_lightning/liblightning_qubit_catalyst.so" = internal constant [127 x i8] c"/workspaces/MQSS-Passes-Suite/build/_deps/venv/lib/python3.11/site-packages/pennylane_lightning/liblightning_qubit_catalyst.so\00"

declare void @__catalyst__rt__finalize()

declare void @__catalyst__rt__initialize(ptr)

declare void @__catalyst__rt__device_release()

declare void @__catalyst__rt__qubit_release_array(ptr)

declare void @__catalyst__qis__Counts(ptr, i64, ...)

declare void @__catalyst__qis__PauliX(ptr, ptr)

declare void @__catalyst__qis__CNOT(ptr, ptr, ptr)

declare ptr @__catalyst__rt__array_get_element_ptr_1d(ptr, i64)

declare ptr @__catalyst__rt__qubit_allocate_array(i64)

declare void @__catalyst__rt__device_init(ptr, ptr, ptr, i64, i1)

declare void @_mlir_memref_to_llvm_free(ptr)

declare ptr @_mlir_memref_to_llvm_alloc(i64)

define { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } @jit_kernel() {
  %1 = call { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } @kernel_0()
  %2 = extractvalue { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } %1, 0
  %3 = extractvalue { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } %1, 1
  %4 = extractvalue { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } %1, 0, 0
  %5 = ptrtoint ptr %4 to i64
  %6 = icmp eq i64 3735928559, %5
  br i1 %6, label %7, label %20

7:                                                ; preds = %0
  %8 = call ptr @_mlir_memref_to_llvm_alloc(i64 32)
  %9 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %8, 0
  %10 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %9, ptr %8, 1
  %11 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %10, i64 0, 2
  %12 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %11, i64 4, 3, 0
  %13 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %12, i64 1, 4, 0
  %14 = extractvalue { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } %1, 0, 3, 0
  %15 = mul i64 %14, 1
  %16 = mul i64 %15, 8
  %17 = extractvalue { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } %1, 0, 1
  %18 = extractvalue { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } %1, 0, 2
  %19 = getelementptr inbounds i64, ptr %17, i64 %18
  call void @llvm.memcpy.p0.p0.i64(ptr %8, ptr %19, i64 %16, i1 false)
  br label %21

20:                                               ; preds = %0
  br label %21

21:                                               ; preds = %7, %20
  %22 = phi { ptr, ptr, i64, [1 x i64], [1 x i64] } [ %2, %20 ], [ %13, %7 ]
  br label %23

23:                                               ; preds = %21
  %24 = extractvalue { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } %1, 1, 0
  %25 = ptrtoint ptr %24 to i64
  %26 = icmp eq i64 3735928559, %25
  br i1 %26, label %27, label %40

27:                                               ; preds = %23
  %28 = call ptr @_mlir_memref_to_llvm_alloc(i64 32)
  %29 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %28, 0
  %30 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %29, ptr %28, 1
  %31 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %30, i64 0, 2
  %32 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %31, i64 4, 3, 0
  %33 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %32, i64 1, 4, 0
  %34 = extractvalue { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } %1, 1, 3, 0
  %35 = mul i64 %34, 1
  %36 = mul i64 %35, 8
  %37 = extractvalue { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } %1, 1, 1
  %38 = extractvalue { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } %1, 1, 2
  %39 = getelementptr inbounds i64, ptr %37, i64 %38
  call void @llvm.memcpy.p0.p0.i64(ptr %28, ptr %39, i64 %36, i1 false)
  br label %41

40:                                               ; preds = %23
  br label %41

41:                                               ; preds = %27, %40
  %42 = phi { ptr, ptr, i64, [1 x i64], [1 x i64] } [ %3, %40 ], [ %33, %27 ]
  br label %43

43:                                               ; preds = %41
  %44 = insertvalue { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } poison, { ptr, ptr, i64, [1 x i64], [1 x i64] } %22, 0
  %45 = insertvalue { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } %44, { ptr, ptr, i64, [1 x i64], [1 x i64] } %42, 1
  ret { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } %45
}

define void @_catalyst_pyface_jit_kernel(ptr %0, ptr %1) {
  call void @_catalyst_ciface_jit_kernel(ptr %0)
  ret void
}

define void @_catalyst_ciface_jit_kernel(ptr %0) {
  %2 = call { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } @jit_kernel()
  store { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } %2, ptr %0, align 8
  ret void
}

define internal { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } @kernel_0() {
  %1 = alloca { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } }, i64 1, align 8
  call void @__catalyst__rt__device_init(ptr @"/workspaces/MQSS-Passes-Suite/build/_deps/venv/lib/python3.11/site-packages/pennylane_lightning/liblightning_qubit_catalyst.so", ptr @LightningSimulator, ptr @"{'mcmc': False, 'num_burnin': 0, 'kernel_name': None}", i64 1000, i1 false)
  %2 = call ptr @__catalyst__rt__qubit_allocate_array(i64 2)
  %3 = call ptr @__catalyst__rt__array_get_element_ptr_1d(ptr %2, i64 0)
  %4 = load ptr, ptr %3, align 8
  %5 = call ptr @__catalyst__rt__array_get_element_ptr_1d(ptr %2, i64 1)
  %6 = load ptr, ptr %5, align 8
  call void @__catalyst__qis__CNOT(ptr %4, ptr %6, ptr null)
  call void @__catalyst__qis__PauliX(ptr %6, ptr null)
  call void @__catalyst__qis__CNOT(ptr %6, ptr %4, ptr null)
  call void @__catalyst__qis__PauliX(ptr %6, ptr null)
  %7 = call ptr @_mlir_memref_to_llvm_alloc(i64 32)
  %8 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %7, 0
  %9 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %8, ptr %7, 1
  %10 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %9, i64 0, 2
  %11 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %10, i64 4, 3, 0
  %12 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %11, i64 1, 4, 0
  %13 = call ptr @_mlir_memref_to_llvm_alloc(i64 32)
  %14 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %13, 0
  %15 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %14, ptr %13, 1
  %16 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %15, i64 0, 2
  %17 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %16, i64 4, 3, 0
  %18 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %17, i64 1, 4, 0
  %19 = insertvalue { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } undef, { ptr, ptr, i64, [1 x i64], [1 x i64] } %12, 0
  %20 = insertvalue { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } %19, { ptr, ptr, i64, [1 x i64], [1 x i64] } %18, 1
  store { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } %20, ptr %1, align 8
  call void (ptr, i64, ...) @__catalyst__qis__Counts(ptr %1, i64 0)
  %21 = call ptr @_mlir_memref_to_llvm_alloc(i64 96)
  %22 = ptrtoint ptr %21 to i64
  %23 = add i64 %22, 63
  %24 = urem i64 %23, 64
  %25 = sub i64 %23, %24
  %26 = inttoptr i64 %25 to ptr
  %27 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %21, 0
  %28 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %27, ptr %26, 1
  %29 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %28, i64 0, 2
  %30 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %29, i64 4, 3, 0
  %31 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %30, i64 1, 4, 0
  br label %32

32:                                               ; preds = %35, %0
  %33 = phi i64 [ %40, %35 ], [ 0, %0 ]
  %34 = icmp slt i64 %33, 4
  br i1 %34, label %35, label %41

35:                                               ; preds = %32
  %36 = getelementptr inbounds double, ptr %7, i64 %33
  %37 = load double, ptr %36, align 8
  %38 = fptosi double %37 to i64
  %39 = getelementptr inbounds i64, ptr %26, i64 %33
  store i64 %38, ptr %39, align 4
  %40 = add i64 %33, 1
  br label %32

41:                                               ; preds = %32
  call void @_mlir_memref_to_llvm_free(ptr %7)
  call void @__catalyst__rt__qubit_release_array(ptr %2)
  call void @__catalyst__rt__device_release()
  %42 = insertvalue { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } poison, { ptr, ptr, i64, [1 x i64], [1 x i64] } %31, 0
  %43 = insertvalue { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } %42, { ptr, ptr, i64, [1 x i64], [1 x i64] } %18, 1
  ret { { ptr, ptr, i64, [1 x i64], [1 x i64] }, { ptr, ptr, i64, [1 x i64], [1 x i64] } } %43
}

define void @setup() {
  call void @__catalyst__rt__initialize(ptr null)
  ret void
}

define void @teardown() {
  call void @__catalyst__rt__finalize()
  ret void
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias writeonly captures(none), ptr noalias readonly captures(none), i64, i1 immarg) #0

attributes #0 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
