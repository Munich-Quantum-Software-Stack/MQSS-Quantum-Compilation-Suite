

/* This code and any associated documentation is provided "as is"

Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/passes/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
*************************************************************************
  author Akshay Bhosale
  date   February 2026
  version 1.0
*************************************************************************/

#include "include/transforms/TranspilationPassUtils.h"
#include "include/transforms/QdmiDriver.h"
#include <llvm/Support/raw_ostream.h>

using namespace mlir;
using namespace llvm;
using namespace qc;
namespace {

// TODO: Use Exact Mapper if Number of Qubits >=8, Heursitic Mapper otherwise.
struct PipelineConfig {
  Architecture arch;
  Configuration settings;

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
    const CouplingMap cm = {{0, 1}, {1, 0}, {1, 2}, {2, 1}, {2, 3},
                            {3, 2}, {3, 4}, {4, 3}, {4, 0}, {0, 4}};
    config.arch.loadCouplingMap(5, cm);
    // Defining the settings of the mqt-mapper
    config.settings.heuristic = Heuristic::GateCountMaxDistance;
    config.settings.layering = Layering::DisjointQubits;
    config.settings.initialLayout = InitialLayout::Identity;
    config.settings.preMappingOptimizations = false;
    config.settings.postMappingOptimizations = false;
    config.settings.lookaheadHeuristic = LookaheadHeuristic::None;
    config.settings.debug = false;
    config.settings.addMeasurementsToMappedCircuit = true;
    return config;
  };

  static PipelineConfig fromFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open())
      throw std::runtime_error("Could not open config file: " + path);

    nlohmann::json j;
    file >> j;

    PipelineConfig config;

    // --- Architecture ---
    const auto &arch = j.at("architecture");
    CouplingMap cm;
    for (const auto &pair : arch.at("coupling_map"))
      cm.insert({pair[0].get<int>(), pair[1].get<int>()});
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
    qdmi_main_driver::DeviceSessionConfig Deviceconfig;
    

    MQSS_DEBUG("Device conf is: " << device_conf << "\n");
    auto dev = createQDMIDevice(device_conf.c_str());

    auto device_porperties = getDeviceProperties(dev);

    auto numQubits = device_porperties.numQubits;
    auto device_coupling_map = device_porperties.cm;
    assert(!device_coupling_map.empty() &&
           "Could not fetch the coupling map of QDMI device!");

    config.arch.loadCouplingMap(numQubits, device_coupling_map);
    MQSS_DEBUG("Device Coupling Map loaded!!");
    // Defining the settings of the mqt-mapper
    config.settings.heuristic = Heuristic::GateCountMaxDistance;
    config.settings.layering = Layering::DisjointQubits;
    config.settings.initialLayout = InitialLayout::Identity;
    config.settings.preMappingOptimizations = false;
    config.settings.postMappingOptimizations = false;
    config.settings.lookaheadHeuristic = LookaheadHeuristic::None;
    config.settings.debug = false;
    config.settings.addMeasurementsToMappedCircuit = true;

    return config;
  }

private:
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

std::optional<double> getConstantDouble(mlir::Value v) {
  auto defOp = v.getDefiningOp<mlir::arith::ConstantOp>();
  if (!defOp)
    return std::nullopt;

  auto attr = llvm::dyn_cast<mlir::FloatAttr>(defOp.getValue());
  if (!attr)
    return std::nullopt;

  return attr.getValueAsDouble();
}

void resolveSSAformForMeasureOps(mlir::Operation *OldMeasOp,
                                 SmallVector<mlir::Value, 2> newResults) {
  // llvm::outs() << "Old meas op: " << *OldMeasOp
  //              << " results: " << OldMeasOp->getResults().size() << ","
  //              << newResults.size() << "\n";
  auto OldResults = OldMeasOp->getResults();
  for (int j = 0; j < OldResults.size(); ++j) {
    auto oldRes = OldResults[j];
    auto newRes = newResults[j];
    oldRes.replaceAllUsesWith(newRes);
  }
  assert(OldMeasOp->use_empty() &&
         "Old measurement Op should not have any users before erasing!");
  OldMeasOp->erase();
}

// Performs a controlled Dead-Code elimination optimization.
// It is less aggressive than MLIR's canonicalize and cse optimizations.
// canonicalize and cse can remove newly introduced gates.
void controlledDCE(SmallPtrSet<mlir::Operation *, 16> OpsToErase,
                   mlir::Value OldAllocaOp, mlir::Value newAllocOp) {
  llvm::SmallPtrSet<mlir::Operation *, 16> operandsToCleanup;

  for (mlir::Operation *op : OpsToErase) {

    for (mlir::Value operand : op->getOperands()) {
      if (!OpsToErase.contains(operand.getDefiningOp()))
        operandsToCleanup.insert(operand.getDefiningOp());
    }
  }

  eraseOpsSafely(OpsToErase);
  eraseOpsSafely(operandsToCleanup);

  OldAllocaOp.replaceAllUsesWith(newAllocOp);
  assert(OldAllocaOp.use_empty() &&
         "Old alloca cannot have uses before being erased!");
  OldAllocaOp.getDefiningOp()->erase();
}

// Load the measurement operation within QuantumComputation
void loadMeasureIntQC(QuantumOpView qview, QuantumComputation &qc) {

  auto targetQubitVector = qview.getQubits(QubitRole::Target).ids;
  assert((targetQubitVector.size() == 1) &&
         "Only Single Qubit Measurement Ops supported");

  auto targetQubit = targetQubitVector[0].index;
  for (auto entry : qview.measurements) {
    qc.measure(entry.QubitIndex, entry.ClassicalBitIndex);
  }
}

void loadGates(mlir::Operation *gateOp, QuantumComputation &qc,
               int64_t controlQubit, int64_t targetQubit, Gate GateTy,
               SmallVector<mlir::Value, 2> params) {

  // llvm::outs() << "Gate Op: " << *gateOp << " , Ty: " << GateTy << "\n";

  std::optional<double> angle;
  // TODO: Load more gates into qc. Add to the following list.
  switch (GateTy) {
  case Gate::CNOT:
    qc.cx(controlQubit, targetQubit);
    break;
  case Gate::CY:
    qc.cy(controlQubit, targetQubit);
    break;
  case Gate::CZ:
    qc.cz(controlQubit, targetQubit);
    break;
  case Gate::PauliX:
    qc.x(targetQubit);
    break;
  case Gate::PauliY:
    qc.y(targetQubit);
    break;
  case Gate::PauliZ:
    qc.z(targetQubit);
    break;
  case Gate::S:
    qc.s(targetQubit);
    break;
  case Gate::T:
    qc.t(targetQubit);
    break;
  case Gate::RX:
    assert(params.size() == 1 && "RX gate should have only 1 parameter!");
    angle = getConstantDouble(params[0]);
    qc.rx(angle.value(), targetQubit);
    break;
  case Gate::RY:
    assert(params.size() == 1 && "RY gate should have only 1 parameter!");
    angle = getConstantDouble(params[0]);
    qc.ry(angle.value(), targetQubit);
    break;
  case Gate::RZ:
    assert(params.size() == 1 && "RZ gate should have only 1 parameter!");
    angle = getConstantDouble(params[0]);
    qc.rz(angle.value(), targetQubit);
    break;
  }
}

void loadGatesIntoQC(mlir::Operation *gateOp, QuantumOpView qview,
                     QuantumComputation &qc, bool isControlled = false) {

  int64_t controlQubitIdx = -2;
  int64_t targetQubitIdx = -2;
  auto targetQubitVector = qview.getQubits(QubitRole::Target).ids;

  if (isControlled) {
    auto controlQubitVector = qview.getQubits(QubitRole::Control).ids;
    assert((controlQubitVector.size() == 1) &&
           (targetQubitVector.size() == 1) &&
           "Only upto 2-Qubit gates supported!");

    controlQubitIdx = controlQubitVector[0].index;
    if (controlQubitIdx == -1) {
      // Gate operation in Catalyst (value semantics)
      // Qubit in Catalyst
      auto operand = controlQubitVector[0].base;
      controlQubitIdx = getOriginQubit(operand)->index;
    }
  }
  assert((targetQubitVector.size() == 1) &&
         "Only upto 1-Qubit Non-controlled gates supported!");

  targetQubitIdx = targetQubitVector[0].index;

  if (targetQubitIdx == -1) {
    // Gate operation in Catalyst (value semantics)
    // Qubit in Catalyst
    auto operand = targetQubitVector[0].base;
    targetQubitIdx = getOriginQubit(operand)->index;
  }
  loadGates(gateOp, qc, controlQubitIdx, targetQubitIdx, qview.GateTy,
            qview.params);
}

struct MeasureMentOpInfoTy {
  const qc::Operation *MeasOp;
  SmallVector<mlir::Value, 2> Results;
};

void createMappedCircuit(mlir::IRRewriter &builder, Location loc,
                         mlir::Value newAllocOp, QuantumComputation &qcMapped,
                         MapVector<mlir::Operation *, int> MeasureOps) {

  std::vector<MeasureMentOpInfoTy> NewMeasureOps;

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
      auto targetRefOp = createExtractOp(loc, builder, newAllocOp, target);
      targetValues.push_back(targetRefOp);
    }
    for (auto ctrl : controls) {
      auto controlRef = createExtractOp(loc, builder, newAllocOp, ctrl.qubit);
      controlValues.push_back(controlRef);
    }
    for (auto p : parameter) {
      // TODO: Apparently all the parameters are floats in QC, may be the case
      //       this is not always true
      auto constantOp = createArithConstantOp(loc, builder, p);
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

      newop = createNewGate(loc, parseGateTy(Gatety), controlValues,
                            targetValues, params, builder);
    } else if (op->getType() == qc::Measure) {
      auto newResults = createMeasureOp(loc, builder, targetValues);
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
                    Configuration settings) {

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
        loadGatesIntoQC(Op, qview, qc, qview.isControlled());
        OpsToErase.insert(Op);
      }

      if (qview.isMeasureOp) {
        loadMeasureIntQC(qview, qc);
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
    auto newAllocOp = createAllocOp(loc, builder, info.AllocatedQubits);

    // cleaning the mlir::funcOp corresponding to the quake circuit
    // analysis.clearKernelBody(kernel, ToErase);

    createMappedCircuit(builder, loc, newAllocOp, qcMapped, MeasureOps);

    controlledDCE(OpsToErase, info.AllocOp, newAllocOp);
  }
}

class CommonMapping
    : public mqss_backend::CommonMappingPassBase<CommonMapping> {

public:
  // Default constructor (required for pass registry)
  CommonMapping() = default;

  // Forward the options constructor to the base
  CommonMapping(const CommonMappingPassOptions &options)
      : CommonMappingPassBase() {
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
      config = PipelineConfig::fromFile(input);
    } else if (!qdmi.empty()) {
      MQSS_DEBUG("-->Selected QDMI device Mapping...\n");
      config = PipelineConfig::fromQDMI(qdmi);
    } else {
      MQSS_DEBUG("--> No input file provided, using pass defaults!"
                 << "\n");
      config = PipelineConfig::defaults();
    }
    auto architecture = config.arch;
    auto settings = config.settings;

    auto &analysis = getAnalysis<DialectAnalysis>();

    performMapping(analysis, architecture, settings);
    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};

} // namespace

std::unique_ptr<mlir::Pass> mqss_backend::CommonMappingPass() {
  return std::make_unique<CommonMapping>();
}

std::unique_ptr<mlir::Pass>
mqss_backend::createCommonMappingPass(const CommonMappingPassOptions &options) {
  return std::make_unique<CommonMapping>(options);
}