
// RUN: %mqss-opt %s --CommonCNOTReversePass 2>&1 | FileCheck %s

// A leading PauliX feeds the CNOT's control: CommonCNOTReversePass must leave
// it untouched while still correctly SSA-threading its result through the
// H-CX-H sandwich, and must rewire the function's own `return` (not just an
// intervening op) to the reversal's final result.
  func.func public @circuit_test_0() ->  !quantum.bit attributes {diff_method = "parameter-shift", llvm.linkage = #llvm.linkage<internal>, qnode} {
    %c1000_i64 = arith.constant 1000 : i64
    %0 = quantum.alloc( 2) : !quantum.reg
    %1 = quantum.extract %0[ 0] : !quantum.reg -> !quantum.bit
    %2 = quantum.extract %0[ 1] : !quantum.reg -> !quantum.bit
    %out_qubits = quantum.custom "PauliX"() %1 : !quantum.bit
    %out_qubits_0:2 = quantum.custom "CNOT"() %out_qubits, %2 : !quantum.bit, !quantum.bit
    return %out_qubits_0#0 : !quantum.bit
  }

// CHECK:  %out_qubits = quantum.custom "PauliX"() %1 : !quantum.bit
// CHECK:  %out_qubits_0 = quantum.custom "H"() %2 : !quantum.bit
// CHECK:  %out_qubits_1 = quantum.custom "H"() %out_qubits : !quantum.bit
// CHECK:  %out_qubits_2:2 = quantum.custom "CNOT"() %out_qubits_0, %out_qubits_1 : !quantum.bit, !quantum.bit
// CHECK:  %out_qubits_3 = quantum.custom "H"() %out_qubits_2#0 : !quantum.bit
// CHECK:  %out_qubits_4 = quantum.custom "H"() %out_qubits_2#1 : !quantum.bit
// CHECK:  return %out_qubits_3
