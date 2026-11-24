// RUN: %mqss-opt %s --CommonMappingPass --cse --canonicalize | FileCheck %s

// Regression test: Catalyst has value semantics, so after mapping every gate
// and measurement must consume the SSA value its predecessor produced for that
// qubit. The mapper used to extract a fresh qubit for every operation, which
// left each gate's results unused; `quantum.custom` has no side effects, so
// the following canonicalize deleted all the gates and only the measurements
// survived. Each returned bit must also come from its own measurement.

func.func @ghz() -> (i1, i1, i1) {
  %reg = quantum.alloc(3) : !quantum.reg

  %q0_0 = quantum.extract %reg[0] : !quantum.reg -> !quantum.bit
  %q0_1 = quantum.custom "Hadamard"() %q0_0 : !quantum.bit

  %q1_0 = quantum.extract %reg[1] : !quantum.reg -> !quantum.bit
  %q0_2, %q1_1 = quantum.custom "CNOT"() %q0_1, %q1_0 : !quantum.bit, !quantum.bit

  %q2_0 = quantum.extract %reg[2] : !quantum.reg -> !quantum.bit
  %q1_2, %q2_1 = quantum.custom "CNOT"() %q1_1, %q2_0 : !quantum.bit, !quantum.bit

  %m0, %q0_3 = quantum.measure %q0_2 : i1, !quantum.bit
  %m1, %q1_3 = quantum.measure %q1_2 : i1, !quantum.bit
  %m2, %q2_2 = quantum.measure %q2_1 : i1, !quantum.bit

  return %m0, %m1, %m2 : i1, i1, i1
}

// CHECK: quantum.alloc(
// CHECK: [[H:%[a-z_0-9]+]] = quantum.custom "H"() %{{[a-z_0-9]+}} : !quantum.bit
// CHECK: [[CX1:%[a-z_0-9]+]]:2 = quantum.custom "CNOT"() [[H]], %{{[a-z_0-9]+}} : !quantum.bit, !quantum.bit
// CHECK: [[CX2:%[a-z_0-9]+]]:2 = quantum.custom "CNOT"() [[CX1]]#1, %{{[a-z_0-9]+}} : !quantum.bit, !quantum.bit
// CHECK: [[M0:%[a-z_0-9]+]], %{{[a-z_0-9]+}} = quantum.measure [[CX1]]#0 : i1, !quantum.bit
// CHECK: [[M1:%[a-z_0-9]+]], %{{[a-z_0-9]+}} = quantum.measure [[CX2]]#0 : i1, !quantum.bit
// CHECK: [[M2:%[a-z_0-9]+]], %{{[a-z_0-9]+}} = quantum.measure [[CX2]]#1 : i1, !quantum.bit
// CHECK: return [[M0]], [[M1]], [[M2]] : i1, i1, i1
