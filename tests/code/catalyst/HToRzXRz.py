

from catalyst import qjit
import pennylane as qml

dev = qml.device("lightning.qubit", wires=2)

@qjit(keep_intermediate=True)
@qml.set_shots(1000)
@qml.qnode(dev)
def circuit_HToRzXRz():
    qml.CNOT(wires=[0, 1])
    qml.Hadamard(wires=0)
    m0 = qml.measure(0)
    m1 = qml.measure(1)
    return m0, m1
