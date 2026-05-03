module @kernel {
  llvm.func @__catalyst__rt__finalize()
  llvm.func @__catalyst__rt__initialize(!llvm.ptr)
  llvm.func @__catalyst__rt__device_release()
  llvm.func @__catalyst__rt__qubit_release_array(!llvm.ptr)
  llvm.func @__catalyst__qis__Counts(!llvm.ptr, i64, ...)
  llvm.func @__catalyst__qis__PauliX(!llvm.ptr, !llvm.ptr)
  llvm.func @__catalyst__qis__CNOT(!llvm.ptr, !llvm.ptr, !llvm.ptr)
  llvm.func @__catalyst__rt__array_get_element_ptr_1d(!llvm.ptr, i64) -> !llvm.ptr
  llvm.func @__catalyst__rt__qubit_allocate_array(i64) -> !llvm.ptr
  llvm.mlir.global internal constant @"{'mcmc': False, 'num_burnin': 0, 'kernel_name': None}"("{'mcmc': False, 'num_burnin': 0, 'kernel_name': None}\00") {addr_space = 0 : i32}
  llvm.mlir.global internal constant @LightningSimulator("LightningSimulator\00") {addr_space = 0 : i32}
  llvm.mlir.global internal constant @"/workspaces/MQSS-Passes-Suite/build/_deps/venv/lib/python3.11/site-packages/pennylane_lightning/liblightning_qubit_catalyst.so"("/workspaces/MQSS-Passes-Suite/build/_deps/venv/lib/python3.11/site-packages/pennylane_lightning/liblightning_qubit_catalyst.so\00") {addr_space = 0 : i32}
  llvm.func @__catalyst__rt__device_init(!llvm.ptr, !llvm.ptr, !llvm.ptr, i64, i1)
  llvm.func @_mlir_memref_to_llvm_free(!llvm.ptr)
  llvm.func @_mlir_memref_to_llvm_alloc(i64) -> !llvm.ptr
  llvm.func @jit_kernel() -> !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)> attributes {llvm.copy_memref, llvm.emit_c_interface} {
    %0 = llvm.mlir.poison : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)>
    %1 = llvm.mlir.constant(0 : index) : i64
    %2 = llvm.mlir.poison : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>
    %3 = llvm.mlir.zero : !llvm.ptr
    %4 = llvm.mlir.constant(1 : index) : i64
    %5 = llvm.mlir.constant(4 : index) : i64
    %6 = llvm.mlir.constant(3735928559 : index) : i64
    %7 = llvm.call @kernel_0() : () -> !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)>
    %8 = llvm.extractvalue %7[0] : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)> 
    %9 = llvm.extractvalue %7[1] : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)> 
    %10 = llvm.extractvalue %7[0, 0] : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)> 
    %11 = llvm.ptrtoint %10 : !llvm.ptr to i64
    %12 = llvm.icmp "eq" %6, %11 : i64
    llvm.cond_br %12, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %13 = llvm.getelementptr %3[4] : (!llvm.ptr) -> !llvm.ptr, i64
    %14 = llvm.ptrtoint %13 : !llvm.ptr to i64
    %15 = llvm.call @_mlir_memref_to_llvm_alloc(%14) : (i64) -> !llvm.ptr
    %16 = llvm.insertvalue %15, %2[0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %17 = llvm.insertvalue %15, %16[1] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %18 = llvm.insertvalue %1, %17[2] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %19 = llvm.insertvalue %5, %18[3, 0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %20 = llvm.insertvalue %4, %19[4, 0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %21 = llvm.extractvalue %7[0, 3, 0] : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)> 
    %22 = llvm.mul %21, %4 : i64
    %23 = llvm.getelementptr %3[1] : (!llvm.ptr) -> !llvm.ptr, i64
    %24 = llvm.ptrtoint %23 : !llvm.ptr to i64
    %25 = llvm.mul %22, %24 : i64
    %26 = llvm.extractvalue %7[0, 1] : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)> 
    %27 = llvm.extractvalue %7[0, 2] : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)> 
    %28 = llvm.getelementptr inbounds %26[%27] : (!llvm.ptr, i64) -> !llvm.ptr, i64
    "llvm.intr.memcpy"(%15, %28, %25) <{isVolatile = false}> : (!llvm.ptr, !llvm.ptr, i64) -> ()
    llvm.br ^bb3(%20 : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)
  ^bb2:  // pred: ^bb0
    llvm.br ^bb3(%8 : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)
  ^bb3(%29: !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>):  // 2 preds: ^bb1, ^bb2
    llvm.br ^bb4
  ^bb4:  // pred: ^bb3
    %30 = llvm.extractvalue %7[1, 0] : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)> 
    %31 = llvm.ptrtoint %30 : !llvm.ptr to i64
    %32 = llvm.icmp "eq" %6, %31 : i64
    llvm.cond_br %32, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %33 = llvm.getelementptr %3[4] : (!llvm.ptr) -> !llvm.ptr, i64
    %34 = llvm.ptrtoint %33 : !llvm.ptr to i64
    %35 = llvm.call @_mlir_memref_to_llvm_alloc(%34) : (i64) -> !llvm.ptr
    %36 = llvm.insertvalue %35, %2[0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %37 = llvm.insertvalue %35, %36[1] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %38 = llvm.insertvalue %1, %37[2] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %39 = llvm.insertvalue %5, %38[3, 0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %40 = llvm.insertvalue %4, %39[4, 0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %41 = llvm.extractvalue %7[1, 3, 0] : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)> 
    %42 = llvm.mul %41, %4 : i64
    %43 = llvm.getelementptr %3[1] : (!llvm.ptr) -> !llvm.ptr, i64
    %44 = llvm.ptrtoint %43 : !llvm.ptr to i64
    %45 = llvm.mul %42, %44 : i64
    %46 = llvm.extractvalue %7[1, 1] : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)> 
    %47 = llvm.extractvalue %7[1, 2] : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)> 
    %48 = llvm.getelementptr inbounds %46[%47] : (!llvm.ptr, i64) -> !llvm.ptr, i64
    "llvm.intr.memcpy"(%35, %48, %45) <{isVolatile = false}> : (!llvm.ptr, !llvm.ptr, i64) -> ()
    llvm.br ^bb7(%40 : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)
  ^bb6:  // pred: ^bb4
    llvm.br ^bb7(%9 : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)
  ^bb7(%49: !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>):  // 2 preds: ^bb5, ^bb6
    llvm.br ^bb8
  ^bb8:  // pred: ^bb7
    %50 = llvm.insertvalue %29, %0[0] : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)> 
    %51 = llvm.insertvalue %49, %50[1] : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)> 
    llvm.return %51 : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)>
  }
  llvm.func @_catalyst_pyface_jit_kernel(%arg0: !llvm.ptr, %arg1: !llvm.ptr) {
    llvm.call @_catalyst_ciface_jit_kernel(%arg0) : (!llvm.ptr) -> ()
    llvm.return
  }
  llvm.func @_catalyst_ciface_jit_kernel(%arg0: !llvm.ptr) attributes {llvm.copy_memref, llvm.emit_c_interface} {
    %0 = llvm.call @jit_kernel() : () -> !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)>
    llvm.store %0, %arg0 : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)>, !llvm.ptr
    llvm.return
  }
  llvm.func internal @kernel_0() -> !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)> attributes {diff_method = "parameter-shift", qnode} {
    %0 = llvm.mlir.poison : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)>
    %1 = llvm.mlir.constant(64 : index) : i64
    %2 = llvm.mlir.undef : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)>
    %3 = llvm.mlir.poison : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>
    %4 = llvm.mlir.zero : !llvm.ptr
    %5 = llvm.mlir.constant(0 : i64) : i64
    %6 = llvm.mlir.constant(2 : i64) : i64
    %7 = llvm.mlir.constant(false) : i1
    %8 = llvm.mlir.addressof @"{'mcmc': False, 'num_burnin': 0, 'kernel_name': None}" : !llvm.ptr
    %9 = llvm.mlir.addressof @LightningSimulator : !llvm.ptr
    %10 = llvm.mlir.addressof @"/workspaces/MQSS-Passes-Suite/build/_deps/venv/lib/python3.11/site-packages/pennylane_lightning/liblightning_qubit_catalyst.so" : !llvm.ptr
    %11 = llvm.mlir.constant(1000 : i64) : i64
    %12 = llvm.mlir.constant(0 : index) : i64
    %13 = llvm.mlir.constant(4 : index) : i64
    %14 = llvm.mlir.constant(1 : index) : i64
    %15 = llvm.mlir.constant(1 : i64) : i64
    %16 = llvm.alloca %15 x !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)> : (i64) -> !llvm.ptr
    %17 = llvm.getelementptr inbounds %10[0, 0] : (!llvm.ptr) -> !llvm.ptr, !llvm.array<127 x i8>
    %18 = llvm.getelementptr inbounds %9[0, 0] : (!llvm.ptr) -> !llvm.ptr, !llvm.array<19 x i8>
    %19 = llvm.getelementptr inbounds %8[0, 0] : (!llvm.ptr) -> !llvm.ptr, !llvm.array<54 x i8>
    llvm.call @__catalyst__rt__device_init(%17, %18, %19, %11, %7) : (!llvm.ptr, !llvm.ptr, !llvm.ptr, i64, i1) -> ()
    %20 = llvm.call @__catalyst__rt__qubit_allocate_array(%6) : (i64) -> !llvm.ptr
    %21 = llvm.call @__catalyst__rt__array_get_element_ptr_1d(%20, %5) : (!llvm.ptr, i64) -> !llvm.ptr
    %22 = llvm.load %21 : !llvm.ptr -> !llvm.ptr
    %23 = llvm.call @__catalyst__rt__array_get_element_ptr_1d(%20, %15) : (!llvm.ptr, i64) -> !llvm.ptr
    %24 = llvm.load %23 : !llvm.ptr -> !llvm.ptr
    llvm.call @__catalyst__qis__CNOT(%22, %24, %4) : (!llvm.ptr, !llvm.ptr, !llvm.ptr) -> ()
    llvm.call @__catalyst__qis__PauliX(%24, %4) : (!llvm.ptr, !llvm.ptr) -> ()
    llvm.call @__catalyst__qis__CNOT(%24, %22, %4) : (!llvm.ptr, !llvm.ptr, !llvm.ptr) -> ()
    llvm.call @__catalyst__qis__PauliX(%24, %4) : (!llvm.ptr, !llvm.ptr) -> ()
    %25 = llvm.getelementptr %4[4] : (!llvm.ptr) -> !llvm.ptr, f64
    %26 = llvm.ptrtoint %25 : !llvm.ptr to i64
    %27 = llvm.call @_mlir_memref_to_llvm_alloc(%26) : (i64) -> !llvm.ptr
    %28 = llvm.insertvalue %27, %3[0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %29 = llvm.insertvalue %27, %28[1] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %30 = llvm.insertvalue %12, %29[2] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %31 = llvm.insertvalue %13, %30[3, 0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %32 = llvm.insertvalue %14, %31[4, 0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %33 = llvm.getelementptr %4[4] : (!llvm.ptr) -> !llvm.ptr, i64
    %34 = llvm.ptrtoint %33 : !llvm.ptr to i64
    %35 = llvm.call @_mlir_memref_to_llvm_alloc(%34) : (i64) -> !llvm.ptr
    %36 = llvm.insertvalue %35, %3[0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %37 = llvm.insertvalue %35, %36[1] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %38 = llvm.insertvalue %12, %37[2] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %39 = llvm.insertvalue %13, %38[3, 0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %40 = llvm.insertvalue %14, %39[4, 0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %41 = llvm.insertvalue %32, %2[0] : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)> 
    %42 = llvm.insertvalue %40, %41[1] : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)> 
    llvm.store %42, %16 : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)>, !llvm.ptr
    llvm.call @__catalyst__qis__Counts(%16, %5) vararg(!llvm.func<void (ptr, i64, ...)>) : (!llvm.ptr, i64) -> ()
    %43 = llvm.getelementptr %4[4] : (!llvm.ptr) -> !llvm.ptr, i64
    %44 = llvm.ptrtoint %43 : !llvm.ptr to i64
    %45 = llvm.add %44, %1 : i64
    %46 = llvm.call @_mlir_memref_to_llvm_alloc(%45) : (i64) -> !llvm.ptr
    %47 = llvm.ptrtoint %46 : !llvm.ptr to i64
    %48 = llvm.sub %1, %14 : i64
    %49 = llvm.add %47, %48 : i64
    %50 = llvm.urem %49, %1 : i64
    %51 = llvm.sub %49, %50 : i64
    %52 = llvm.inttoptr %51 : i64 to !llvm.ptr
    %53 = llvm.insertvalue %46, %3[0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %54 = llvm.insertvalue %52, %53[1] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %55 = llvm.insertvalue %12, %54[2] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %56 = llvm.insertvalue %13, %55[3, 0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %57 = llvm.insertvalue %14, %56[4, 0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    llvm.br ^bb1(%12 : i64)
  ^bb1(%58: i64):  // 2 preds: ^bb0, ^bb2
    %59 = llvm.icmp "slt" %58, %13 : i64
    llvm.cond_br %59, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %60 = llvm.getelementptr inbounds %27[%58] : (!llvm.ptr, i64) -> !llvm.ptr, f64
    %61 = llvm.load %60 : !llvm.ptr -> f64
    %62 = llvm.fptosi %61 : f64 to i64
    %63 = llvm.getelementptr inbounds %52[%58] : (!llvm.ptr, i64) -> !llvm.ptr, i64
    llvm.store %62, %63 : i64, !llvm.ptr
    %64 = llvm.add %58, %14 : i64
    llvm.br ^bb1(%64 : i64)
  ^bb3:  // pred: ^bb1
    llvm.call @_mlir_memref_to_llvm_free(%27) : (!llvm.ptr) -> ()
    llvm.call @__catalyst__rt__qubit_release_array(%20) : (!llvm.ptr) -> ()
    llvm.call @__catalyst__rt__device_release() : () -> ()
    %65 = llvm.insertvalue %57, %0[0] : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)> 
    %66 = llvm.insertvalue %40, %65[1] : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)> 
    llvm.return %66 : !llvm.struct<(struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>, struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>)>
  }
  llvm.func @setup() {
    %0 = llvm.mlir.zero : !llvm.ptr
    llvm.call @__catalyst__rt__initialize(%0) : (!llvm.ptr) -> ()
    llvm.return
  }
  llvm.func @teardown() {
    llvm.call @__catalyst__rt__finalize() : () -> ()
    llvm.return
  }
}