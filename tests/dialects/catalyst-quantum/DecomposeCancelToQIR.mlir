// RUN: %mqss-opt %s --CommonCNOTReversePass --CommonGateCancellationPass=mode=CancelGate --convert-quantum-to-llvm --mlir-to-llvmIR -o /dev/null 2>&1 | FileCheck %s

// A three-stage pipeline: reverse the CNOT into an H-CX-H sandwich, cancel the
// adjacent Hadamard pair that forms on the target-output chain (where the
// circuit's own two Hadamards sit), then lower the simplified circuit all the
// way to QIR-flavoured LLVM IR. The control-output chain's own boundary H has
// no further consumer in this circuit but is still a real op, so it still
// lowers to a call even though nothing downstream reads its result. Pointer
// SSA numbers are matched loosely for the same reason as in bell_state.mlir.
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
// CHECK:   call ptr @__catalyst__rt__qubit_allocate_array(i64 2)
// CHECK:   call void @__catalyst__qis__H(ptr {{.*}}, ptr null)
// CHECK:   call void @__catalyst__qis__H(ptr {{.*}}, ptr null)
// CHECK:   call void @__catalyst__qis__CNOT(ptr {{.*}}, ptr {{.*}}, ptr null)
// CHECK:   call void @__catalyst__qis__H(ptr {{.*}}, ptr null)
// CHECK:   call void @__catalyst__qis__Hadamard(ptr {{.*}}, ptr null)
// CHECK:   call ptr @__catalyst__qis__Measure(ptr {{.*}}, i32 -1)
// CHECK:   ret void
// CHECK: }
