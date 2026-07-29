from catalyst import qjit
import pennylane as qml

dev = qml.device("lightning.qubit", wires=2)

@qjit(keep_intermediate=True)
@qml.set_shots(1000)
@qml.qnode(dev)
def circuit_HZHToX():
    qml.Hadamard(wires=0)
    qml.PauliZ(wires=0)
    qml.Hadamard(wires=0)
    qml.Hadamard(wires=1)
    qml.CZ(wires=[0, 1])
    qml.Hadamard(wires=1)
    return qml.counts()
