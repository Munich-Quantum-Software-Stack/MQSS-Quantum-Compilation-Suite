

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

enum Gate { CNOT, PauliX, H, Hadamard, RX, PauliZ, Y, UNKNOWN };

struct QubitID {
  Value base;
  std::size_t index;
};

struct QuantumOpView {
  Gate GateTy = Gate::UNKNOWN; //<---------- Important to initialize enums
  std::vector<QubitID> ControlQubits = {};
  std::vector<QubitID> TargetQubits = {};
  std::vector<Value> Params = {};
  bool hasSideEffects = false;
};


using QuantumOpInfo = std::map<Operation *, QuantumOpView>;



inline Gate parseGateTy(const StringRef &GateTy) {
  if (GateTy == "CNOT")
    return CNOT;
  if (GateTy == "PauliX")
    return PauliX;
  if (GateTy == "H")
    return H;
  if (GateTy == "RX")
    return RX;
  if (GateTy == "PauliZ")
    return PauliZ;
  if (GateTy == "Y")
    return Y;
  return UNKNOWN;
}

inline StringRef parseGateTy(const Gate &GateTy) {
  if (GateTy == Gate::CNOT)
    return "CNOT";
  if (GateTy == Gate::PauliX)
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

using tupleVectorsQubitIDs =
    std::tuple<std::vector<QubitID>, std::vector<QubitID>>;


    

class MyModuleAnalysis {

public:
  virtual ~MyModuleAnalysis() = default;

  virtual SmallVector<func::FuncOp, 16> fetchQuantumKernels() = 0;

  virtual void gatherOpInfo() = 0; // Pure virtual functions that need to be implemented by
                                  // derived classes

  virtual mlir::LogicalResult verifyModule() = 0;

  virtual const llvm::DenseMap<func::FuncOp, QuantumOpInfo> getKernelDialectInfo() = 0;

};