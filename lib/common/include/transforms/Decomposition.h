

#include "PassIncludes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <cmath>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/Support/Debug.h>
#include <mlir/IR/Operation.h>
#include <mlir/IR/Value.h>
#include <vector>

struct GateTy {
  Gate NewGateTy = Gate::UNKNOWN;
  SmallVector<double> ConstValues;
};

struct DecomposePassInfoTy {
  Gate GateToDecompose = Gate::UNKNOWN;
  std::vector<GateTy> Pattern;
};


/**
PauliX  -> RX(pi)
PauliY  -> RY(pi)
PauliZ  -> RZ(pi)

S       -> RZ(pi/2)
S†      -> RZ(-pi/2)

T       -> RZ(pi/4)
T†      -> RZ(-pi/4)

H       -> RZ(pi) RY(pi/2) RZ(pi)
        or another equivalent native decomposition

CNOT    -> H(target) CZ(control, target) H(target)
 */
// Perform CX to H-CZ-H or CZ to H-CX-H decomposition.
// For Value Semantics, SSA form needs to be fixed. An example is shown here:
// The SSA chain for the following example CNOT Op:
//      %out_qubits:2 = quantum.custom "CNOT"() %1, %2 : !quantum.bit,
//      !quantum.bit %3 = quantum.insert %0[ 0], %out_qubits#0 : !quantum.reg,
//      !quantum.bit %4 = quantum.insert %3[ 1], %out_qubits#1 : !quantum.reg,
//      !quantum.bit
// Resolves as follows:
//      %h1_out = quantum.custom "Hadamard"() %2 : !quantum.bit
//      %cz_out:2 = quantum.custom "CZ"() %1, %h1_out : !quantum.bit,
//      !quantum.bit %h2_out = quantum.custom "Hadamard"() %cz_out#1 :
//      !quantum.bit %3 = quantum.insert %0[ 0], %cz_out#0 : !quantum.reg,
//      !quantum.bit %4 = quantum.insert %3[ 1], %h2_out : !quantum.reg,
//      !quantum.bit
// Explanation:
//    1. %2 (target qubit) flows into the first Hadamard → produces %h1_out
//    2. %1 (control qubit) and %h1_out flow into CZ → produces %cz_out#0
//    3. (control) and %cz_out#1 (target) %cz_out#1 flows into the second
//    4. Hadamard → produces %h2_out %cz_out#0 and %h2_out replace the
//    original
//    5. %out_qubits#0 and %out_qubits#1 in the two quantum.insert ops

static void performDecomposition(MyModuleAnalysis &analysis,
                                 const DecomposePassInfoTy PassInfo) {

  SmallSetVector<Operation *, 8> ToErase;
  
  auto KernelDialectInfo = analysis.getKernelDialectInfo();
  for (auto &[kernel, kernelInfo] : KernelDialectInfo) {
    auto OpQuantumView = kernelInfo.OpQViewMap;
   
    if (kernelInfo.AllocatedQubits == 0)
      continue;

    mlir::IRRewriter ConstOpBuilder(kernel->getContext());
    auto *funcblock = &kernel.getBody().front();
    ConstOpBuilder.setInsertionPointToStart(funcblock);

    for (auto &[GateOp, GateQView] : OpQuantumView) {

      if (GateQView.GateTy != PassInfo.GateToDecompose) {
        continue;
      }

      mlir::IRRewriter builder(GateOp->getContext());
      builder.setInsertionPointAfter(GateOp);

      auto ControlInQubitOps = GateQView.getQubits(QubitRole::Control).in;
      auto TargetInQubitOps = GateQView.getQubits(QubitRole::Target).in;

      auto loc = GateOp->getLoc();
      std::vector<tuple<Operation *, QuantumOpView>> DecomposePattern;

      for (unsigned i=0; i < PassInfo.Pattern.size(); ++i) {
        auto [GateToCreate, ConstVals] = PassInfo.Pattern[i];
        auto NewGateTy = parseGateTy(GateToCreate);
        auto GateQubitRoles = getGateOpRoles(NewGateTy);

        if (GateQubitRoles.empty()){
          report_fatal_error(Twine("Unsupported Gate: ") + Twine(NewGateTy));
        }
        
        SmallVector<Value, 2> params;

        if (!ConstVals.empty()) {
          for (auto val : ConstVals) {
            auto arithVal = createArithConstantOp(
                kernel->getLoc(), ConstOpBuilder, val);
            params.push_back(arithVal);
          }
        }
        Operation *NewGate;
        if (GateQubitRoles[0] == QubitRole::Control) {
          NewGate = createNewGate(loc, NewGateTy, ControlInQubitOps,
                                  TargetInQubitOps, params, builder);
        } else {
          NewGate = createNewGate(loc, NewGateTy, {}, TargetInQubitOps,
                                  params, builder);
        }
        llvm::outs() << "-->Created: " << *NewGate << "\n";
        analysis.addOperation(NewGate);
        auto NewGateQView = analysis.getOpInfo(NewGate);
        auto NewGateTargetOuts = NewGateQView.getQubits(QubitRole::Target).out;
        auto NewGateControlOuts = NewGateQView.getQubits(QubitRole::Control).out;

        // Output Qubits of the current new gate will be the inputs
        // to the next new gate.
        if(!NewGateControlOuts.empty()){
          ControlInQubitOps = NewGateControlOuts;
        }
        if(!NewGateTargetOuts.empty()){
          TargetInQubitOps = NewGateTargetOuts;
        }

        // If the gate created is the final gate in the pattern,
        // replace all uses of the Results of GateOp with the results
        // of the newly created final gate.
        // TODO: The following only fixes the uses of the Target Qubits of
        //        the results of GateOp. Need to fix the uses of the Control Qubits
        if(i == PassInfo.Pattern.size()-1){
          auto GateOpTargetResults = GateQView.getQubits(QubitRole::Target).out;
          auto GateOpControlResults = GateQView.getQubits(QubitRole::Control).out;
          if(!GateOpTargetResults.empty()){
            GateOpTargetResults[0].replaceAllUsesWith(NewGateTargetOuts[0]);
          }
          if(!GateOpControlResults.empty()){
            // Replace the uses of the Control Qubits of the Gate to decompose
            // with the Control Qubit of last newly create Controlled Gate.
            // TODO: Is this sound?
            GateOpControlResults[0].replaceAllUsesWith(ControlInQubitOps[0]);
          }
        }
      }
      ToErase.insert(GateOp);
    }
  }

  for (auto *Op : ToErase) {
    //auto OpQView = analysis.getOpInfo(Op);
    MQSS_DEBUG("-->To Erase: " << *Op << "\n");
    //fixSSAForm(DecomposePattern, OpQView);
    cancel(Op);
  }

  return;
}