

// #include "Extractor.h"
#include "QuakeExtractor.h"
#include "CatalystExtractor.h"
#include <memory>
#include <string>

enum class QuantumDialect { Quake, Catalyst, Unknown };

static string DialectTytoString(QuantumDialect DialectTy) {

  switch (DialectTy) {
  case QuantumDialect::Quake:
    return "Quake";
  case QuantumDialect::Catalyst:
    return "Catalyst";
  default:
    return "Unknown";
  }
}
static QuantumDialect detectFramework(ModuleOp module) {
  QuantumDialect fw = QuantumDialect::Unknown;
  module.walk([&](Operation *op) {
    auto ns = op->getDialect()->getNamespace();
    if (ns == "quake") { fw = QuantumDialect::Quake; return WalkResult::interrupt(); }
    if (ns == "quantum") { fw = QuantumDialect::Catalyst; return WalkResult::interrupt(); }
    return WalkResult::advance();
  });
  return fw;
}

class DialectAnalysisSelector {
public:
  DialectAnalysisSelector(Operation *op) {
    auto module = cast<ModuleOp>(op);
    switch (detectFramework(module)) {
    case QuantumDialect::Quake:
      impl = std::make_unique<QuakeAnalysis>(module);
      DialectTy = QuantumDialect::Quake;
      break;
    case QuantumDialect::Catalyst:
      impl = std::make_unique<CatalystQuantumAnalysis>(module);
      DialectTy = QuantumDialect::Catalyst;
      break;
    default:
      impl = nullptr; // pass checks this
      DialectTy = QuantumDialect::Unknown;
    }
  }

  MyModuleAnalysis *get() { return impl.get(); }
  QuantumDialect getDialect() { return DialectTy; };

private:
  std::unique_ptr<MyModuleAnalysis> impl;
  QuantumDialect DialectTy;
};