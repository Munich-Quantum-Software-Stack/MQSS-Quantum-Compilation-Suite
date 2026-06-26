
// RUN: %mqss-catalyst-opt %s --CommonCommutePass=mode=CX-Z 2>&1 | FileCheck %s

module @circuit_CommuteCNotZ {
  func.func public @jit_circuit_CommuteCNotZ() -> (tensor<8xi64>, tensor<8xi64>) attributes {llvm.emit_c_interface} {
    %0:2 = call @circuit_CommuteCNotZ_0() : () -> (tensor<8xi64>, tensor<8xi64>)
    return %0#0, %0#1 : tensor<8xi64>, tensor<8xi64>
  }
  func.func public @circuit_CommuteCNotZ_0() -> (tensor<8xi64>, tensor<8xi64>) attributes {diff_method = "parameter-shift", llvm.linkage = #llvm.linkage<internal>, qnode} {
    %c1000_i64 = arith.constant 1000 : i64
    %cst = arith.constant 2.400000e+00 : f64
    %cst_0 = arith.constant 5.141600e+00 : f64
    quantum.device shots(%c1000_i64) ["/workspaces/MQSS-Passes-Suite/build/_deps/venv/lib/python3.11/site-packages/pennylane_lightning/liblightning_qubit_catalyst.so", "LightningSimulator", "{'mcmc': False, 'num_burnin': 0, 'kernel_name': None}"]
    %0 = quantum.alloc( 3) : !quantum.reg
    %1 = quantum.extract %0[ 2] : !quantum.reg -> !quantum.bit
    %out_qubits = quantum.custom "RZ"(%cst) %1 : !quantum.bit
    %2 = quantum.extract %0[ 0] : !quantum.reg -> !quantum.bit
    %3 = quantum.extract %0[ 1] : !quantum.reg -> !quantum.bit
    %out_qubits_1:2 = quantum.custom "CNOT"() %2, %3 : !quantum.bit, !quantum.bit
    %out_qubits_2 = quantum.custom "PauliZ"() %out_qubits_1#1 : !quantum.bit
    %out_qubits_3:2 = quantum.custom "CNOT"() %out_qubits_2, %out_qubits_1#0 : !quantum.bit, !quantum.bit
    %out_qubits_4 = quantum.custom "PauliZ"() %out_qubits_3#0 : !quantum.bit
    %out_qubits_5:2 = quantum.custom "CNOT"() %out_qubits_3#1, %out_qubits_4 : !quantum.bit, !quantum.bit
    %out_qubits_6 = quantum.custom "RX"(%cst_0) %out_qubits_5#1 : !quantum.bit
    %out_qubits_7 = quantum.custom "PauliZ"() %out_qubits_6 : !quantum.bit
    %4 = quantum.insert %0[ 0], %out_qubits_5#0 : !quantum.reg, !quantum.bit
    %5 = quantum.insert %4[ 1], %out_qubits_7 : !quantum.reg, !quantum.bit
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
  func.func @setup() {
    quantum.init
    return
  }
  func.func @teardown() {
    quantum.finalize
    return
  }
}

// CHECK: %out_qubits_1:2 = quantum.custom "CNOT"() %2, %3 : !quantum.bit, !quantum.bit
// CHECK: %out_qubits_2 = quantum.custom "PauliZ"() %out_qubits_1#1 : !quantum.bit
// CHECK: %out_qubits_3 = quantum.custom "PauliZ"() %out_qubits_2 : !quantum.bit
// CHECK: %out_qubits_4:2 = quantum.custom "CNOT"() %out_qubits_3, %out_qubits_1#0 : !quantum.bit, !quantum.bit
