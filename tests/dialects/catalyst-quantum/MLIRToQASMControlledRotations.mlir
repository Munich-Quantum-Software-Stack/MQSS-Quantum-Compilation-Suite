// RUN: %mqss-translate --mlir-to-qasm2 %s | FileCheck %s

// Controlled rotations must all be emitted. CRY and CRZ used to be dropped
// silently because only CRX had a case in the gate loader.

func.func @controlled_rotations() -> (i1, i1) {
  %c = arith.constant 0.25 : f64
  %reg = quantum.alloc(2) : !quantum.reg
  %a0 = quantum.extract %reg[0] : !quantum.reg -> !quantum.bit
  %b0 = quantum.extract %reg[1] : !quantum.reg -> !quantum.bit
  %a1, %b1 = quantum.custom "CRX"(%c) %a0, %b0 : !quantum.bit, !quantum.bit
  %a2, %b2 = quantum.custom "CRY"(%c) %a1, %b1 : !quantum.bit, !quantum.bit
  %a3, %b3 = quantum.custom "CRZ"(%c) %a2, %b2 : !quantum.bit, !quantum.bit
  %m0, %a4 = quantum.measure %a3 : i1, !quantum.bit
  %m1, %b4 = quantum.measure %b3 : i1, !quantum.bit
  return %m0, %m1 : i1, i1
}

// CHECK: OPENQASM 2.0;
// CHECK: qreg q[2];
// CHECK: crx(0.25) q[0], q[1];
// CHECK-NEXT: cry(0.25) q[0], q[1];
// CHECK-NEXT: crz(0.25) q[0], q[1];
