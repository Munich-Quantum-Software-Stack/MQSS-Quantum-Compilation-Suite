

from catalyst import qjit
import pennylane as qml

dev = qml.device("lightning.qubit", wires=2)

@qjit(keep_intermediate=True)
@qml.set_shots(1000)
@qml.qnode(dev)
def circuit():
    qml.PauliX(wires=1)
    qml.CNOT(wires=[0, 1])
    qml.CNOT(wires=[0, 1])
    qml.CNOT(wires=[0, 1])
    qml.CNOT(wires=[0, 1])
    qml.CNOT(wires=[0, 1])
    qml.PauliX(wires=1)
    qml.CNOT(wires=[1, 0])
    return qml.counts()

def __catalyst_compile_args__circuit():
    return ({})