module @kernel {
  func.func public @jit_kernel() -> (memref<4xi64>, memref<4xi64>) attributes {llvm.copy_memref, llvm.emit_c_interface} {
    %0 = llvm.mlir.constant(3735928559 : index) : i64
    %1:2 = call @kernel_0() : () -> (memref<4xi64>, memref<4xi64>)
    %2 = builtin.unrealized_conversion_cast %1#0 : memref<4xi64> to !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>
    %3 = llvm.extractvalue %2[0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %4 = llvm.ptrtoint %3 : !llvm.ptr to i64
    %5 = llvm.icmp "eq" %0, %4 : i64
    %6 = scf.if %5 -> (memref<4xi64>) {
      %alloc = memref.alloc() : memref<4xi64>
      memref.copy %1#0, %alloc : memref<4xi64> to memref<4xi64>
      scf.yield %alloc : memref<4xi64>
    } else {
      scf.yield %1#0 : memref<4xi64>
    }
    %7 = builtin.unrealized_conversion_cast %1#1 : memref<4xi64> to !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>
    %8 = llvm.extractvalue %7[0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %9 = llvm.ptrtoint %8 : !llvm.ptr to i64
    %10 = llvm.icmp "eq" %0, %9 : i64
    %11 = scf.if %10 -> (memref<4xi64>) {
      %alloc = memref.alloc() : memref<4xi64>
      memref.copy %1#1, %alloc : memref<4xi64> to memref<4xi64>
      scf.yield %alloc : memref<4xi64>
    } else {
      scf.yield %1#1 : memref<4xi64>
    }
    return %6, %11 : memref<4xi64>, memref<4xi64>
  }
  func.func public @kernel_0() -> (memref<4xi64>, memref<4xi64>) attributes {diff_method = "parameter-shift", llvm.linkage = #llvm.linkage<internal>, qnode} {
    %c1000_i64 = arith.constant 1000 : i64
    quantum.device shots(%c1000_i64) ["/workspaces/MQSS-Passes-Suite/build/_deps/venv/lib/python3.11/site-packages/pennylane_lightning/liblightning_qubit_catalyst.so", "LightningSimulator", "{'mcmc': False, 'num_burnin': 0, 'kernel_name': None}"]
    %0 = quantum.alloc( 2) : !quantum.reg
    %1 = quantum.extract %0[ 0] : !quantum.reg -> !quantum.bit
    %2 = quantum.extract %0[ 1] : !quantum.reg -> !quantum.bit
    %out_qubits:2 = quantum.custom "CNOT"() %1, %2 : !quantum.bit, !quantum.bit
    %out_qubits_0 = quantum.custom "PauliX"() %out_qubits#1 : !quantum.bit
    %out_qubits_1:2 = quantum.custom "CNOT"() %out_qubits_0, %out_qubits#0 : !quantum.bit, !quantum.bit
    %out_qubits_2 = quantum.custom "PauliX"() %out_qubits_1#0 : !quantum.bit
    %3 = quantum.insert %0[ 0], %out_qubits_1#1 : !quantum.reg, !quantum.bit
    %4 = quantum.insert %3[ 1], %out_qubits_2 : !quantum.reg, !quantum.bit
    %5 = quantum.compbasis qreg %4 : !quantum.obs
    %alloc = memref.alloc() : memref<4xf64>
    %alloc_3 = memref.alloc() : memref<4xi64>
    quantum.counts %5 in(%alloc : memref<4xf64>, %alloc_3 : memref<4xi64>)
    %alloc_4 = memref.alloc() {alignment = 64 : i64} : memref<4xi64>
    linalg.generic {indexing_maps = [affine_map<(d0) -> (d0)>, affine_map<(d0) -> (d0)>], iterator_types = ["parallel"]} ins(%alloc : memref<4xf64>) outs(%alloc_4 : memref<4xi64>) {
    ^bb0(%in: f64, %out: i64):
      %6 = arith.fptosi %in : f64 to i64
      linalg.yield %6 : i64
    }
    memref.dealloc %alloc : memref<4xf64>
    quantum.dealloc %4 : !quantum.reg
    quantum.device_release
    return %alloc_4, %alloc_3 : memref<4xi64>, memref<4xi64>
  }
  func.func @setup() {
    quantum.init
    return
  }
  func.func @teardown() {
    quantum.finalize
    return
  }
}