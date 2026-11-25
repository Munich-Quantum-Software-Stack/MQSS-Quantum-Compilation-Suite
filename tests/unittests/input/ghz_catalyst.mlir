// A 3-qubit GHZ circuit in the Catalyst quantum dialect: H on qubit 0, then
// CNOT(0 -> 1) and CNOT(1 -> 2), then a measurement of every qubit.

func.func @ghz() -> (i1, i1, i1) {
  // Allocate a 3-qubit register.
  %reg = quantum.alloc(3) : !quantum.reg

  // H on qubit 0.
  %q0_0 = quantum.extract %reg[0] : !quantum.reg -> !quantum.bit
  %q0_1 = quantum.custom "Hadamard"() %q0_0 : !quantum.bit

  // CNOT(0 -> 1): consumes q0_1 and q1_0, produces two new values.
  %q1_0 = quantum.extract %reg[1] : !quantum.reg -> !quantum.bit
  %q0_2, %q1_1 = quantum.custom "CNOT"() %q0_1, %q1_0 : !quantum.bit, !quantum.bit

  // CNOT(1 -> 2): uses the updated qubit 1.
  %q2_0 = quantum.extract %reg[2] : !quantum.reg -> !quantum.bit
  %q1_2, %q2_1 = quantum.custom "CNOT"() %q1_1, %q2_0 : !quantum.bit, !quantum.bit

  // Measure all three; each measurement consumes and returns the qubit.
  %m0, %q0_3 = quantum.measure %q0_2 : i1, !quantum.bit
  %m1, %q1_3 = quantum.measure %q1_2 : i1, !quantum.bit
  %m2, %q2_2 = quantum.measure %q2_1 : i1, !quantum.bit

  return %m0, %m1, %m2 : i1, i1, i1
}
