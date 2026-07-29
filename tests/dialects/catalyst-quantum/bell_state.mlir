// RUN: %mqss-opt %s --CommonDecompositionPass=mode=HToRzXRz --convert-quantum-to-llvm --mlir-to-llvmIR -o /dev/null 2>&1 | FileCheck %s

func.func @bell() -> (i1, i1) {
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

// CHECK: call void @__catalyst__qis__RZ(double 0x400921FB54442D18, ptr %3, ptr null)
// CHECK: call void @__catalyst__qis__PauliX(ptr %3, ptr null)
// CHECK: call void @__catalyst__qis__RZ(double 0x3FF921FB54442D18, ptr %3, ptr null)
// CHECK: %4 = call ptr @__catalyst__rt__array_get_element_ptr_1d(ptr %1, i64 1)
// CHECK: %5 = load ptr, ptr %4, align 8
// CHECK: call void @__catalyst__qis__CNOT(ptr %3, ptr %5, ptr null)
