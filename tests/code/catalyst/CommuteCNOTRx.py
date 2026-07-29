

# mqss-cc /workspaces/MQSS-Passes-Suite/test-mqss-passes/code/catalyst/CommuteCNOTRx.py --function circuit \
#     --stage HLOLoweringStage --out-dir output/ --passes=CommonCommutePass=mode=CX-RX

from catalyst import qjit
import pennylane as qml

dev = qml.device("lightning.qubit", wires=3)

@qjit(keep_intermediate=True)
@qml.set_shots(1000)
@qml.qnode(dev)
def circuit_CommuteCNOTRx():
    qml.CNOT(wires=[0, 1])
    qml.PauliX(wires=2)
    qml.RX(2.4, wires=1)

    qml.CNOT(wires=[1, 0])
    qml.RX(3.1416, wires=1)

    qml.CNOT(wires=[0, 1])
    qml.PauliX(wires=1)
    qml.RX(5.1416, wires=1)

    return qml.counts()

def __catalyst_compile_args__circuit():
    return ({})
