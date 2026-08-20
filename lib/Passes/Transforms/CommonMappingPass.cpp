
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

#include "Passes/Transforms/MappingPassUtils.h"
#include "Utils/DebugUtils.h"
#include "sc/configuration/Configuration.hpp"

#include <llvm/Support/raw_ostream.h>
#include <utility>

namespace mqss::mqssci::opt {

#define GEN_PASS_DEF_COMMONMAPPINGPASS
#include "Passes/Transforms/Transforms.h.inc"

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

  static PipelineConfig defaults() {
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

  static PipelineConfig fromCustomCouplingMap(
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
    config.arch.loadCouplingMap(5, cm);
    // Defining the settings of the mqt-mapper
    config.settings = getDefaultSettings();
    return config;
  };

  static PipelineConfig fromJSON(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open())
      throw std::runtime_error("Could not open config file: " + path);

    nlohmann::json j;
    file >> j;

    PipelineConfig config;

    // --- Architecture ---
    const auto &arch = j.at("architecture");
    auto cm = getCouplingMap(j);
    config.arch.loadCouplingMap(arch.at("num_qubits").get<int>(), cm);

    // --- Mapper Settings ---
    const auto &ms = j.at("mapper_settings");
    config.settings.heuristic = heuristicFromString(ms.at("heuristic"));
    config.settings.layering = layeringFromString(ms.at("layering"));
    config.settings.initialLayout =
        initialLayoutFromString(ms.at("initial_layout"));
    config.settings.preMappingOptimizations =
        ms.at("pre_mapping_optimizations").get<bool>();
    config.settings.postMappingOptimizations =
        ms.at("post_mapping_optimizations").get<bool>();
    config.settings.lookaheadHeuristic =
        lookaheadFromString(ms.at("lookahead_heuristic"));
    config.settings.debug = ms.at("debug").get<bool>();
    config.settings.addMeasurementsToMappedCircuit =
        ms.at("add_measurements_to_mapped_circuit").get<bool>();

    return config;
  }

  static PipelineConfig fromQDMI(string device_conf) {

    PipelineConfig config;

    MQSS_DEBUG("-->Device conf is: " << device_conf << "\n");
    auto dev = createQDMIDevice(device_conf.c_str());

    auto device_properties = getDeviceProperties(dev);

    auto numQubits = device_properties.numQubits;
    auto device_coupling_map = device_properties.cm;
    assert(!device_coupling_map.empty() &&
           "Could not fetch the coupling map of QDMI device!");

    config.arch.loadCouplingMap(numQubits, device_coupling_map);
    MQSS_DEBUG("Device Coupling Map loaded!!");
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
  static Heuristic heuristicFromString(const std::string &s) {
    if (s == "GateCountMaxDistance")
      return Heuristic::GateCountMaxDistance;
    if (s == "GateCountSumDistance")
      return Heuristic::GateCountSumDistance;
    // ... other variants
    throw std::invalid_argument("Unknown heuristic: " + s);
  }

  static Layering layeringFromString(const std::string &s) {
    if (s == "DisjointQubits")
      return Layering::DisjointQubits;
    if (s == "OddGates")
      return Layering::OddGates;
    // ... other variants
    throw std::invalid_argument("Unknown layering: " + s);
  }

  static InitialLayout initialLayoutFromString(const std::string &s) {
    if (s == "Identity")
      return InitialLayout::Identity;
    if (s == "Static")
      return InitialLayout::Static;
    if (s == "Dynamic")
      return InitialLayout::Dynamic;
    // ... other variants
    throw std::invalid_argument("Unknown initial layout: " + s);
  }

  static LookaheadHeuristic lookaheadFromString(const std::string &s) {
    if (s == "None")
      return LookaheadHeuristic::None;
    if (s == "GateCountMaxDistance")
      return LookaheadHeuristic::GateCountMaxDistance;
    // ... other variants
    throw std::invalid_argument("Unknown lookahead heuristic: " + s);
  }
};

struct MeasureMeantOpInfoTy {
  const qc::Operation *MeasOp;
  SmallVector<mlir::Value, 2> Results;
};

void createMappedCircuit(mlir::IRRewriter &builder, Location loc,
                         QuantumDialect DialectTy, mlir::Value newAllocOp,
                         QuantumComputation &qcMapped,
                         MapVector<mlir::Operation *, int> MeasureOps) {

  std::vector<MeasureMeantOpInfoTy> NewMeasureOps;

  for (const auto &op : qcMapped) {
    if (op->getType() == qc::Barrier)
      continue;
    auto &targets = op->getTargets();
    auto &controls = op->getControls();
    auto parameter = op->getParameter();

    // defining the list of controls, targets and parameters
    SmallVector<mlir::Value, 2> params = {};
    SmallVector<mlir::Value, 2> controlValues = {};
    SmallVector<mlir::Value, 2> targetValues = {};

    for (auto target : targets) {
      auto targetRefOp =
          createExtractOp(loc, DialectTy, builder, newAllocOp, target);
      targetValues.push_back(targetRefOp);
    }
    for (auto ctrl : controls) {
      auto controlRef =
          createExtractOp(loc, DialectTy, builder, newAllocOp, ctrl.qubit);
      controlValues.push_back(controlRef);
    }
    for (auto p : parameter) {
      // TODO: Apparently all the parameters are floats in QC, may be the case
      //       this is not always true
      auto constantOp = createArithConstantOp(loc, DialectTy, builder, p);
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
    }

    if (Gatety != Gate::UNKNOWN) {

      newop = createNewGate(loc, DialectTy, parseGateTy(Gatety), controlValues,
                            targetValues, params, builder);
    } else if (op->getType() == qc::Measure) {
      auto newResults = createMeasureOp(loc, DialectTy, builder, targetValues);
      NewMeasureOps.push_back({op.get(), newResults});
    }
  }

  for (auto &[oldMeasOp, numMeasurements] : MeasureOps) {
    if (numMeasurements == 1) {
      auto newResults = NewMeasureOps[0].Results;
      resolveSSAformForMeasureOps(oldMeasOp, newResults);
    }
  }
  // builder.create<func::ReturnOp>(loc);
#ifdef MQSS_ENABLE_DEBUG
  MQSS_DEBUG("Dumping QC after mapping:\n");
  qcMapped.print(std::cout);
#endif
}

void performMapping(MyModuleAnalysis &analysis, Architecture architecture,
                    Configuration settings, QuantumDialect DialectTy) {

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
        loadGateOpsIntoQC(Op, qview, qc, qview.isControlled());
        OpsToErase.insert(Op);
      }

      if (qview.isMeasureOp) {
        loadMeasureOpIntQC(qview, qc);
        MeasureOps[Op] = qview.measurements.size();
      }
    }
#ifdef MQSS_ENABLE_DEBUG
    MQSS_DEBUG("--> Before mapping, Dumping QC:\n");
    qc.print(std::cout);
#endif

    // Map the circuit
    const auto mapper = std::make_unique<HeuristicMapper>(qc, architecture);

    mapper->map(settings);

    auto qcMapped = qc::QuantumComputation();
    qcMapped = mapper->moveMappedCircuit();

    if (qcMapped.empty())
      llvm::errs() << "Could not Map circuit!"
                   << "\n";

    mlir::IRRewriter builder(kernel->getContext());
    mlir::Location loc = kernel.getLoc();

    mlir::Block &entry = kernel.getBody().front();

    // entry.clear();                         // erase old ops
    builder.setInsertionPointToStart(&entry); // CRITICAL

    // Location loc = kernel.getLoc();
    auto newAllocOp =
        createAllocOp(loc, DialectTy, builder, info.AllocatedQubits);

    // cleaning the mlir::funcOp corresponding to the quake circuit
    // analysis.clearKernelBody(kernel, ToErase);

    createMappedCircuit(builder, loc, DialectTy, newAllocOp, qcMapped,
                        MeasureOps);

    controlledDCE(OpsToErase, info.AllocOp, newAllocOp);
  }
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

    PipelineConfig config;
    if (!input.empty()) {
      MQSS_DEBUG("-->Reading Coupling Map from JSON input...\n");
      config = PipelineConfig::fromJSON(input);
    } else if (!qdmi.empty()) {
      MQSS_DEBUG("-->Selected QDMI device Mapping...\n");
      config = PipelineConfig::fromQDMI(qdmi);
    } else if (!coupling_map.empty()) {
      MQSS_DEBUG("-->Reading Custom Coupling Map...\n");
      config = PipelineConfig::fromCustomCouplingMap(coupling_map);
    } else {
      MQSS_DEBUG("--> No JSON or QDMI input, using pass defaults!" << "\n");
      config = PipelineConfig::defaults();
    }
    auto architecture = config.arch;
    auto settings = config.settings;

    auto &selector = getAnalysis<DialectAnalysisSelector>();
    auto &analysis = *selector.get();
    auto DialectTy = selector.getDialect();

    performMapping(analysis, architecture, settings, DialectTy);
    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
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
