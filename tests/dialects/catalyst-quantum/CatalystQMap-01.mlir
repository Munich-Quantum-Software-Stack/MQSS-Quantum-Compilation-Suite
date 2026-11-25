
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

// CHECK: %[[OUT_QUBITS:[A-Za-z0-9_]+]]:2 = quantum.custom "CNOT"() %{{[A-Za-z0-9_]+}}, %{{[A-Za-z0-9_]+}} : !quantum.bit, !quantum.bit
// CHECK: %[[V3:[A-Za-z0-9_]+]] = quantum.extract %[[V0:[A-Za-z0-9_]+]][ 1] : !quantum.reg -> !quantum.bit
// CHECK: %[[V4:[A-Za-z0-9_]+]] = quantum.extract %[[V0]][ 2] : !quantum.reg -> !quantum.bit
// CHECK: %[[OUT_QUBITS_0:[A-Za-z0-9_]+]]:2 = quantum.custom "CNOT"() %[[V4]], %[[V3]] : !quantum.bit, !quantum.bit
// CHECK: %[[V5:[A-Za-z0-9_]+]] = quantum.extract %[[V0]][ 0] : !quantum.reg -> !quantum.bit
// CHECK: %[[OUT_QUBITS_1:[A-Za-z0-9_]+]]:2 = quantum.custom "SWAP"() %[[V5]], %[[OUT_QUBITS]]#0 : !quantum.bit, !quantum.bit
// CHECK: %[[OUT_QUBITS_2:[A-Za-z0-9_]+]]:2 = quantum.custom "CNOT"() %[[OUT_QUBITS_1]]#0, %[[OUT_QUBITS_0]]#1 : !quantum.bit, !quantum.bit
// CHECK: %{{[A-Za-z0-9_]+}}, %[[OUT_QUBIT:[A-Za-z0-9_]+]] = quantum.measure %[[OUT_QUBITS_2]]#0 : i1, !quantum.bit
// CHECK: %{{[A-Za-z0-9_]+}}, %[[OUT_QUBIT_4:[A-Za-z0-9_]+]] = quantum.measure %[[OUT_QUBITS_2]]#1 : i1, !quantum.bit
// CHECK: %{{[A-Za-z0-9_]+}}, %[[OUT_QUBIT_6:[A-Za-z0-9_]+]] = quantum.measure %[[OUT_QUBITS_0]]#0 : i1, !quantum.bit
// CHECK: %{{[A-Za-z0-9_]+}}, %[[OUT_QUBIT_8:[A-Za-z0-9_]+]] = quantum.measure %[[OUT_QUBITS]]#1 : i1, !quantum.bit
// CHECK: %{{[A-Za-z0-9_]+}}, %[[OUT_QUBIT_10:[A-Za-z0-9_]+]] = quantum.measure %[[OUT_QUBITS_1]]#1 : i1, !quantum.bit
// CHECK: %{{[A-Za-z0-9_]+}} = quantum.extract %[[V0]][ 0] : !quantum.reg -> !quantum.bit
// CHECK: %[[V7:[A-Za-z0-9_]+]] = quantum.insert %[[V0]][ 0], %[[OUT_QUBIT_10]] : !quantum.reg, !quantum.bit
// CHECK: %[[V8:[A-Za-z0-9_]+]] = quantum.insert %[[V7]][ 1], %[[OUT_QUBIT_4]] : !quantum.reg, !quantum.bit
// CHECK: %[[V9:[A-Za-z0-9_]+]] = quantum.insert %[[V8]][ 2], %[[OUT_QUBIT_8]] : !quantum.reg, !quantum.bit
// CHECK: %[[V10:[A-Za-z0-9_]+]] = quantum.insert %[[V9]][ 3], %[[OUT_QUBIT_6]] : !quantum.reg, !quantum.bit
// CHECK: %{{[A-Za-z0-9_]+}} = quantum.insert %[[V10]][ 4], %[[OUT_QUBIT]] : !quantum.reg, !quantum.bit
