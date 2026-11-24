// RUN: %mqss-translate --mlir-to-qasm2 %s | FileCheck %s

// A rotation angle written as an expression of constants (here -pi / 2, via
// arith.negf and arith.divf) must be folded to a literal, not rejected as a
// "non-constant" angle.

func.func @fold() -> i1 {
  %pi = arith.constant 3.14159265358979 : f64
  %two = arith.constant 2.0 : f64
  %neg = arith.negf %pi : f64
  %half = arith.divf %neg, %two : f64
  %reg = quantum.alloc(1) : !quantum.reg
  %q0 = quantum.extract %reg[0] : !quantum.reg -> !quantum.bit
  %q1 = quantum.custom "RX"(%half) %q0 : !quantum.bit
  %m, %q2 = quantum.measure %q1 : i1, !quantum.bit
  return %m : i1
}

// CHECK: OPENQASM 2.0;
// CHECK: rx(-1.5707963267949) q[0];
