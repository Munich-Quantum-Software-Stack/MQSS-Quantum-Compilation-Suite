
// RUN: %mqss-opt %s --CommonDecompositionPass=mode=CHToCX 2>&1 | FileCheck %s

  func.func public @circuit_test_0() ->  !quantum.bit attributes {diff_method = "parameter-shift", llvm.linkage = #llvm.linkage<internal>, qnode} {
    %c1000_i64 = arith.constant 1000 : i64
    %0 = quantum.alloc( 2) : !quantum.reg
    %1 = quantum.extract %0[ 0] : !quantum.reg -> !quantum.bit
    %out_qubits = quantum.custom "PauliX"() %1 : !quantum.bit
    %out:2 = quantum.custom "Hadamard"() %out_qubits, %1 : !quantum.bit, !quantum.bit
    return %out#0 : !quantum.bit
  }

// CHECK:  %out_qubits = quantum.custom "PauliX"() %1 : !quantum.bit
// CHECK:  %out_qubits_0 = quantum.custom "S"() %1 : !quantum.bit
// CHECK:  %out_qubits_1 = quantum.custom "H"() %out_qubits_0 : !quantum.bit
// CHECK:  %out_qubits_2 = quantum.custom "T"() %out_qubits_1 : !quantum.bit
// CHECK:  %out_qubits_3:2 = quantum.custom "CNOT"() %out_qubits, %out_qubits_2 : !quantum.bit, !quantum.bit
// CHECK:  %out_qubits_4 = quantum.custom "T"() %out_qubits_3#1 adj : !quantum.bit
// CHECK:  %out_qubits_5 = quantum.custom "H"() %out_qubits_4 : !quantum.bit
// CHECK:  %out_qubits_6 = quantum.custom "S"() %out_qubits_5 adj : !quantum.bit