
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

// Default checker configuration used when a caller doesn't supply one:
// partial-equivalence checking on, so a mapping pass introducing ancillas
// doesn't spuriously fail verification. Callers that need different
// behavior (e.g. tests wanting to pin a single deterministic checker) pass
// their own ec::Configuration instead -- runAfterPass must not mutate
// whatever it's given, so this is the only place the default is set.
inline ec::Configuration defaultVerificationConfig() {
  ec::Configuration config;
  config.functionality.checkPartialEquivalence = true;
  return config;
}

class VerifyPassInstrumentation : public mlir::PassInstrumentation {
public:
  explicit VerifyPassInstrumentation(
      llvm::DenseMap<StringRef, VerifyQuantumComputationTy> snap_shot,
      ec::Configuration config = defaultVerificationConfig())
      : cached_module_snapshot(std::move(snap_shot)),
        checker_config(std::move(config)) {}
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
  ec::Configuration checker_config{};
};
} // namespace mqss::mqssci::verify
