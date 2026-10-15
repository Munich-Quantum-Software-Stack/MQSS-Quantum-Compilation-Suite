
/* This code and any associated documentation is provided "as is"

Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
*/

#include "Passes/Transforms/TransformPasses.h"
#include "Utils/MQTCoreUtils.h"
#include "Utils/TranspilationPassUtils.h"
#include "sc/configuration/Configuration.hpp"

#include <llvm/Support/raw_ostream.h>
#include <mlir/IR/Diagnostics.h>
#include <mlir/IR/Location.h>
#include <utility>

namespace mqss::mqssci::opt {

#define GEN_PASS_DEF_COMMONMAPPINGPASS
#include "Passes/Transforms/TransformPasses.h.inc"

} // namespace mqss::mqssci::opt

using namespace llvm;
using namespace qc;
namespace {

// TODO: Use Exact Mapper if Number of Qubits >=8, Heursitic Mapper otherwise.
struct PipelineConfig {
  Architecture arch;
  Configuration settings;

  static Configuration getDefaultSettings() {
    Configuration DefaultSettings;
    DefaultSettings.heuristic = Heuristic::GateCountMaxDistance;
    DefaultSettings.layering = Layering::DisjointQubits;
    DefaultSettings.initialLayout = InitialLayout::Identity;
    DefaultSettings.preMappingOptimizations = false;
    DefaultSettings.postMappingOptimizations = false;
    DefaultSettings.lookaheadHeuristic = LookaheadHeuristic::None;
    DefaultSettings.debug = false;
    DefaultSettings.addMeasurementsToMappedCircuit = true;
    return DefaultSettings;
  }

  static llvm::Expected<PipelineConfig> defaults() {
    PipelineConfig config;
    // Defining test architecture
    /*
        3
       / \
      4   2
      |   |
      0---1
    */
    const CouplingMap cm = getCouplingMap();
    config.arch.loadCouplingMap(5, cm);
    // Defining the settings of the mqt-mapper
    config.settings = getDefaultSettings();
    return config;
  };

  static llvm::Expected<PipelineConfig> fromCustomCouplingMap(
      const std::vector<std::pair<std::uint32_t, std::uint32_t>>
          &coupling_map) {
    PipelineConfig config;
    // Defining test architecture
    /*
        3
       / \
      4   2
      |   |
      0---1
    */
    const CouplingMap cm = getCouplingMap(coupling_map);
    if (cm.empty()) {
      return llvm::createStringError(
          "Mapping: Could not load coupling map from JSON!");
    }
    std::set<std::uint32_t> qubits;
    for (const auto &[a, b] : cm) {
      qubits.insert(a);
      qubits.insert(b);
    }
    std::size_t numQubits = qubits.size();
    config.arch.loadCouplingMap(numQubits, cm);
    // Defining the settings of the mqt-mapper
    config.settings = getDefaultSettings();
    return config;
  };

  static llvm::Expected<PipelineConfig> fromJSON(const std::string &path) {
    PipelineConfig config;
    std::ifstream file(path);
    if (!file.is_open()) {
      return llvm::createStringError("Mapping: Could not open config file: " +
                                     path);
    }

    try {

      nlohmann::json j;
      file >> j;

      // --- Mapper Settings ---
      const auto &ms = j.at("mapper_settings");
      if (ms.empty()) {
        return llvm::createStringError(
            "Could not load Mapper Settings from JSON");
      }
      auto heuristicOrErr = heuristicFromString(ms.at("heuristic"));
      if (!heuristicOrErr) {
        return heuristicOrErr.takeError();
      }

      auto layeringOrErr = layeringFromString(ms.at("layering"));
      if (!layeringOrErr) {
        return layeringOrErr.takeError();
      }
      auto layoutOrErr = initialLayoutFromString(ms.at("initial_layout"));
      if (!layoutOrErr) {
        return layoutOrErr.takeError();
      }
      auto lookaheadOrErr = lookaheadFromString(ms.at("lookahead_heuristic"));
      if (!lookaheadOrErr) {
        return lookaheadOrErr.takeError();
      }
      config.settings.preMappingOptimizations =
          ms.at("pre_mapping_optimizations").get<bool>();
      config.settings.postMappingOptimizations =
          ms.at("post_mapping_optimizations").get<bool>();

      config.settings.debug = ms.at("debug").get<bool>();
      config.settings.addMeasurementsToMappedCircuit =
          ms.at("add_measurements_to_mapped_circuit").get<bool>();

      config.settings.heuristic = *heuristicOrErr;
      config.settings.lookaheadHeuristic = *lookaheadOrErr;
      config.settings.layering = *layeringOrErr;
      config.settings.initialLayout = *layoutOrErr;
      // --- Architecture ---
      const auto &arch = j.at("architecture");
      auto cm = getCouplingMap(j);
      if (cm.empty()) {
        return llvm::createStringError(
            "Mapping: Could not load coupling map from JSON!");
      }
      config.arch.loadCouplingMap(arch.at("num_qubits").get<int>(), cm);

      return config;
    } catch (const nlohmann::json::exception &e) {
      return llvm::createStringError("Mapping: Malformed JSON config: " +
                                     std::string(e.what()));
    }
  }

  static llvm::Expected<PipelineConfig> fromQDMI(string device_conf) {

    PipelineConfig config;

    MQSS_DEBUG("-->Device conf is: " << device_conf << "\n");
    auto dev = createQDMIDevice(device_conf.c_str());
    if (!dev) {
      return llvm::createStringError("Mapping: Could not create QDMI device!");
    }

    auto device_properties = getDeviceProperties(*dev);
    if (!device_properties) {
      return llvm::createStringError(
          "Mapping: Could not fetch QDMI device properties!");
    }

    auto numQubits = device_properties->numQubits;
    auto device_coupling_map = device_properties->cm;

    if (device_coupling_map.empty()) {
      return llvm::createStringError(
          "Mapping: Could not fetch the coupling map of QDMI device!");
    }

    config.arch.loadCouplingMap(numQubits, device_coupling_map);
    MQSS_DEBUG("Mapping: Device Coupling Map loaded!!");
    // Defining the settings of the mqt-mapper
    config.settings = getDefaultSettings();
    return config;
  }

private:
  static CouplingMap getCouplingMap() {

    return {{0, 1}, {1, 0}, {1, 2}, {2, 1}, {2, 3},
            {3, 2}, {3, 4}, {4, 3}, {4, 0}, {0, 4}};
  }

  static CouplingMap getCouplingMap(nlohmann::json json_input) {
    const auto &arch = json_input.at("architecture");
    CouplingMap cm;
    for (const auto &pair : arch.at("coupling_map"))
      cm.insert({pair[0].get<int>(), pair[1].get<int>()});
    return cm;
  }

  static CouplingMap
  getCouplingMap(const std::vector<std::pair<std::uint32_t, std::uint32_t>>
                     &qubit_connectivity) {
    CouplingMap cm;
    for (const auto &[src_id, dest_id] : qubit_connectivity) {
      cm.insert({src_id, dest_id});
    }

    return cm;
  }

  // -- String -> Enum helpers --
  static llvm::Expected<Heuristic> heuristicFromString(const std::string &s) {
    if (s == "GateCountMaxDistance")
      return Heuristic::GateCountMaxDistance;
    if (s == "GateCountSumDistance")
      return Heuristic::GateCountSumDistance;
    // ... other variants
    return llvm::createStringError("Mapping: Unknown heuristic: " + s);
  }

  static llvm::Expected<Layering> layeringFromString(const std::string &s) {
    if (s == "DisjointQubits")
      return Layering::DisjointQubits;
    if (s == "OddGates")
      return Layering::OddGates;
    // ... other variants

    return llvm::createStringError("Mapping: Unknown heuristic: " + s);
  }

  static llvm::Expected<InitialLayout>
  initialLayoutFromString(const std::string &s) {
    if (s == "Identity")
      return InitialLayout::Identity;
    if (s == "Static")
      return InitialLayout::Static;
    if (s == "Dynamic")
      return InitialLayout::Dynamic;
    // ... other variants

    return llvm::createStringError("Mapping: Unknown heuristic: " + s);
  }

  static llvm::Expected<LookaheadHeuristic>
  lookaheadFromString(const std::string &s) {
    if (s == "None")
      return LookaheadHeuristic::None;
    if (s == "GateCountMaxDistance")
      return LookaheadHeuristic::GateCountMaxDistance;
    // ... other variants

    return llvm::createStringError("Mapping: Unknown heuristic: " + s);
  }
};

struct MeasureMeantOpInfoTy {
  const qc::Operation *MeasOp;
  SmallVector<mlir::Value> Results;
};

// Performs a controlled Dead-Code elimination optimization.
// It is less aggressive than MLIR's canonicalize and cse optimizations.
// canonicalize and cse can remove newly introduced gates.
bool controlledDCE(SmallPtrSet<mlir::Operation *, 16> OpsToErase,
                   mlir::Value OldAllocaOp, mlir::Value newAllocOp) {
  llvm::SmallPtrSet<mlir::Operation *, 16> operandsToCleanup;

  for (mlir::Operation *op : OpsToErase) {
    for (mlir::Value operand : op->getOperands()) {
      if (operand == OldAllocaOp)
        continue; // handled separately below; don't double-erase
      if (!OpsToErase.contains(operand.getDefiningOp()))
        operandsToCleanup.insert(operand.getDefiningOp());
    }
  }

  if (!eraseOpsSafely(OldAllocaOp.getLoc(), OpsToErase)) {
    mlir::emitError(OldAllocaOp.getLoc(),
                    "Controlled DCE: Ops not erased safely");
    return false;
  }
  if (!eraseOpsSafely(OldAllocaOp.getLoc(), operandsToCleanup)) {
    mlir::emitError(OldAllocaOp.getLoc(),
                    "Controlled DCE: Ops not erased safely");
    return false;
  }

  OldAllocaOp.replaceAllUsesWith(newAllocOp);
  if (!OldAllocaOp.use_empty()) {
    mlir::emitError(
        OldAllocaOp.getLoc(),
        "Mapping: Old alloca cannot have uses before being erased!");
    return false;
  }
  OldAllocaOp.getDefiningOp()->erase();
  return true;
}

bool createMappedCircuit(mlir::IRRewriter &builder, Location &loc,
                         QuantumDialect &DialectTy, mlir::Value newAllocOp,
                         QuantumComputation &qcMapped,
                         MapVector<mlir::Operation *, int> &MeasureOps) {

  std::vector<MeasureMeantOpInfoTy> NewMeasureOps;

  for (const auto &op : qcMapped) {
    if (op->getType() == qc::Barrier)
      continue;
    auto &targets = op->getTargets();
    auto &controls = op->getControls();
    auto parameter = op->getParameter();

    // defining the list of controls, targets and parameters
    SmallVector<mlir::Value> params = {};
    SmallVector<mlir::Value> controlValues = {};
    SmallVector<mlir::Value> targetValues = {};

    for (auto target : targets) {
      auto targetRefOp =
          createExtractOp(loc, DialectTy, builder, newAllocOp, target);
      if (!targetRefOp) {
        mlir::emitError(loc, "Mapping: Could not create targetRefOp");
        return false;
      }
      targetValues.push_back(targetRefOp);
    }
    for (auto ctrl : controls) {
      auto controlRef =
          createExtractOp(loc, DialectTy, builder, newAllocOp, ctrl.qubit);
      if (!controlRef) {
        mlir::emitError(loc, "Mapping: Could not create ExtractRefOp");
        return false;
      }
      controlValues.push_back(controlRef);
    }
    for (auto p : parameter) {
      // TODO: Apparently all the parameters are floats in QC, may be the case
      //       this is not always true
      auto constantOp = createArithConstantOp(loc, DialectTy, builder, p);
      if (!constantOp) {
        mlir::emitError(loc, "Mapping: Could not create ConstantOp");
        return false;
      }
      params.push_back(constantOp);
    }

    mlir::Operation *newop;
    Gate Gatety = Gate::UNKNOWN;
    switch (op->getType()) {
    case qc::X:
      if (controlValues.empty())
        Gatety = Gate::PauliX;
      else
        Gatety = Gate::CNOT;
      break;
    case qc::Y:
      if (controlValues.empty())
        Gatety = Gate::PauliY;
      else
        Gatety = Gate::CY;
      break;
    case qc::Z:
      if (controlValues.empty())
        Gatety = Gate::PauliZ;
      else
        Gatety = Gate::CZ;
      break;
    case qc::H:
      Gatety = Gate::H;
      break;
    case qc::S:
      Gatety = Gate::S;
      break;
    case qc::T:
      Gatety = Gate::T;
      break;
    case qc::SWAP:
      Gatety = Gate::SWAP;
      break;
    case qc::RX:
      Gatety = Gate::RX;
      break;
    case qc::RY:
      Gatety = Gate::RY;
      break;
    case qc::RZ:
      Gatety = Gate::RZ;
      break;
    case qc::Sdg:
      Gatety = Gate::SAdj;
      break;
    case qc::Tdg:
      Gatety = Gate::TAdj;
      break;
    }

    if (Gatety != Gate::UNKNOWN) {

      newop = createNewGate(loc, DialectTy, parseGateTy(Gatety), controlValues,
                            targetValues, params, builder);
      if (!newop) {
        mlir::emitError(loc, "Mapping: Could not create new GateOp");
        return false;
      }
    } else if (op->getType() == qc::Measure) {
      auto newResults = createMeasureOp(loc, DialectTy, builder, targetValues);
      if (newResults.empty()) {
        mlir::emitError(loc, "Mapping: Could not create MeasureOp");
        return false;
      }
      NewMeasureOps.push_back({op.get(), newResults});
    } else {
      mlir::emitError(loc,
                      "Mapping: Unsupported gate type produced by the mapper");
      return false;
    }
  }

  // TODO: This always resolves against NewMeasureOps[0] -- the *first*
  // newly-created measurement overall -- regardless of which oldMeasOp is
  // being processed. For a kernel with more than one measurement statement
  // (e.g. several separate quake.mz/quantum.measure ops), every old measure
  // op ends up silently wired to that same first new measurement's results
  // instead of its own corresponding one. Needs to look up the NewMeasureOps
  // entry that actually corresponds to oldMeasOp (matching on identity or
  // position) rather than hardcoding index 0.
  for (auto &[oldMeasOp, numMeasurements] : MeasureOps) {
    if (numMeasurements == 1) {
      auto newResults = NewMeasureOps[0].Results;
      if (!resolveSSAformForMeasureOps(oldMeasOp, newResults)) {
        mlir::emitError(loc,
                        "Mapping: Could not resolve SSA form for MeasureOp");
        return false;
      }
    }
  }
  // builder.create<func::ReturnOp>(loc);
#ifdef MQSS_ENABLE_DEBUG
  MQSS_DEBUG("Dumping QC after mapping:\n");
  qcMapped.print(std::cout);
#endif
  return true;
}

bool performMapping(MyModuleAnalysis &analysis, Architecture architecture,
                    Configuration &settings, QuantumDialect &DialectTy) {

  for (auto &[kernel, info] : analysis.getKernelDialectInfo()) {

    MQSS_DEBUG("\nkernel: " << kernel.getSymName() << "\n"
                            << " total input qubits: " << info.AllocatedQubits
                            << " Measure qubits: " << info.NumMeasureQubits
                            << "\n\n");

    if (info.AllocatedQubits == 0)
      continue;

    if (architecture.getNqubits() < info.AllocatedQubits) {
      MQSS_DEBUG("Physical device Qubits: " << architecture.getNqubits()
                                            << " are less than the no. of "
                                               "logical circuit qubits: "
                                            << info.AllocatedQubits
                                            << "\n Mapping aborted for the "
                                               "kernel!\n");
      continue;
    }

    qc::QuantumComputation qc{info.AllocatedQubits, info.NumMeasureQubits};
    MapVector<mlir::Operation *, int> MeasureOps;
    SmallPtrSet<mlir::Operation *, 16> OpsToErase;
    SmallVector<SmallVector<mlir::Value, 2>> AllResults;

    for (auto &[Op, qview] : info.OpQViewMap) {
      if (qview.GateTy != Gate::UNKNOWN) {
        if (!loadGateOpsIntoQC(Op, qview, qc, qview.isControlled())) {
          mlir::emitError(Op->getLoc(), "Mapping: Error when loading GateOp "
                                        "into MQT's QuantumComputation");
          return false;
        }
        OpsToErase.insert(Op);
      }

      if (qview.isMeasureOp) {
        if (!loadMeasureOpIntoQC(qview, qc, Op->getLoc())) {
          mlir::emitError(Op->getLoc(), "Mapping: Error when loading MeasureOp "
                                        "into MQT's QuantumComputation");
          return false;
        }
        MeasureOps[Op] = qview.measurements.size();
        if (qview.measurements.size() != 1)
          OpsToErase.insert(
              Op); // only the shape resolveSSAformForMeasureOps can't handle
      }
    }
#ifdef MQSS_ENABLE_DEBUG
    MQSS_DEBUG("--> Before mapping, Dumping QC:\n");
    qc.print(std::cout);
#endif

    // Map the circuit
    unique_ptr<HeuristicMapper> mapper;
    try {
      mapper = std::make_unique<HeuristicMapper>(qc, architecture);
    } catch (const std::exception &e) {
      mlir::emitError(kernel->getLoc(),
                      "Mapping: Could not create Heuristic Mapper Obj")
          << e.what();
      return false;
    }
    try {
      mapper->map(settings);
    } catch (const std::exception &e) {
      mlir::emitError(kernel->getLoc(),
                      "Mapping: Could not map the circuit to the architecture!")
          << e.what();
      return false;
    }

    auto qcMapped = qc::QuantumComputation();
    qcMapped = mapper->moveMappedCircuit();

    if (qcMapped.empty()) {
      mlir::emitError(kernel->getLoc(), "Mapping: Could not move circuit to "
                                        "MQT-Core's qc::QuantumComputation!");
      return false;
    }

    mlir::IRRewriter builder(kernel->getContext());
    mlir::Location loc = kernel.getLoc();

    mlir::Block &entry = kernel.getBody().front();

    // entry.clear();                         // erase old ops
    builder.setInsertionPointToStart(&entry); // CRITICAL

    // Location loc = kernel.getLoc();
    auto newAllocOp =
        createAllocOp(loc, DialectTy, builder, info.AllocatedQubits);
    if (!newAllocOp) {
      mlir::emitError(loc, "Mapping: Could not create AllocOp");
      return false;
    }

    // cleaning the mlir::funcOp corresponding to the quake circuit
    // analysis.clearKernelBody(kernel, ToErase);

    if (!createMappedCircuit(builder, loc, DialectTy, newAllocOp, qcMapped,
                             MeasureOps)) {
      mlir::emitError(loc, "Could not create Mapping for Circuit!");
      return false;
    }

    if (!controlledDCE(OpsToErase, info.AllocOp, newAllocOp)) {
      mlir::emitError(loc, "Could not perform Controlled DCE!");
      return false;
    }
  }
  return true;
}

struct CommonMapping
    : public mqss::mqssci::opt::impl::CommonMappingPassBase<CommonMapping> {

public:
  // Default constructor (required for pass registry)
  CommonMapping() = default;

  explicit CommonMapping(
      std::vector<std::pair<std::uint32_t, std::uint32_t>> coupling_map)
      : coupling_map(std::move(coupling_map)) {}

  // Forward the options constructor to the base
  explicit CommonMapping(const CommonMappingPassOptions &options) {
    input = options.input;
    qdmi = options.qdmi;
  }

  void runOnOperation() override {

    // Note: Dialect specific analysis is needed to proceed
    //       This is needed currently because we do not "parse" the dialects.
    //        Parsing would involve a more sophisticated Internal IR to
    //        represent operations of all supported dialects.

    MQSS_DEBUG("\n[Applying Pass: MappingPass]\n");

    auto fail = [&](llvm::Error err) {
      mlir::emitError(getOperation().getLoc())
          << llvm::toString(std::move(err));
      signalPassFailure();
    };

    PipelineConfig config;

    if (!input.empty()) {
      MQSS_DEBUG("-->Reading Coupling Map from JSON input...\n");
      auto configOrErr = PipelineConfig::fromJSON(input);
      if (!configOrErr) {
        fail(configOrErr.takeError());
        return;
      }
      config = *configOrErr;
    } else if (!qdmi.empty()) {
      MQSS_DEBUG("-->Selected QDMI device Mapping...\n");
      auto configOrErr = PipelineConfig::fromQDMI(qdmi);
      if (!configOrErr) {
        fail(configOrErr.takeError());
        return;
      }
      config = *configOrErr;
    } else if (!coupling_map.empty()) {
      MQSS_DEBUG("-->Reading Custom Coupling Map...\n");
      auto configOrErr = PipelineConfig::fromCustomCouplingMap(coupling_map);
      if (!configOrErr) {
        fail(configOrErr.takeError());
        return;
      }
      config = *configOrErr;
    } else {
      MQSS_DEBUG("--> No JSON or QDMI input, using pass defaults!" << "\n");
      auto configOrErr = PipelineConfig::defaults();
      if (!configOrErr) {
        fail(configOrErr.takeError());
        return;
      }
      config = *configOrErr;
    }

    auto architecture = config.arch;
    auto settings = config.settings;

    auto &selector = getAnalysis<DialectAnalysisSelector>();
    auto &analysis = *selector.get();
    auto DialectTy = selector.getDialect();

    if (!performMapping(analysis, architecture, settings, DialectTy)) {
      signalPassFailure();
      return;
    }
    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
      signalPassFailure();
    }
  }

private:
  std::vector<std::pair<std::uint32_t, std::uint32_t>> coupling_map;
};

} // namespace

std::unique_ptr<mlir::Pass> mqss::mqssci::opt::CommonMappingPass() {
  return std::make_unique<CommonMapping>();
}

std::unique_ptr<mlir::Pass>
mqss::mqssci::opt::CommonMappingPass(const CommonMappingPassOptions &options) {
  return std::make_unique<CommonMapping>(options);
}

std::unique_ptr<mlir::Pass> mqss::mqssci::opt::CommonMappingPass(
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> &coupling_map) {
  return std::make_unique<CommonMapping>(coupling_map);
}
