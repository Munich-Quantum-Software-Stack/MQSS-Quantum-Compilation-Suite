

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace mlir;

enum Gate { CNOT, PAULIX, H, Hadamard, RX, PauliZ, Y, UNKNOWN };

inline Gate parseGateTy(const StringRef &GateTy) {
  if (GateTy == "CNOT")
    return CNOT;
  if (GateTy == "PauliX")
    return PAULIX;
  if (GateTy == "H")
    return H;
  if (GateTy == "RX")
    return RX;
  if (GateTy == "Z")
    return PauliZ;
  if (GateTy == "Y")
    return Y;
  return UNKNOWN;
}

inline StringRef parseGateTy(const Gate &GateTy) {
  if (GateTy == Gate::CNOT)
    return "CNOT";
  if (GateTy == Gate::PAULIX)
    return "PauliX";
  if (GateTy == Gate::H)
    return "H";
  if (GateTy == Gate::RX)
    return "RX";
  if (GateTy == Gate::PauliZ)
    return "PauliZ";
  if (GateTy == Gate::Y)
    return "Y";
  return "UNKNOWN";
}

struct QubitID {
  Value base;
  std::size_t index;
};

struct QuantumOpView {
  Gate GateTy = Gate::UNKNOWN; //<---------- Important to initialize enums
  std::vector<QubitID> ControlQubits = {};
  std::vector<QubitID> TargetQubits = {};
  bool hasSideEffects = false;
};

struct DialectInfo {

  SmallVector<func::FuncOp, 16> QuantumKernels;
  std::vector<Operation *> GateOps;
  std::unordered_map<Operation *, QuantumOpView> OpQuantumView;
};

using tupleVectorsQubitIDs =
    std::tuple<std::vector<QubitID>, std::vector<QubitID>>;

class MyModuleAnalysis {

public:
  virtual ~MyModuleAnalysis() = default;
  virtual std::vector<Operation *>
  getGateOps() = 0; // Pure virtual functions that need to be implemented by
                    // derived classes

  virtual void fetchQuantumKernels() = 0;

  virtual void gatherOpInfo() = 0;

  virtual mlir::LogicalResult verifyModule() = 0;

  DialectInfo getDialectInfo() { return Info; }

protected:
  DialectInfo Info;
};