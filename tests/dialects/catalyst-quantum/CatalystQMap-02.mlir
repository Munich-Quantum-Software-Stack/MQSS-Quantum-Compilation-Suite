
// RUN: %mqss-opt %s --CommonMappingPass=qdmi=cxx_qdmi.conf 2>&1  | sed -E 's/\x1b\[[0-9;]*m//g' | FileCheck %s

func.func @circuit_qmap2() attributes {qnode} {
  %cst   = arith.constant 2.250000e+00 : f64
  %cst_0 = arith.constant 3.141600e+00 : f64
  %cst_1 = arith.constant 1.500000e+00 : f64

  %0 = quantum.alloc(5) : !quantum.reg

  %q0 = quantum.extract %0[0] : !quantum.reg -> !quantum.bit
  %q1 = quantum.extract %0[1] : !quantum.reg -> !quantum.bit
  %q2 = quantum.extract %0[2] : !quantum.reg -> !quantum.bit
  %q3 = quantum.extract %0[3] : !quantum.reg -> !quantum.bit
  %q4 = quantum.extract %0[4] : !quantum.reg -> !quantum.bit

  %cnot1_out:2 = quantum.custom "CNOT"() %q4, %q2 : !quantum.bit, !quantum.bit
  %cnot2_out:2 = quantum.custom "CNOT"() %q3, %q1 : !quantum.bit, !quantum.bit
  %cnot3_out:2 = quantum.custom "CNOT"() %cnot1_out#0, %cnot2_out#1 : !quantum.bit, !quantum.bit

  %rx_out  = quantum.custom "RX"(%cst_1) %cnot3_out#1 : !quantum.bit
  %ry_out  = quantum.custom "RY"(%cst_0) %cnot1_out#1 : !quantum.bit
  %rz_out  = quantum.custom "RZ"(%cst)   %cnot2_out#0 : !quantum.bit
  %t_out   = quantum.custom "T"()        %cnot3_out#0 : !quantum.bit

  %mz0:2 = quantum.measure %q0    : i1, !quantum.bit
  %mz1:2 = quantum.measure %rx_out : i1, !quantum.bit
  %mz2:2 = quantum.measure %ry_out : i1, !quantum.bit
  %mz3:2 = quantum.measure %rz_out : i1, !quantum.bit
  %mz4:2 = quantum.measure %t_out  : i1, !quantum.bit

  %r0 = quantum.insert %0[0],  %mz0#1 : !quantum.reg, !quantum.bit
  %r1 = quantum.insert %r0[1], %mz1#1 : !quantum.reg, !quantum.bit
  %r2 = quantum.insert %r1[2], %mz2#1 : !quantum.reg, !quantum.bit
  %r3 = quantum.insert %r2[3], %mz3#1 : !quantum.reg, !quantum.bit
  %r4 = quantum.insert %r3[4], %mz4#1 : !quantum.reg, !quantum.bit

  quantum.dealloc %r4 : !quantum.reg
  func.return
}

// CHECK: %0 = quantum.alloc( 5) : !quantum.reg
// CHECK: %1 = quantum.extract %0[ 3] : !quantum.reg -> !quantum.bit
// CHECK: %2 = quantum.extract %0[ 4] : !quantum.reg -> !quantum.bit
// CHECK: %out_qubits:2 = quantum.custom "CNOT"() %2, %1 : !quantum.bit, !quantum.bit
// CHECK: %3 = quantum.extract %0[ 1] : !quantum.reg -> !quantum.bit
// CHECK: %4 = quantum.extract %0[ 2] : !quantum.reg -> !quantum.bit
// CHECK: %out_qubits_0:2 = quantum.custom "CNOT"() %4, %3 : !quantum.bit, !quantum.bit
// CHECK: %5 = quantum.extract %0[ 0] : !quantum.reg -> !quantum.bit
// CHECK: %6 = quantum.extract %0[ 4] : !quantum.reg -> !quantum.bit
// CHECK: %out_qubits_1:2 = quantum.custom "SWAP"() %5, %6 : !quantum.bit, !quantum.bit
// CHECK: %7 = quantum.extract %0[ 1] : !quantum.reg -> !quantum.bit
// CHECK: %8 = quantum.extract %0[ 0] : !quantum.reg -> !quantum.bit
// CHECK: %out_qubits_2:2 = quantum.custom "CNOT"() %8, %7 : !quantum.bit, !quantum.bit
// CHECK: %9 = quantum.extract %0[ 3] : !quantum.reg -> !quantum.bit
// CHECK: %cst = arith.constant 3.141600e+00 : f64
// CHECK: %out_qubits_3 = quantum.custom "RY"(%cst) %9 : !quantum.bit
// CHECK: %10 = quantum.extract %0[ 2] : !quantum.reg -> !quantum.bit
// CHECK: %cst_4 = arith.constant 2.250000e+00 : f64
// CHECK: %out_qubits_5 = quantum.custom "RZ"(%cst_4) %10 : !quantum.bit
// CHECK: %11 = quantum.extract %0[ 1] : !quantum.reg -> !quantum.bit
// CHECK: %cst_6 = arith.constant 1.500000e+00 : f64
// CHECK: %out_qubits_7 = quantum.custom "RX"(%cst_6) %11 : !quantum.bit
// CHECK: %12 = quantum.extract %0[ 0] : !quantum.reg -> !quantum.bit
// CHECK: %out_qubits_8 = quantum.custom "T"() %12 : !quantum.bit