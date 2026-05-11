

from catalyst import qjit
import pennylane as qml

dev = qml.device("lightning.qubit", wires=3)

@qjit(keep_intermediate=True)
@qml.set_shots(1000)
@qml.qnode(dev)
def circuit_CommuteRXCNOT():
    # rx(2.4, q[1])  →  RX rotation on wire 1
    qml.RX(2.4, wires=1)

    # x(q[2])  →  Pauli-X on wire 2
    qml.PauliX(wires=2)

    # x<cudaq::ctrl>(q[0], q[1])  →  CNOT with control=0, target=1
    qml.CNOT(wires=[0, 1])

    # rx(3.1416, q[1])  →  RX rotation on wire 1
    qml.RX(3.1416, wires=1)

    # x<cudaq::ctrl>(q[1], q[0])  →  CNOT with control=1, target=0
    qml.CNOT(wires=[1, 0])

    # rx(5.1416, q[1])  →  RX rotation on wire 1
    qml.RX(5.1416, wires=1)

    # x(q[1])  →  Pauli-X on wire 1
    qml.PauliX(wires=1)

    # x<cudaq::ctrl>(q[0], q[1])  →  CNOT with control=0, target=1
    qml.CNOT(wires=[0, 1])

    # mz(q)  →  measure all qubits in Z-basis, return counts
    return qml.counts()
