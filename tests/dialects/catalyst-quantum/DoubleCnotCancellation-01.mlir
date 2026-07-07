
// RUN: %mqss-opt %s --CommonGateCancellationPass=mode=CancelGate 2>&1 | FileCheck %s

module {
  func.func @cnot_cancel_test() {
    %reg = quantum.alloc(2) : !quantum.reg

    %q0 = quantum.extract %reg[0] : !quantum.reg -> !quantum.bit
    %q1 = quantum.extract %reg[1] : !quantum.reg -> !quantum.bit

    // First CNOT
    %q1_1, %q2_2 = quantum.custom "CNOT"() %q0, %q1 {gate_name = "CNOT"} : !quantum.bit, !quantum.bit

    // Second CNOT (same qubits)
    %q1_3, %q2_4 = quantum.custom "CNOT"() %q0, %q1 {gate_name = "CNOT"} : !quantum.bit, !quantum.bit

    // Operation with side-effects
    %v, %out = quantum.measure %q0 : i1, !quantum.bit

    return
  }
}

  // CHECK: %1 = quantum.extract %0[ 0] : !quantum.reg -> !quantum.bit
  // CHECK: %2 = quantum.extract %0[ 1] : !quantum.reg -> !quantum.bit
  // CHECK: %mres, %out_qubit = quantum.measure %1 : i1, !quantum.bit