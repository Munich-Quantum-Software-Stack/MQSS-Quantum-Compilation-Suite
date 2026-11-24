
// RUN: %mqss-opt %s --CommonMappingPass=input=%S/../../input/qmap.json 2>&1  | sed -E 's/\x1b\[[0-9;]*m//g' | FileCheck %s

func.func @circuit_qmap1() attributes {qnode} {
  %0 = quantum.alloc(5) : !quantum.reg

  %q0 = quantum.extract %0[0] : !quantum.reg -> !quantum.bit
  %q1 = quantum.extract %0[1] : !quantum.reg -> !quantum.bit
  %q2 = quantum.extract %0[2] : !quantum.reg -> !quantum.bit
  %q3 = quantum.extract %0[3] : !quantum.reg -> !quantum.bit
  %q4 = quantum.extract %0[4] : !quantum.reg -> !quantum.bit

  %cnot1_out:2 = quantum.custom "CNOT"() %q4, %q2 : !quantum.bit, !quantum.bit
  %cnot2_out:2 = quantum.custom "CNOT"() %q3, %q1 : !quantum.bit, !quantum.bit
  %cnot3_out:2 = quantum.custom "CNOT"() %cnot1_out#0, %cnot2_out#1 : !quantum.bit, !quantum.bit

  %mz0:2 = quantum.measure %q0 : i1, !quantum.bit
  %mz1:2 = quantum.measure %cnot1_out#1 : i1, !quantum.bit
  %mz2:2 = quantum.measure %cnot3_out#1 : i1, !quantum.bit
  %mz3:2 = quantum.measure %cnot2_out#0 : i1, !quantum.bit
  %mz4:2 = quantum.measure %cnot3_out#0 : i1, !quantum.bit

  %r0 = quantum.insert %0[0], %mz0#1 : !quantum.reg, !quantum.bit
  %r1 = quantum.insert %r0[1], %mz1#1 : !quantum.reg, !quantum.bit
  %r2 = quantum.insert %r1[2], %mz2#1 : !quantum.reg, !quantum.bit
  %r3 = quantum.insert %r2[3], %mz3#1 : !quantum.reg, !quantum.bit
  %r4 = quantum.insert %r3[4], %mz4#1 : !quantum.reg, !quantum.bit

  quantum.dealloc %r4 : !quantum.reg
  func.return
}

// CHECK: %out_qubits:2 = quantum.custom "CNOT"() %2, %1 : !quantum.bit, !quantum.bit
// CHECK: %3 = quantum.extract %0[ 1] : !quantum.reg -> !quantum.bit
// CHECK: %4 = quantum.extract %0[ 2] : !quantum.reg -> !quantum.bit
// CHECK: %out_qubits_0:2 = quantum.custom "CNOT"() %4, %3 : !quantum.bit, !quantum.bit
// CHECK: %5 = quantum.extract %0[ 0] : !quantum.reg -> !quantum.bit
// CHECK: %out_qubits_1:2 = quantum.custom "SWAP"() %5, %out_qubits#0 : !quantum.bit, !quantum.bit
// CHECK: %out_qubits_2:2 = quantum.custom "CNOT"() %out_qubits_1#0, %out_qubits_0#1 : !quantum.bit, !quantum.bit
// CHECK: %mres, %out_qubit = quantum.measure %out_qubits_2#0 : i1, !quantum.bit
// CHECK: %mres_3, %out_qubit_4 = quantum.measure %out_qubits_2#1 : i1, !quantum.bit
// CHECK: %mres_5, %out_qubit_6 = quantum.measure %out_qubits_0#0 : i1, !quantum.bit
// CHECK: %mres_7, %out_qubit_8 = quantum.measure %out_qubits#1 : i1, !quantum.bit
// CHECK: %mres_9, %out_qubit_10 = quantum.measure %out_qubits_1#1 : i1, !quantum.bit
// CHECK: %6 = quantum.extract %0[ 0] : !quantum.reg -> !quantum.bit
// CHECK: %7 = quantum.insert %0[ 0], %out_qubit_10 : !quantum.reg, !quantum.bit
// CHECK: %8 = quantum.insert %7[ 1], %out_qubit_4 : !quantum.reg, !quantum.bit
// CHECK: %9 = quantum.insert %8[ 2], %out_qubit_8 : !quantum.reg, !quantum.bit
// CHECK: %10 = quantum.insert %9[ 3], %out_qubit_6 : !quantum.reg, !quantum.bit
// CHECK: %11 = quantum.insert %10[ 4], %out_qubit : !quantum.reg, !quantum.bit
