
// RUN: mqss-opt %s --BasisConversionPass=gates=prx,cz --cse --canonicalize --catalyst-quantum-to-qasm=mode=qasm3 -o /dev/null 2>&1 | FileCheck %s

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

// CHECK: OPENQASM 3.0;
// CHECK: include "stdgates.inc";
// CHECK: gate r(param0,param1) q0 { U(param0,-pi/2 + param1,pi/2 - param1) q0; }
// CHECK: qubit[3] q;
// CHECK: bit[3] c;
// CHECK: r(1.5707963267949,1.5707963267949) q[0];
// CHECK: r(3.14159265358979,0) q[0];
// CHECK: r(1.5707963267949,1.5707963267949) q[1];
// CHECK: r(3.14159265358979,0) q[1];
// CHECK: cz q[0], q[1];
// CHECK: r(1.5707963267949,1.5707963267949) q[1];
// CHECK: r(3.14159265358979,0) q[1];
// CHECK: r(1.5707963267949,1.5707963267949) q[2];
// CHECK: r(3.14159265358979,0) q[2];
// CHECK: cz q[1], q[2];
// CHECK: r(1.5707963267949,1.5707963267949) q[2];
// CHECK: r(3.14159265358979,0) q[2];
// CHECK: c[0] = measure q[0];
// CHECK: c[1] = measure q[1];
// CHECK: c[2] = measure q[2];
