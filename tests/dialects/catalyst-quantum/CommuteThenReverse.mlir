
// RUN: %mqss-opt %s --CommonCommutePass=mode=CX-Z --CommonCNOTReversePass 2>&1 | FileCheck %s

// CommonCommutePass moves a PauliZ across the CNOT chain in the first circuit, and
// CommonCNOTReversePass then rewrites every remaining CNOT (in both circuits) into
// its H-CX-H reversed-control-target form.
module @circuit_CommuteThenReverse {
  func.func public @jit_circuit_CommuteThenReverse() -> (tensor<8xi64>, tensor<8xi64>, tensor<4xi64>, tensor<4xi64>) attributes {llvm.emit_c_interface} {
    %0:2 = call @circuit_test_0() : () -> (tensor<8xi64>, tensor<8xi64>)
    %1:2 = call @circuit_test1_0() : () -> (tensor<4xi64>, tensor<4xi64>)
    return %0#0, %0#1, %1#0, %1#1 : tensor<8xi64>, tensor<8xi64>, tensor<4xi64>, tensor<4xi64>
  }
  func.func public @circuit_test_0() -> (tensor<8xi64>, tensor<8xi64>) attributes {diff_method = "parameter-shift", llvm.linkage = #llvm.linkage<internal>, qnode} {
    %c1000_i64 = arith.constant 1000 : i64
    %cst = arith.constant 2.400000e+00 : f64
    quantum.device shots(%c1000_i64) ["/workspaces/MQSS-Passes-Suite/build/_deps/venv/lib/python3.11/site-packages/pennylane_lightning/liblightning_qubit_catalyst.so", "LightningSimulator", "{'mcmc': False, 'num_burnin': 0, 'kernel_name': None}"]
    %0 = quantum.alloc( 3) : !quantum.reg
    %1 = quantum.extract %0[ 2] : !quantum.reg -> !quantum.bit
    %out_qubits = quantum.custom "RX"(%cst) %1 : !quantum.bit
    %2 = quantum.extract %0[ 0] : !quantum.reg -> !quantum.bit
    %3 = quantum.extract %0[ 1] : !quantum.reg -> !quantum.bit
    %out_qubits_1:2 = quantum.custom "CNOT"() %2, %3 : !quantum.bit, !quantum.bit
    %out_qubits_2 = quantum.custom "PauliZ"() %out_qubits_1#1 : !quantum.bit
    %out_qubits_3:2 = quantum.custom "CNOT"() %out_qubits_2, %out_qubits_1#0 : !quantum.bit, !quantum.bit
    %4 = quantum.insert %0[ 0], %out_qubits_3#0 : !quantum.reg, !quantum.bit
    %5 = quantum.insert %4[ 1], %out_qubits_3#1 : !quantum.reg, !quantum.bit
    %6 = quantum.insert %5[ 2], %out_qubits : !quantum.reg, !quantum.bit
    %7 = quantum.compbasis qreg %6 : !quantum.obs
    %eigvals, %counts = quantum.counts %7 : tensor<8xf64>, tensor<8xi64>
    %8 = tensor.empty() : tensor<8xi64>
    %9 = linalg.generic {indexing_maps = [affine_map<(d0) -> (d0)>, affine_map<(d0) -> (d0)>], iterator_types = ["parallel"]} ins(%eigvals : tensor<8xf64>) outs(%8 : tensor<8xi64>) {
    ^bb0(%in: f64, %out: i64):
      %10 = arith.fptosi %in : f64 to i64
      linalg.yield %10 : i64
    } -> tensor<8xi64>
    quantum.dealloc %6 : !quantum.reg
    quantum.device_release
    return %9, %counts : tensor<8xi64>, tensor<8xi64>
  }
  func.func public @circuit_test1_0() -> (tensor<4xi64>, tensor<4xi64>) attributes {diff_method = "parameter-shift", llvm.linkage = #llvm.linkage<internal>, qnode} {
    %c1000_i64 = arith.constant 1000 : i64
    quantum.device shots(%c1000_i64) ["/workspaces/MQSS-Passes-Suite/build/_deps/venv/lib/python3.11/site-packages/pennylane_lightning/liblightning_qubit_catalyst.so", "LightningSimulator", "{'mcmc': False, 'num_burnin': 0, 'kernel_name': None}"]
    %0 = quantum.alloc( 2) : !quantum.reg
    %1 = quantum.extract %0[ 0] : !quantum.reg -> !quantum.bit
    %2 = quantum.extract %0[ 1] : !quantum.reg -> !quantum.bit
    %out_qubits:2 = quantum.custom "CNOT"() %1, %2 : !quantum.bit, !quantum.bit
    %3 = quantum.insert %0[ 0], %out_qubits#0 : !quantum.reg, !quantum.bit
    %4 = quantum.insert %3[ 1], %out_qubits#1 : !quantum.reg, !quantum.bit
    %5 = quantum.compbasis qreg %4 : !quantum.obs
    %eigvals, %counts = quantum.counts %5 : tensor<4xf64>, tensor<4xi64>
    %6 = tensor.empty() : tensor<4xi64>
    %7 = linalg.generic {indexing_maps = [affine_map<(d0) -> (d0)>, affine_map<(d0) -> (d0)>], iterator_types = ["parallel"]} ins(%eigvals : tensor<4xf64>) outs(%6 : tensor<4xi64>) {
    ^bb0(%in: f64, %out: i64):
      %8 = arith.fptosi %in : f64 to i64
      linalg.yield %8 : i64
    } -> tensor<4xi64>
    quantum.dealloc %4 : !quantum.reg
    quantum.device_release
    return %7, %counts : tensor<4xi64>, tensor<4xi64>
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

// CHECK: %out_qubits_0 = quantum.custom "H"() %3 : !quantum.bit
// CHECK: %out_qubits_1 = quantum.custom "H"() %2 : !quantum.bit
// CHECK: %out_qubits_2:2 = quantum.custom "CNOT"() %out_qubits_0, %out_qubits_1 : !quantum.bit, !quantum.bit
// CHECK: %out_qubits_3 = quantum.custom "H"() %out_qubits_2#0 : !quantum.bit
// CHECK: %out_qubits_4 = quantum.custom "H"() %out_qubits_2#1 : !quantum.bit
// CHECK: %out_qubits_5 = quantum.custom "PauliZ"() %out_qubits_4 : !quantum.bit
// CHECK: %out_qubits_6 = quantum.custom "H"() %out_qubits_3 : !quantum.bit
// CHECK: %out_qubits_7 = quantum.custom "H"() %out_qubits_5 : !quantum.bit
// CHECK: %out_qubits_8:2 = quantum.custom "CNOT"() %out_qubits_6, %out_qubits_7 : !quantum.bit, !quantum.bit
// CHECK: %out_qubits_9 = quantum.custom "H"() %out_qubits_8#0 : !quantum.bit
// CHECK: %out_qubits_10 = quantum.custom "H"() %out_qubits_8#1 : !quantum.bit

// CHECK: %out_qubits = quantum.custom "H"() %2 : !quantum.bit
// CHECK: %out_qubits_0 = quantum.custom "H"() %1 : !quantum.bit
// CHECK: %out_qubits_1:2 = quantum.custom "CNOT"() %out_qubits, %out_qubits_0 : !quantum.bit, !quantum.bit
// CHECK: %out_qubits_2 = quantum.custom "H"() %out_qubits_1#0 : !quantum.bit
// CHECK: %out_qubits_3 = quantum.custom "H"() %out_qubits_1#1 : !quantum.bit
