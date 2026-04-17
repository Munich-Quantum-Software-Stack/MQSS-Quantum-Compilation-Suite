import pennylane as qml
from catalyst import qjit

dev = qml.device("lightning.qubit", wires=2)

@qjit(keep_intermediate=True)
@qml.qnode(dev)
def circuit(theta: float):
    qml.Hadamard(0)
    qml.RX(theta, 1)
    qml.CNOT(wires=[0, 1])
    return qml.expval(qml.Z(1))

def __catalyst_compile_args__circuit():
    return ((0.5,), {})