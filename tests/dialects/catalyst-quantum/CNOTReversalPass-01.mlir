
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

// CHECK:  %[[OUT_QUBITS:[A-Za-z0-9_]+]] = quantum.custom "PauliX"() %{{[A-Za-z0-9_]+}} : !quantum.bit
// CHECK:  %[[OUT_QUBITS_0:[A-Za-z0-9_]+]] = quantum.custom "H"() %{{[A-Za-z0-9_]+}} : !quantum.bit
// CHECK:  %[[OUT_QUBITS_1:[A-Za-z0-9_]+]] = quantum.custom "H"() %[[OUT_QUBITS]] : !quantum.bit
// CHECK:  %[[OUT_QUBITS_2:[A-Za-z0-9_]+]]:2 = quantum.custom "CNOT"() %[[OUT_QUBITS_0]], %[[OUT_QUBITS_1]] : !quantum.bit, !quantum.bit
// CHECK:  %[[OUT_QUBITS_3:[A-Za-z0-9_]+]] = quantum.custom "H"() %[[OUT_QUBITS_2]]#0 : !quantum.bit
// CHECK:  %{{[A-Za-z0-9_]+}} = quantum.custom "H"() %[[OUT_QUBITS_2]]#1 : !quantum.bit
// CHECK:  return %[[OUT_QUBITS_3]]
