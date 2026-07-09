
// RUN: %mqss-catalyst-opt %s --CommonMappingPass=input=/workspaces/MQSS-Passes-Suite/tests/input/qmap.json 2>&1  | sed -E 's/\x1b\[[0-9;]*m//g' | FileCheck %s

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

// CHECK: %out_qubits_0:2 = quantum.custom "CNOT"() %4, %3 : !quantum.bit, !quantum.bit
// CHECK: %5 = quantum.extract %0[ 0] : !quantum.reg -> !quantum.bit
// CHECK: %6 = quantum.extract %0[ 4] : !quantum.reg -> !quantum.bit
// CHECK: %out_qubits_1:2 = quantum.custom "SWAP"() %5, %6 : !quantum.bit, !quantum.bit
// CHECK: %7 = quantum.extract %0[ 1] : !quantum.reg -> !quantum.bit
// CHECK: %8 = quantum.extract %0[ 0] : !quantum.reg -> !quantum.bit
// CHECK: %out_qubits_2:2 = quantum.custom "CNOT"() %8, %7 : !quantum.bit, !quantum.bit