
#include "EquivalenceCheckingManager.hpp"
#include "Passes/Analysis/Extractor.h"
#include "Utils/MQTCoreUtils.h"
#include "checker/dd/DDAlternatingChecker.hpp"

#include <cstddef>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <mlir/Pass/PassInstrumentation.h>
#include <string>
#include <unordered_map>
#include <vector>

struct VerifyQuantumComputationTy {

  qc::QuantumComputation qc1;
  qc::QuantumComputation qc2;
};

namespace mqss::mqssci::verify {
class VerifyPassInstrumentation : public mlir::PassInstrumentation {
public:
  explicit VerifyPassInstrumentation(
      llvm::DenseMap<StringRef, VerifyQuantumComputationTy> snap_shot)
      : cached_module_snapshot(std::move(snap_shot)) {}
  llvm::DenseMap<StringRef, VerifyQuantumComputationTy> get_module_snap_shot();

private:
  qc::QuantumComputation
  createMQTQuantumComputation(std::size_t allocatedQubits,
                              std::size_t numMeasureQubits,
                              MapVector<Operation *, QuantumOpView> OpQView);
  void runBeforePass(mlir::Pass *pass, mlir::Operation *op) override;
  void runAfterPass(mlir::Pass *pass, mlir::Operation *op) override;
  void runAfterPassFailed(Pass *pass, Operation *op) override;
  llvm::DenseMap<llvm::StringRef, VerifyQuantumComputationTy>
      cached_module_snapshot;
};
} // namespace mqss::mqssci::verify
