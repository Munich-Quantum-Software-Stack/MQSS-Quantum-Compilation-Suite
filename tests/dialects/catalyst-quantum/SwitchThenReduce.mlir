
// RUN: %mqss-opt %s --CommonSwitchPass=mode=XYZHtoHXYZ --CommonReductionPass=mode=SZToSAdj --canonicalize 2>&1 | FileCheck %s

// Runs CommonSwitchPass on a PauliX..Hadamard circuit and CommonReductionPass on an
// independent S..PauliZ circuit in the same module, verifying both passes apply
// their respective rewrites in a single pipeline invocation.
module @circuit_SwitchThenReduce {
  func.func public @jit_circuit_SwitchThenReduce() -> (tensor<4xi64>, tensor<4xi64>, tensor<4xi64>, tensor<4xi64>) attributes {llvm.emit_c_interface} {
    %0:2 = call @circuit_test_0() : () -> (tensor<4xi64>, tensor<4xi64>)
    %1:2 = call @circuit_test1_0() : () -> (tensor<4xi64>, tensor<4xi64>)
    return %0#0, %0#1, %1#0, %1#1 : tensor<4xi64>, tensor<4xi64>, tensor<4xi64>, tensor<4xi64>
  }
  func.func public @circuit_test_0() -> (tensor<4xi64>, tensor<4xi64>) attributes {diff_method = "parameter-shift", llvm.linkage = #llvm.linkage<internal>, qnode} {
    %c1000_i64 = arith.constant 1000 : i64
    quantum.device shots(%c1000_i64) ["/workspaces/MQSS-Passes-Suite/build/_deps/venv/lib/python3.11/site-packages/pennylane_lightning/liblightning_qubit_catalyst.so", "LightningSimulator", "{'mcmc': False, 'num_burnin': 0, 'kernel_name': None}"]
    %0 = quantum.alloc( 2) : !quantum.reg
    %1 = quantum.extract %0[ 0] : !quantum.reg -> !quantum.bit
    %out_qubits = quantum.custom "PauliX"() %1 : !quantum.bit
    %out_qubits_0 = quantum.custom "Hadamard"() %out_qubits : !quantum.bit
    %2 = quantum.extract %0[ 1] : !quantum.reg -> !quantum.bit
    %3 = quantum.insert %0[ 0], %out_qubits_0 : !quantum.reg, !quantum.bit
    %4 = quantum.insert %3[ 1], %2 : !quantum.reg, !quantum.bit
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
  func.func public @circuit_test1_0() -> (tensor<4xi64>, tensor<4xi64>) attributes {diff_method = "parameter-shift", llvm.linkage = #llvm.linkage<internal>, qnode} {
    %c1000_i64 = arith.constant 1000 : i64
    quantum.device shots(%c1000_i64) ["/workspaces/MQSS-Passes-Suite/build/_deps/venv/lib/python3.11/site-packages/pennylane_lightning/liblightning_qubit_catalyst.so", "LightningSimulator", "{'mcmc': False, 'num_burnin': 0, 'kernel_name': None}"]
    %0 = quantum.alloc( 2) : !quantum.reg
    %1 = quantum.extract %0[ 0] : !quantum.reg -> !quantum.bit
    %out_qubits = quantum.custom "S"() %1 : !quantum.bit
    %out_qubits_0 = quantum.custom "PauliZ"() %out_qubits : !quantum.bit
    %2 = quantum.extract %0[ 1] : !quantum.reg -> !quantum.bit
    %3 = quantum.insert %0[ 0], %out_qubits_0 : !quantum.reg, !quantum.bit
    %4 = quantum.insert %3[ 1], %2 : !quantum.reg, !quantum.bit
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

// CHECK: %out_qubits = quantum.custom "Hadamard"() %1 : !quantum.bit
// CHECK: %2 = quantum.extract %0[ 1] : !quantum.reg -> !quantum.bit
// CHECK: %3 = quantum.insert %0[ 0], %out_qubits : !quantum.reg, !quantum.bit

// CHECK: %out_qubits = quantum.custom "S"() %1 adj : !quantum.bit
// CHECK: %2 = quantum.extract %0[ 1] : !quantum.reg -> !quantum.bit
// CHECK: %3 = quantum.insert %0[ 0], %out_qubits : !quantum.reg, !quantum.bit
