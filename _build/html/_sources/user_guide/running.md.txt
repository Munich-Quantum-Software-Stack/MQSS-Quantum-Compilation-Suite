# Running test circuits

The driver script for running example circuits (in c++/python) is ```mqss-cc```.
After the targets are generated this script is installed within the ```INSTALL_DIR```
and should be on the ```$PATH``` environment variable fo your bash shell.
Check by running the command:

```sh
$mqss-cc -h
```

If nothing prints, run the command ```eval "$(make set-target-paths)"``` from the ```root```
directory once again.

## C++ test circuits

The compilation suite accepts c++ circuits written using [cudaq](https://github.com/NVIDIA/cuda-quantum).
Following is an example:

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

To run the above test circuit using ```mqss-cc``` use the following command:

```bash
mqss-cc test.cpp --emit-qir --out-dir output/ --passes=CommonCommutePass=mode=CX-RX
```

The ```Commutation Optimization pass``` is applied to commute ```CNOT and RX``` gates.
The output in this case will be QIR since the ```emit-qir``` flag is enabled.

Note: One can use cudaq to write quantum circuits in python. But this is not supported
      yet within ```mqss-cc```.

## Python test circuits

The compilation suite accepts python circuits written using [catalyst](https://github.com/PennyLaneAI/catalyst).
Following is an example:

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
mqss-cc test.py --function circuit_CommuteCNOTRx --out-dir output/ --passes=CommonGateCancellationPass=mode=CancelGate
```

Do Not forget to mention the function to compile afer the ```--function``` flag.
Refer to [Passes](passes.md) for a list of all available MLIR passes.
