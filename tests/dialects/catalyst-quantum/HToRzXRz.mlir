
// RUN: %mqss-opt %s --CommonDecompositionPass=mode=HToRzXRz  2>&1 | FileCheck %s

module @circuit_HToRzXRz {
  func.func public @circuit_HToRzXRz_0() -> (tensor<i1>, tensor<i1>, tensor<i1>, tensor<i1>) attributes {diff_method = "adjoint", llvm.linkage = #llvm.linkage<internal>, qnode} {
    %c1_i64 = arith.constant 1 : i64
    quantum.device shots(%c1_i64) ["/workspaces/MQSS-Passes-Suite/_deps/.venv/lib/python3.11/site-packages/pennylane_lightning/liblightning_qubit_catalyst.so", "LightningSimulator", "{'mcmc': False, 'num_burnin': 0, 'kernel_name': None}"]
    %0 = quantum.alloc( 2) : !quantum.reg
    %1 = quantum.extract %0[ 0] : !quantum.reg -> !quantum.bit
    %2 = quantum.extract %0[ 1] : !quantum.reg -> !quantum.bit
    %out_qubits:2 = quantum.custom "CNOT"() %1, %2 : !quantum.bit, !quantum.bit
    %out_qubits_0 = quantum.custom "Hadamard"() %out_qubits#0 : !quantum.bit
    %mres, %out_qubit = quantum.measure %out_qubits_0 : i1, !quantum.bit
    %from_elements = tensor.from_elements %mres : tensor<i1>
    %mres_1, %out_qubit_2 = quantum.measure %out_qubits#1 : i1, !quantum.bit
    %from_elements_3 = tensor.from_elements %mres_1 : tensor<i1>
    %3 = quantum.insert %0[ 0], %out_qubit : !quantum.reg, !quantum.bit
    %4 = quantum.insert %3[ 1], %out_qubit_2 : !quantum.reg, !quantum.bit
    quantum.dealloc %4 : !quantum.reg
    quantum.device_release
    return %from_elements, %from_elements_3, %from_elements, %from_elements_3 : tensor<i1>, tensor<i1>, tensor<i1>, tensor<i1>
  }
}

// CHECK:  %cst = arith.constant 3.1415926535897931 : f64
// CHECK:  %cst_0 = arith.constant 1.5707963267948966 : f64
// CHECK:  %out_qubits_1 = quantum.custom "RZ"(%cst) %out_qubits#0 : !quantum.bit
// CHECK:  %out_qubits_2 = quantum.custom "PauliX"() %out_qubits_1 : !quantum.bit
// CHECK:  %out_qubits_3 = quantum.custom "RZ"(%cst_0) %out_qubits_2 : !quantum.bit
// CHECK: %mres, %out_qubit = quantum.measure %out_qubits_3 : i1, !quantum.bit