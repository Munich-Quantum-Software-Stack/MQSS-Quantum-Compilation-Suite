
// RUN: %mqss-opt %s --CommonCommutePass=mode=CX-X 2>&1 | FileCheck %s

module {
  func.func @CommuteCNot_X_test() {

    %reg = quantum.alloc(2) : !quantum.reg

    %q0 = quantum.extract %reg[0] : !quantum.reg -> !quantum.bit
    %q1 = quantum.extract %reg[1] : !quantum.reg -> !quantum.bit

    %q1_1,%q1_2 = quantum.custom "CNOT"() %q0, %q1 : !quantum.bit, !quantum.bit
    %q1_3 = quantum.custom "PauliX"() %q1 : !quantum.bit

    // CHECK: %out_qubits = quantum.custom "PauliX"() %2 : !quantum.bit
    // CHECK: %out_qubits_0:2 = quantum.custom "CNOT"() %1, %2 : !quantum.bit, !quantum.bit
    // CHECK: %out_qubits_1:2 = quantum.custom "CNOT"() %out_qubits_0#1, %out_qubits : !quantum.bit, !quantum.bit
    // CHECK: %out_qubits_2 = quantum.custom "PauliX"() %out_qubits_0#1 : !quantum.bit


    %q0_1, %q0_2 = quantum.custom "CNOT"() %q1_2, %q1_3 : !quantum.bit, !quantum.bit

    %q1_4 = quantum.custom "PauliX"() %q1_2 : !quantum.bit

    %meas, %v = quantum.measure %q1_4 : i1, !quantum.bit

    return
  }
}
