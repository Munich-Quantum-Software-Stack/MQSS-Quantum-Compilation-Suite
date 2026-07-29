
from catalyst import qjit
import pennylane as qml

dev = qml.device("lightning.qubit", wires=2)

@qjit(keep_intermediate=True)
@qml.set_shots(1000)
@qml.qnode(dev)
def circuit_SZToSAdj():
    qml.adjoint(qml.S)(wires=0)
    qml.S(wires=1)
    qml.PauliZ(wires=1)
    return qml.counts()
