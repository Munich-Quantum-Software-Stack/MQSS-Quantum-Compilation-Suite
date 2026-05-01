
# mqss-cc /workspaces/MQSS-Passes-Suite/test-mqss-passes/code/catalyst/CommuteCNOTX.py --function kernel \ 
# --stage HLOLoweringStage --out-dir output/ --passes=CommonCommutePass=mode=CX-X

from catalyst import qjit
import pennylane as qml

dev = qml.device("lightning.qubit", wires=2)

@qjit(keep_intermediate=True) 
@qml.set_shots(1000)
@qml.qnode(dev)
def kernel():
    qml.CNOT(wires=[0, 1])
    qml.PauliX(wires=1)
    qml.CNOT(wires=[1, 0])
    qml.PauliX(wires=1)
    return qml.counts()

def __catalyst_compile_args__circuit():
    return ({})