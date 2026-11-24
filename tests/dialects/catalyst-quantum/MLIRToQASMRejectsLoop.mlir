// RUN: %not %mqss-translate --mlir-to-qasm2 %s 2>&1 | FileCheck %s

// The QASM printer needs straight-line code. A loop with several iterations
// around quantum operations would otherwise be flattened to a single
// execution (the Hadamard printed once instead of three times), so it must be
// rejected rather than silently translated.

func.func @loop() -> i1 {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  %reg = quantum.alloc(1) : !quantum.reg
  %q0 = quantum.extract %reg[0] : !quantum.reg -> !quantum.bit
  %q = scf.for %i = %c0 to %c3 step %c1 iter_args(%qi = %q0) -> (!quantum.bit) {
    %qn = quantum.custom "Hadamard"() %qi : !quantum.bit
    scf.yield %qn : !quantum.bit
  }
  %m, %q2 = quantum.measure %q : i1, !quantum.bit
  return %m : i1
}

// CHECK: error: {{.*}}'scf.for' around quantum operations was not unrolled/folded
