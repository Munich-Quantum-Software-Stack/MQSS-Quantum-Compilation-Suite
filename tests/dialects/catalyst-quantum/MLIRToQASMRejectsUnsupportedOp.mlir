// RUN: %not %mqss-translate --mlir-to-qasm2 %s 2>&1 | FileCheck %s

// A quantum operation with no OpenQASM equivalent (here a global phase) used
// to be skipped silently, producing a circuit that was wrong with no error.
// It must now be rejected at the operation.

func.func @unsupported_op() -> i1 {
  %c = arith.constant 0.5 : f64
  %reg = quantum.alloc(1) : !quantum.reg
  %q0 = quantum.extract %reg[0] : !quantum.reg -> !quantum.bit
  quantum.gphase(%c)
  %q1 = quantum.custom "Hadamard"() %q0 : !quantum.bit
  %m, %q2 = quantum.measure %q1 : i1, !quantum.bit
  return %m : i1
}

// CHECK: error: Mapping: operation 'quantum.gphase' has no OpenQASM equivalent
// CHECK: error: MLIRToQASM: Mapping: unsupported operation
