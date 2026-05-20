

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

#include <set>

using namespace llvm;
using namespace mlir;
using namespace std;
enum Gate {
  CNOT,
  PauliX,
  PauliZ,
  CZ,
  PauliY,
  CY,
  H,
  Hadamard,
  RX,
  RY,
  RZ,
  S,
  SAdj,
  T,
  SWAP,
  UNKNOWN
};

struct QubitID {
  Value base;
  int64_t index;
};

using tupleVectorsQubitIDs =
    std::tuple<SmallVector<QubitID, 2>, SmallVector<QubitID, 2>>;

using tupleVectorsValues =
    std::tuple<SmallVector<mlir::Value, 2>, SmallVector<mlir::Value, 2>>;

enum class QubitRole { Control, Target };

struct QubitOperands {
  SmallVector<QubitID, 2> ids;
  SmallVector<mlir::Value, 2> in;
  SmallVector<mlir::Value, 2> out;

  [[nodiscard]] bool empty() const {
    return ids.empty() && in.empty() && out.empty();
  }

  [[nodiscard]] bool hasValueSemantics() const { return !out.empty(); }
};

struct MeasurementInfo {
  int64_t QubitIndex;
  int ClassicalBitIndex;
};

struct QuantumOpView {
  Gate GateTy = Gate::UNKNOWN;

  std::map<QubitRole, QubitOperands> qubits;
  SmallVector<MeasurementInfo> measurements;
  SmallVector<mlir::Value, 2> params;

  bool isAdjoint = false;
  bool hasSideEffects = false;
  bool isMeasureOp = false;

  [[nodiscard]] const QubitOperands &getQubits(QubitRole role) const {
    static const QubitOperands empty;
    auto it = qubits.find(role);
    return it == qubits.end() ? empty : it->second;
  }

  QubitOperands &getQubits(QubitRole role) { return qubits[role]; }

  bool isControlled() { return !getQubits(QubitRole::Control).empty(); }

  [[nodiscard]] bool hasValueSemantics() const {
    return getQubits(QubitRole::Control).hasValueSemantics() ||
           getQubits(QubitRole::Target).hasValueSemantics();
  }

  [[nodiscard]] bool isParameterized() const { return !params.empty(); }

  [[nodiscard]] bool mayNotCommute() const { return hasSideEffects; }
};

struct QuantumKernelInfo {
  size_t AllocatedQubits = 0;
  Value AllocOp;
  size_t NumMeasureQubits = 0;
  MapVector<Operation *, QuantumOpView> OpQViewMap;
};

inline Gate parseGateTy(const StringRef &GateTy) {
  if (GateTy == "CNOT")
    return CNOT;
  if (GateTy == "PauliX")
    return PauliX;
  if (GateTy == "PauliZ")
    return PauliZ;
  if (GateTy == "PauliY")
    return PauliY;
  if (GateTy == "Hadamard" || GateTy == "H")
    return H;
  if (GateTy == "RX")
    return RX;
  if (GateTy == "RY")
    return RY;
  if (GateTy == "RZ")
    return RZ;

  if (GateTy == "CZ")
    return CZ;
  if (GateTy == "S")
    return S;
  if (GateTy == "SAdj")
    return SAdj;
  if (GateTy == "T")
    return T;
  if (GateTy == "SWAP")
    return SWAP;
  return UNKNOWN;
}

inline StringRef parseGateTy(const Gate &GateTy) {
  if (GateTy == Gate::CNOT)
    return "CNOT";
  if (GateTy == Gate::PauliX)
    return "PauliX";
  if (GateTy == Gate::H || GateTy == Gate::Hadamard)
    return "H";
  if (GateTy == Gate::RX)
    return "RX";
  if (GateTy == Gate::RY)
    return "RY";
  if (GateTy == Gate::RZ)
    return "RZ";
  if (GateTy == Gate::PauliZ)
    return "PauliZ";
  if (GateTy == Gate::CZ)
    return "CZ";
  if (GateTy == Gate::PauliY)
    return "PauliY";
  if (GateTy == Gate::S)
    return "S";
  if (GateTy == Gate::T)
    return "T";
  if (GateTy == Gate::SAdj)
    return "SAdj";
  if (GateTy == Gate::SWAP)
    return "SWAP";
  return "UNKNOWN";
}

static std::optional<QubitID> getOriginQubit(mlir::Value v) {
  // llvm::outs() << "Checking: " << v << "\n";
  while (true) {
    auto definingOp = v.getDefiningOp();

    if (!definingOp) {
      // llvm::outs().indent(4) << "def op is NULL\n";
      return std::nullopt; // block argument or unknown source
    }

    // llvm::outs().indent(4) << "def op: " << *definingOp << "\n";
    // Case 1: %q4 = quantum.extract %reg[4]
    if (definingOp->getName().getStringRef() == "quantum.extract") {
      mlir::Value reg = definingOp->getOperand(0);

      // You need to adapt this depending on how Catalyst stores the index.
      // Often it is an IntegerAttr.

      auto indexAttr = definingOp->getAttrOfType<mlir::IntegerAttr>("idx_attr");
      if (!indexAttr) {
        // llvm::outs().indent(4) << "Index Attr is null\n";
        return std::nullopt;
      }
      // llvm::outs().indent(4) << "reg: " << reg << ", Index attr: " <<
      // indexAttr.getInt() << "\n";

      return QubitID{reg, indexAttr.getInt()};
    }

    // Case 2:
    // %out:2 = quantum.custom "CNOT"() %a, %b
    // %out#0 maps to operand 0
    // %out#1 maps to operand 1
    if (definingOp->getName().getStringRef() == "quantum.custom") {
      auto result = llvm::dyn_cast<mlir::OpResult>(v);
      if (!result)
        return std::nullopt;

      unsigned resultNo = result.getResultNumber();

      if (resultNo >= definingOp->getNumOperands())
        return std::nullopt;

      v = definingOp->getOperand(resultNo);
      continue;
    }

    return std::nullopt;
  }
}

class MyModuleAnalysis {

public:
  virtual ~MyModuleAnalysis() = default;

  virtual SmallVector<func::FuncOp, 16> fetchQuantumKernels() = 0;

  virtual void gatherOpInfo() = 0; // Pure virtual functions that need to be
                                   // implemented by derived classes

  virtual mlir::LogicalResult verifyModule() = 0;

  virtual MapVector<mlir::func::FuncOp, QuantumKernelInfo>
  getKernelDialectInfo() = 0;

  virtual void addOperation(Operation *NewOp) = 0;

  virtual bool UpdateOperands(Operation *Op, QubitRole Role, Value OrigValue,
                              Value NewValue) = 0;

  virtual const QuantumOpView getOpInfo(Operation *Op) = 0;

  virtual void clearKernelBody(func::FuncOp &kernel,
                               std::set<Operation *> notToErase) = 0;
};