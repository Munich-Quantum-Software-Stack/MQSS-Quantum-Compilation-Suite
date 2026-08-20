// RUN: %mqss-opt %s --CommonDecompositionPass=mode=CxToHCzH --CommonGateCancellationPass=mode=CancelGate --convert-quantum-to-llvm --mlir-to-llvmIR -o /dev/null 2>&1 | FileCheck %s

// A three-stage pipeline: decompose CNOT to H-CZ-H, cancel the resulting run of three
// consecutive Hadamards down to one, then lower the simplified circuit all the way to
// QIR-flavoured LLVM IR.
module {
  func.func @DecomposeCancelToQIR_test() {
    %qreg = quantum.alloc(2) : !quantum.reg
    %q0 = quantum.extract %qreg[0] : !quantum.reg -> !quantum.bit
    %q1 = quantum.extract %qreg[1] : !quantum.reg -> !quantum.bit
    %q0_1, %q1_1 = quantum.custom "CNOT"() %q0, %q1 : !quantum.bit, !quantum.bit
    %q1_2 = quantum.custom "Hadamard"() %q1_1 : !quantum.bit
    %q1_3 = quantum.custom "Hadamard"() %q1_2 : !quantum.bit
    %mz0:2 = quantum.measure %q1_3 : i1, !quantum.bit
    return
  }
}

// CHECK: define void @DecomposeCancelToQIR_test() {
// CHECK:   %1 = call ptr @__catalyst__rt__qubit_allocate_array(i64 2)
// CHECK:   %2 = call ptr @__catalyst__rt__array_get_element_ptr_1d(ptr %1, i64 0)
// CHECK:   %3 = load ptr, ptr %2, align 8
// CHECK:   %4 = call ptr @__catalyst__rt__array_get_element_ptr_1d(ptr %1, i64 1)
// CHECK:   %5 = load ptr, ptr %4, align 8
// CHECK:   call void @__catalyst__qis__H(ptr %5, ptr null)
// CHECK:   call void @__catalyst__qis__CZ(ptr %3, ptr %5, ptr null)
// CHECK:   call void @__catalyst__qis__Hadamard(ptr %5, ptr null)
// CHECK:   %6 = call ptr @__catalyst__qis__Measure(ptr %5, i32 -1)
// CHECK:   %7 = load i1, ptr %6, align 1
// CHECK:   ret void
// CHECK: }
