
// RUN: %mqss-opt %s --CommonCommutePass=mode=CX-RX --CommonGateCancellationPass=mode=CancelGate 2>&1 | FileCheck %s

// Sequences CommonCommutePass (moving RX ahead of the CX that follows it) with
// CommonGateCancellationPass, which independently removes the redundant back-to-back
// CNOT pair on qubits 2,3 that the commute step leaves untouched.
module @circuit_CommuteThenCancel {
  func.func public @jit_circuit_CommuteThenCancel() -> (tensor<16xi64>, tensor<16xi64>) attributes {llvm.emit_c_interface} {
    %0:2 = call @circuit_CommuteThenCancel_0() : () -> (tensor<16xi64>, tensor<16xi64>)
    return %0#0, %0#1 : tensor<16xi64>, tensor<16xi64>
  }
  func.func public @circuit_CommuteThenCancel_0() -> (tensor<16xi64>, tensor<16xi64>) attributes {diff_method = "parameter-shift", llvm.linkage = #llvm.linkage<internal>, qnode} {
    %c1000_i64 = arith.constant 1000 : i64
    %cst = arith.constant 2.400000e+00 : f64
    quantum.device shots(%c1000_i64) ["/workspaces/MQSS-Passes-Suite/build/_deps/venv/lib/python3.11/site-packages/pennylane_lightning/liblightning_qubit_catalyst.so", "LightningSimulator", "{'mcmc': False, 'num_burnin': 0, 'kernel_name': None}"]
    %0 = quantum.alloc( 4) : !quantum.reg
    %1 = quantum.extract %0[ 0] : !quantum.reg -> !quantum.bit
    %2 = quantum.extract %0[ 1] : !quantum.reg -> !quantum.bit
    %out_qubits:2 = quantum.custom "CNOT"() %1, %2 : !quantum.bit, !quantum.bit
    %out_qubits_0 = quantum.custom "RX"(%cst) %out_qubits#1 : !quantum.bit
    %out_qubits_1:2 = quantum.custom "CNOT"() %out_qubits_0, %out_qubits#0 : !quantum.bit, !quantum.bit
    %3 = quantum.extract %0[ 2] : !quantum.reg -> !quantum.bit
    %4 = quantum.extract %0[ 3] : !quantum.reg -> !quantum.bit
    %out_qubits_2:2 = quantum.custom "CNOT"() %3, %4 : !quantum.bit, !quantum.bit
    %out_qubits_3:2 = quantum.custom "CNOT"() %out_qubits_2#0, %out_qubits_2#1 : !quantum.bit, !quantum.bit
    %5 = quantum.insert %0[ 0], %out_qubits_1#0 : !quantum.reg, !quantum.bit
    %6 = quantum.insert %5[ 1], %out_qubits_1#1 : !quantum.reg, !quantum.bit
    %7 = quantum.insert %6[ 2], %out_qubits_3#0 : !quantum.reg, !quantum.bit
    %8 = quantum.insert %7[ 3], %out_qubits_3#1 : !quantum.reg, !quantum.bit
    %9 = quantum.compbasis qreg %8 : !quantum.obs
    %eigvals, %counts = quantum.counts %9 : tensor<16xf64>, tensor<16xi64>
    %10 = tensor.empty() : tensor<16xi64>
    %11 = linalg.generic {indexing_maps = [affine_map<(d0) -> (d0)>, affine_map<(d0) -> (d0)>], iterator_types = ["parallel"]} ins(%eigvals : tensor<16xf64>) outs(%10 : tensor<16xi64>) {
    ^bb0(%in: f64, %out: i64):
      %12 = arith.fptosi %in : f64 to i64
      linalg.yield %12 : i64
    } -> tensor<16xi64>
    quantum.dealloc %8 : !quantum.reg
    quantum.device_release
    return %11, %counts : tensor<16xi64>, tensor<16xi64>
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

// CHECK: %1 = quantum.extract %0[ 0] : !quantum.reg -> !quantum.bit
// CHECK: %2 = quantum.extract %0[ 1] : !quantum.reg -> !quantum.bit
// CHECK: %out_qubits = quantum.custom "RX"(%cst) %2 : !quantum.bit
// CHECK: %out_qubits_0:2 = quantum.custom "CNOT"() %1, %out_qubits : !quantum.bit, !quantum.bit
// CHECK: %out_qubits_1:2 = quantum.custom "CNOT"() %out_qubits, %out_qubits_0#0 : !quantum.bit, !quantum.bit
// CHECK: %3 = quantum.extract %0[ 2] : !quantum.reg -> !quantum.bit
// CHECK: %4 = quantum.extract %0[ 3] : !quantum.reg -> !quantum.bit
// CHECK: %5 = quantum.insert %0[ 0], %out_qubits_1#0 : !quantum.reg, !quantum.bit
// CHECK: %6 = quantum.insert %5[ 1], %out_qubits_1#1 : !quantum.reg, !quantum.bit
// CHECK: %7 = quantum.insert %6[ 2], %3 : !quantum.reg, !quantum.bit
// CHECK: %8 = quantum.insert %7[ 3], %4 : !quantum.reg, !quantum.bit
