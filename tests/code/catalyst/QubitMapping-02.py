

from catalyst import qjit
import pennylane as qml

dev = qml.device("lightning.qubit", wires=5)

@qjit(keep_intermediate=True)
@qml.set_shots(1000)
@qml.qnode(dev)
def circuit_QubitMapping():
    qml.CNOT(wires=[4, 2])
    qml.CNOT(wires=[3, 1])
    qml.CNOT(wires=[4, 1])
    qml.RX(1.5, wires=1)
    qml.RY(3.1416, wires=2)
    qml.RZ(2.25, wires=3)
    qml.T(wires=4)
    m0 = qml.measure(0)
    m1 = qml.measure(1)
    m2 = qml.measure(2)
    m3 = qml.measure(3)
    m4 = qml.measure(4)
    return m0, m1, m2, m3, m4
