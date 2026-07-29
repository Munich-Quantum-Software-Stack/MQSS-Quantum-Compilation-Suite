
// RUN: %mqss-opt %s --CommonSwitchPass=mode=HXYZtoXYZH 2>&1 | FileCheck %s

module {
  func.func @HadamardX_GateSwitch_test0() {

    %cst = arith.constant 3.140000e+00 : f64
    %_ = quantum.alloc( 2) : !quantum.reg
    %ZERO_0 = quantum.extract %_[ 0] : !quantum.reg -> !quantum.bit
    %ZERO_1 = quantum.extract %_[ 1] : !quantum.reg -> !quantum.bit

    %ONE_0 = quantum.custom "PauliX"() %ZERO_0 : !quantum.bit
    %PLUS_0 = quantum.custom "H"() %ZERO_0 : !quantum.bit
    %PLUS_1 = quantum.custom "H"() %ZERO_1 : !quantum.bit
    %ONE_1 = quantum.custom "PauliX"() %ZERO_1 : !quantum.bit

    // CHECK: %out_qubits_1 = quantum.custom "PauliZ"() %2 : !quantum.bit
    // CHECK: %out_qubits_2 = quantum.custom "H"() %2 : !quantum.bit


    %obs = quantum.compbasis qreg %_ : !quantum.obs

    return
  }

  func.func @HadamardY_GateSwitch_test1() {

    %q = quantum.alloc(2) : !quantum.reg

    %q0_0 = quantum.extract %q[0] : !quantum.reg -> !quantum.bit
    %q1_0 = quantum.extract %q[1] : !quantum.reg -> !quantum.bit

    %q0_1 = quantum.custom "PauliY"() %q0_0 : !quantum.bit
    %q0_2 = quantum.custom "H"() %q0_0 : !quantum.bit

    %q1_1 = quantum.custom "H"() %q1_0 : !quantum.bit
    %q1_2 = quantum.custom "PauliY"() %q1_0 : !quantum.bit

    // CHECK: %out_qubits_1 = quantum.custom "PauliY"() %2 : !quantum.bit
    // CHECK: %out_qubits_2 = quantum.custom "H"() %2 : !quantum.bit

    %meas, %v = quantum.measure %q1_1 : i1, !quantum.bit

    return
  }
  func.func @HadamardX_GateSwitch_test2() {

    %cst = arith.constant 3.140000e+00 : f64
    %_ = quantum.alloc( 2) : !quantum.reg
    %ZERO_0 = quantum.extract %_[ 0] : !quantum.reg -> !quantum.bit
    %ZERO_1 = quantum.extract %_[ 1] : !quantum.reg -> !quantum.bit

    %ONE_0 = quantum.custom "PauliZ"() %ZERO_0 : !quantum.bit
    %PLUS_0 = quantum.custom "H"() %ZERO_0 : !quantum.bit
    %PLUS_1 = quantum.custom "H"() %ZERO_1 : !quantum.bit
    %ONE_1 = quantum.custom "PauliZ"() %ZERO_1 : !quantum.bit

    // CHECK: %out_qubits_1 = quantum.custom "PauliX"() %2 : !quantum.bit
    // CHECK: %out_qubits_2 = quantum.custom "H"() %2 : !quantum.bit


    %obs = quantum.compbasis qreg %_ : !quantum.obs

    return
  }
}
