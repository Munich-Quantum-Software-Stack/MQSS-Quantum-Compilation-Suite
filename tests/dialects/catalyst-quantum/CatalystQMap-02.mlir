
// RUN: %mqss-catalyst-opt %s --CommonMappingPass 2>&1  | sed -E 's/\x1b\[[0-9;]*m//g' | FileCheck %s

func.func @circuit() attributes {qnode} {
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

// CHECK: i:   0   1   2   3   4
// CHECK: 1:   |   |   |   x   c
// CHECK: 2:   |   x   c   |   |
// CHECK: 3:  sw   |   |   |  sw
// CHECK: 4:   c   x   |   |   |
// CHECK: 5:   |   |   |  ry   |  p: (3.1415999999999999481) 
// CHECK: 6:   |   |  rz   |   |  p: (2.25) 
// CHECK: 7:   |  rx   |   |   |  p: (1.5) 
// CHECK: 8:   t   |   |   |   |
// CHECK: 9:====================
// CHECK: 10:   4   |   |   |   |
// CHECK: 11:   |   1   |   |   |
// CHECK: 12:   |   |   3   |   |
// CHECK: 13:   |   |   |   2   |
// CHECK: 14:   |   |   |   |   0
// CHECK:  o:   4   1   3   2   0