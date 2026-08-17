<!--------------------------------------------------------------------------------------------------
Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
----------------------------------------------------------------------------------------------------->

# Running test circuits

## Using mqss-opt

The binary `mqss-opt` is used to invoke passes on input MLIR dialects. To check the list of
available passes Run:

```sh
mqss-opt -h
```

Make sure `mqss-opt` is on the`$PATH` environment variable of your shell. If it isn't Run:
`eval "$(make set-target-paths)"`.

### MLIR dialect input (quake or catalyst-quantum)

Here is the Bell-State circuit in the Quake MLIR dialect:

```llvm
module attributes {cc.sizeof_string = 24 : i64, llvm.data_layout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128", llvm.triple = "aarch64-unknown-linux-gnu", quake.mangled_name_map = {__nvqpp__mlirgen__bellILm2EE = "_ZN4bellILm2EEclEv"}} {
  func.func @__nvqpp__mlirgen__bellILm2EE() attributes {"cudaq-entrypoint", "cudaq-kernel"} {
    %0 = quake.alloca !quake.veq<2>
    %1 = quake.extract_ref %0[0] : (!quake.veq<2>) -> !quake.ref
    quake.h %1 : (!quake.ref) -> ()
    %2 = quake.extract_ref %0[0] : (!quake.veq<2>) -> !quake.ref
    %3 = quake.extract_ref %0[1] : (!quake.veq<2>) -> !quake.ref
    quake.x [%2] %3 : (!quake.ref, !quake.ref) -> ()
    %q0 = quake.extract_ref %0[0] : (!quake.veq<2>) -> !quake.ref
    %q1 = quake.extract_ref %0[1] : (!quake.veq<2>) -> !quake.ref
    %m0 = quake.mz %q0 : (!quake.ref) -> !quake.measure
    %m1 = quake.mz %q1 : (!quake.ref) -> !quake.measure
    return
  }
}
```

Following command shows an example of how passes can be invoked on this circuit (assuming the
dialect is saved as bell_state.qke):

```sh
mqss-opt bell-state.qke --cse --canonicalize --BasisConversionPass=gates=phased_rx,cz
```

The output is the same input `bell-state.qke` dialect but with transformations. In this case, the
hadamard and CNOT gates in the input dialect `quake.h` and `quake.x` will be decomposed to the
`phased_rx` and `cz` gates. Similarly, one can invoke passes on the catalyst-quantum mlir dialect by
just replacing the quake dialect input with the catalyst-quantum input.

## Using mqss-cc script (Frontend test)

Note: Before running a Frontend test, make sure you follow the installation instructions within
[build](build.md).</br> The driver script for running example circuits (in c++/python) is `mqss-cc`.
After the targets are generated this script is installed within the `INSTALL_DIR` and should be on
the `$PATH` environment variable for your bash shell. Check by running the command:

```sh
$mqss-cc -h
```

If nothing prints, run the command `eval "$(make set-target-paths)"` from the `root` directory once
again.

### C++ test circuits

The compilation suite accepts c++ circuits written using
[cudaq](https://github.com/NVIDIA/cuda-quantum). Following is an example:

```c++
#include <cudaq.h>
#include <fstream>
#include <iostream>

template <std::size_t N> struct test {
  auto operator()() __qpu__ {

    // Compile-time sized array like std::array
    cudaq::qarray<N> q;
    x<cudaq::ctrl>(q[0], q[1]);
    x(q[2]);
    rx(2.4, q[1]);

    x<cudaq::ctrl>(q[1], q[0]);
    rx(3.1416, q[1]);

    x<cudaq::ctrl>(q[0], q[1]);
    x(q[1]);
    rx(5.1416, q[1]);

    mz(q[0]);
    mz(q[1]);
  }
};

int main() {
  auto kernel = test<3>{};
  auto counts = cudaq::sample(kernel);
  counts.dump();
  return 0;
}

```

To run the above test circuit using `mqss-cc` use the following command:

```bash
mqss-cc test.cpp --out-dir output/ --passes=CommonGateCancellationPass=mode=CancelGate
```

The `Commutation Optimization pass` is applied to commute `CNOT and RX` gates. The output in this
case will be QIR since the `emit-qir` flag is enabled.

Note: One can use cudaq to write quantum circuits in python. But this is not supported yet within
`mqss-cc`.

### Python test circuits

The compilation suite accepts python circuits written using
[catalyst](https://github.com/PennyLaneAI/catalyst). Following is an example:

```python

from catalyst import qjit
import pennylane as qml

dev = qml.device("lightning.qubit", wires=3)

@qjit(keep_intermediate=True)
@qml.set_shots(1000)
@qml.qnode(dev)
def circuit_CommuteCNOTRx():
    qml.CNOT(wires=[0, 1])
    qml.PauliX(wires=2)
    qml.RX(2.4, wires=1)

    qml.CNOT(wires=[1, 0])
    qml.RX(3.1416, wires=1)

    qml.CNOT(wires=[0, 1])
    qml.PauliX(wires=1)
    qml.RX(5.1416, wires=1)

    return qml.counts()

```

```bash
mqss-cc test.py --function circuit \
           --out-dir output/ --passes=CommonGateCancellationPass=mode=CancelGate
```

Do Not forget to mention the function to compile after the `--function` flag. Refer to
[Passes](passes.md) for a list of all available MLIR passes.
