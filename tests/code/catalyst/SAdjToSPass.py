from catalyst import qjit
import pennylane as qml

dev = qml.device("lightning.qubit", wires=2)

@qjit(keep_intermediate=True)
@qml.set_shots(1000)
@qml.qnode(dev)
def circuit_SAdjZToS():
    qml.adjoint(qml.S)(wires=0)
    qml.PauliZ(wires=0)
    qml.S(wires=1)
    return qml.counts()