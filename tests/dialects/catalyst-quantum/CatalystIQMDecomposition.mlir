// RUN: %mqss-opt %s --BasisConversionPass=gates=prx,cz --cse --canonicalize | %mqss-translate --convert-quantum-to-llvm | %mqss-translate --llvm-dialect-to-llvm-ir | FileCheck %s


func.func @bell_to_iqm() -> (i1, i1) {
  // Allocate a 2-qubit register.
  %reg = quantum.alloc(2) : !quantum.reg

  // Extract qubit 0, apply Hadamard, get back the updated qubit value.
  %q0_0 = quantum.extract %reg[0] : !quantum.reg -> !quantum.bit
  %q0_1 = quantum.custom "Hadamard"() %q0_0 : !quantum.bit

  // Extract qubit 1; apply CNOT with q0 as control, q1 as target.
  // Value semantics: CNOT consumes both qubits and produces two new values.
  %q1_0 = quantum.extract %reg[1] : !quantum.reg -> !quantum.bit
  %q0_2, %q1_1 = quantum.custom "CNOT"() %q0_1, %q1_0 : !quantum.bit, !quantum.bit

  // Measure both qubits. Each measurement consumes and returns the qubit.
  %m0, %q0_3 = quantum.measure %q0_2 : i1, !quantum.bit
  %m1, %q1_2 = quantum.measure %q1_1 : i1, !quantum.bit

  return %m0, %m1 : i1, i1
}

// CHECK: call void @__catalyst__qis__PhasedRx(double 0x3FF921FB54442D18, double 0x3FF921FB54442D18, ptr %[[V3:[A-Za-z0-9_]+]], ptr null)
// CHECK: call void @__catalyst__qis__PhasedRx(double 0x400921FB54442D18, double 0.000000e+00, ptr %[[V3]], ptr null)
// CHECK: %[[V4:[A-Za-z0-9_]+]] = call ptr @__catalyst__rt__array_get_element_ptr_1d(ptr %{{[A-Za-z0-9_]+}}, i64 1)
// CHECK: %[[V5:[A-Za-z0-9_]+]] = load ptr, ptr %[[V4]], align 8
// CHECK: call void @__catalyst__qis__PhasedRx(double 0x3FF921FB54442D18, double 0x3FF921FB54442D18, ptr %[[V5]], ptr null)
// CHECK: call void @__catalyst__qis__PhasedRx(double 0x400921FB54442D18, double 0.000000e+00, ptr %[[V5]], ptr null)
// CHECK: call void @__catalyst__qis__CZ(ptr %[[V3]], ptr %[[V5]], ptr null)
// CHECK: call void @__catalyst__qis__PhasedRx(double 0x3FF921FB54442D18, double 0x3FF921FB54442D18, ptr %[[V5]], ptr null)
// CHECK: call void @__catalyst__qis__PhasedRx(double 0x400921FB54442D18, double 0.000000e+00, ptr %[[V5]], ptr null)
// CHECK: %[[V6:[A-Za-z0-9_]+]] = call ptr @__catalyst__qis__Measure(ptr %[[V3]], i32 -1)
// CHECK: %{{[A-Za-z0-9_]+}} = load i1, ptr %[[V6]], align 1
// CHECK: %{{[A-Za-z0-9_]+}} = call ptr @__catalyst__qis__Measure(ptr %[[V5]], i32 -1)
