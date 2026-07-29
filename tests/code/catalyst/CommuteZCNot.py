

from catalyst import qjit
import pennylane as qml

dev = qml.device("lightning.qubit", wires=3)

@qjit(keep_intermediate=True)
@qml.set_shots(1000)
@qml.qnode(dev)
def circuit_CommuteZCNot():
    qml.PauliZ(wires=1)
    qml.RZ(2.4, wires=2)
    qml.CNOT(wires=[0, 1])
    qml.PauliZ(wires=1)
    qml.CNOT(wires=[1, 0])
    qml.PauliZ(wires=1)
    qml.RX(5.1416, wires=1)
    qml.CNOT(wires=[0, 1])
    return qml.counts()
