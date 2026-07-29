from catalyst import qjit
import pennylane as qml

dev = qml.device("lightning.qubit", wires=2)

@qjit(keep_intermediate=True)
def module_XYZHadamard():

    @qml.set_shots(1000)
    @qml.qnode(dev)
    def circuit_test():
        qml.PauliX(wires=0)
        qml.Hadamard(wires=0)
        qml.Hadamard(wires=1)
        qml.PauliX(wires=1)
        return qml.counts()

    @qml.set_shots(1000)
    @qml.qnode(dev)
    def circuit_test1():
        qml.PauliY(wires=0)
        qml.Hadamard(wires=0)
        qml.Hadamard(wires=1)
        qml.PauliY(wires=1)
        return qml.counts()

    @qml.set_shots(1000)
    @qml.qnode(dev)
    def circuit_test2():
        qml.PauliZ(wires=0)
        qml.Hadamard(wires=0)
        qml.Hadamard(wires=1)
        qml.PauliZ(wires=1)
        return qml.counts()

    return circuit_test(), circuit_test1(), circuit_test2()
