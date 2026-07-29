

from catalyst import qjit
import pennylane as qml

dev = qml.device("lightning.qubit", wires=2)

@qjit(keep_intermediate=True)
@qml.set_shots(1000)
@qml.qnode(dev)
def circuit_CzToHCxH():
    qml.CZ(wires=[0, 1])
    return qml.counts()
