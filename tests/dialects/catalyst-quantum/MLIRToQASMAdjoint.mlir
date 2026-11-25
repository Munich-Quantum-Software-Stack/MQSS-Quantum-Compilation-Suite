// RUN: %mqss-translate --mlir-to-qasm3 %s | FileCheck %s

// The `adjoint` attribute must be honored: T and S become their inverses,
// rotations get a negated angle and self-inverse gates (Hadamard) are
// unchanged. A non-adjoint rotation keeps its angle.

func.func @adjoint() -> i1 {
  %c = arith.constant 0.5 : f64
  %reg = quantum.alloc(1) : !quantum.reg
  %q0 = quantum.extract %reg[0] : !quantum.reg -> !quantum.bit
  %q1 = quantum.custom "T"() %q0 {adjoint} : !quantum.bit
  %q2 = quantum.custom "S"() %q1 {adjoint} : !quantum.bit
  %q3 = quantum.custom "RX"(%c) %q2 {adjoint} : !quantum.bit
  %q4 = quantum.custom "RZ"(%c) %q3 : !quantum.bit
  %q5 = quantum.custom "Hadamard"() %q4 {adjoint} : !quantum.bit
  %m, %q6 = quantum.measure %q5 : i1, !quantum.bit
  return %m : i1
}

// CHECK: OPENQASM 3.0;
// CHECK: qubit[1] q;
// CHECK: tdg q[0];
// CHECK-NEXT: sdg q[0];
// CHECK-NEXT: rx(-0.5) q[0];
// CHECK-NEXT: rz(0.5) q[0];
// CHECK-NEXT: h q[0];
