

from catalyst import qjit
import pennylane as qml

dev = qml.device("lightning.qubit", wires=3)

@qjit(keep_intermediate=True)
@qml.set_shots(1000)
@qml.qnode(dev)
def circuit_CommuteCNotZ():
    # x<cudaq::ctrl>(q[0], q[1])  →  CNOT with control=0, target=1
    qml.CNOT(wires=[0, 1])

    # rz(2.4, q[2])  →  RZ rotation on wire 2
    qml.RZ(2.4, wires=2)

    # z(q[1])  →  Pauli-Z on wire 1
    qml.PauliZ(wires=1)

    # x<cudaq::ctrl>(q[1], q[0])  →  CNOT with control=1, target=0
    qml.CNOT(wires=[1, 0])

    # z(q[1])  →  Pauli-Z on wire 1
    qml.PauliZ(wires=1)

    # x<cudaq::ctrl>(q[0], q[1])  →  CNOT with control=0, target=1
    qml.CNOT(wires=[0, 1])

    # rx(5.1416, q[1])  →  RX rotation on wire 1
    qml.RX(5.1416, wires=1)

    # z(q[1])  →  Pauli-Z on wire 1
    qml.PauliZ(wires=1)

    # mz(q)  →  measure all qubits in Z-basis, return counts
    return qml.counts()