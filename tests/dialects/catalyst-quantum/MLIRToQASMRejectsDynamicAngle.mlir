// RUN: %not %mqss-translate --mlir-to-qasm2 %s 2>&1 | FileCheck %s

// A rotation angle that is a function argument is not a compile-time
// constant, so it cannot be written into OpenQASM. The translation must fail
// with a diagnostic at the offending gate instead of printing a wrong angle.

func.func @dynamic_angle(%theta: f64) -> i1 {
  %reg = quantum.alloc(1) : !quantum.reg
  %q0 = quantum.extract %reg[0] : !quantum.reg -> !quantum.bit
  %q1 = quantum.custom "RX"(%theta) %q0 : !quantum.bit
  %m, %q2 = quantum.measure %q1 : i1, !quantum.bit
  return %m : i1
}

// CHECK: error: Mapping: No constant angle found to create RX gate!
// CHECK: error: MLIRToQASM: Mapping: Error when loading GateOp into MQT's QuantumComputation
