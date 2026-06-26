

from catalyst import qjit
import pennylane as qml

dev = qml.device("lightning.qubit", wires=2)

@qjit(keep_intermediate=True)
@qml.set_shots(1000)
@qml.qnode(dev)
def circuit_ReverseCx():
    qml.PauliZ(wires=1)
    qml.PauliZ(wires=0)
    qml.CNOT(wires=[1, 0])
    qml.PauliY(wires=1)
    qml.PauliY(wires=0)
    return qml.counts()