// RUN: %mqss-opt %s --CommonCommutePass=mode=CX-RX | %mqss-translate --mlir-to-qir | FileCheck %s

module {
  func.func @QuantumToQIRConversion_test() {

    %cst = arith.constant 5.141600e+00 : f64
    %cst_0 = arith.constant 2.400000e+00 : f64

    %qreg = quantum.alloc(3) : !quantum.reg

    %q0 = quantum.extract %qreg[0] : !quantum.reg -> !quantum.bit
    %q1 = quantum.extract %qreg[1] : !quantum.reg -> !quantum.bit

    %q0_1, %q1_1 = quantum.custom "CNOT"() %q0, %q1 : !quantum.bit, !quantum.bit
    %q2 = quantum.extract %qreg[2] : !quantum.reg -> !quantum.bit
    %q2_1 = quantum.custom "RX"(%cst_0) %q1 : !quantum.bit

    %q1_2 = quantum.custom "PauliZ"() %q0 : !quantum.bit
    %q1_3, %q0_2 = quantum.custom "CNOT"() %q1_2, %q0_1 : !quantum.bit, !quantum.bit
    %q1_4 = quantum.custom "PauliZ"() %q1_3 : !quantum.bit
    %q0_3, %q1_5 = quantum.custom "CNOT"() %q0_2, %q1_4 : !quantum.bit, !quantum.bit
    %q1_6 = quantum.custom "RX"(%cst) %q1_5 : !quantum.bit
    %q1_7 = quantum.custom "PauliZ"() %q1_6 : !quantum.bit

    %mz0:2 = quantum.measure %q1_7 : i1, !quantum.bit

    return
  }
}


// CHECK: define void @QuantumToQIRConversion_test() {
// CHECK:   %[[V1:[A-Za-z0-9_]+]] = call ptr @__catalyst__rt__qubit_allocate_array(i64 3)
// CHECK:   %[[V2:[A-Za-z0-9_]+]] = call ptr @__catalyst__rt__array_get_element_ptr_1d(ptr %[[V1]], i64 0)
// CHECK:   %[[V3:[A-Za-z0-9_]+]] = load ptr, ptr %[[V2]], align 8
// CHECK:   %[[V4:[A-Za-z0-9_]+]] = call ptr @__catalyst__rt__array_get_element_ptr_1d(ptr %[[V1]], i64 1)
// CHECK:   %[[V5:[A-Za-z0-9_]+]] = load ptr, ptr %[[V4]], align 8
// CHECK:   %[[V6:[A-Za-z0-9_]+]] = call ptr @__catalyst__rt__array_get_element_ptr_1d(ptr %[[V1]], i64 2)
// CHECK:   %{{[A-Za-z0-9_]+}} = load ptr, ptr %[[V6]], align 8
// CHECK:   call void @__catalyst__qis__RX(double 2.400000e+00, ptr %[[V5]], ptr null)
// CHECK:   call void @__catalyst__qis__CNOT(ptr %[[V3]], ptr %[[V5]], ptr null)
// CHECK:   call void @__catalyst__qis__PauliZ(ptr %[[V3]], ptr null)
// CHECK:   call void @__catalyst__qis__CNOT(ptr %[[V3]], ptr %[[V3]], ptr null)
// CHECK:   call void @__catalyst__qis__PauliZ(ptr %[[V3]], ptr null)
// CHECK:   call void @__catalyst__qis__CNOT(ptr %[[V3]], ptr %[[V3]], ptr null)
// CHECK:   call void @__catalyst__qis__RX(double 5.141600e+00, ptr %[[V3]], ptr null)
// CHECK:  call void @__catalyst__qis__PauliZ(ptr %[[V3]], ptr null)
// CHECK:   %[[V8:[A-Za-z0-9_]+]] = call ptr @__catalyst__qis__Measure(ptr %[[V3]], i32 -1)
// CHECK:   %{{[A-Za-z0-9_]+}} = load i1, ptr %[[V8]], align 1
// CHECK:   ret void
// CHECK: }
