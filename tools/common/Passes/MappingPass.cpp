

#include "include/PassUtils.h"

using namespace mlir;
using namespace llvm;
using namespace qc;
namespace {

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

void loadControlledGates(mlir::Operation *gateOp, QuantumOpView qview,
                         QuantumComputation &qc) {

  auto controlQubitVector = qview.getQubits(QubitRole::Control).ids;
  auto targetQubitVector = qview.getQubits(QubitRole::Target).ids;
  assert((controlQubitVector.size() == 1 && targetQubitVector.size() == 1) &&
         "Only upto 2-Qubit gates supported!");
  auto controlQubit = controlQubitVector[0].index;
  auto targetQubit = targetQubitVector[0].index;

  llvm::outs() << "Gate Op: " << *gateOp << "\n";
  if ((controlQubit == -1) && (targetQubit == -1)) {
    // Gate operation in Catalyst (value semantics)
    // Qubit in Catalyst
    SmallVector<std::optional<QubitID>, 2> OriginQubits;
    for (auto operand : gateOp->getOperands()) {
      auto originOp = getOriginQubit(operand);
      OriginQubits.push_back(originOp);
    }

    if (qview.GateTy == Gate::CNOT) {
      qc.cx(OriginQubits[0]->index, OriginQubits[1]->index);
    }
  } else {
    if (qview.GateTy == Gate::CNOT) {
      qc.cx(controlQubit, targetQubit);
    }
  }
  // Gate operation in Quake (reference semantics)
}

void loadMeasureOp(mlir::Operation *measureOp, QuantumOpView qview,
                   QuantumComputation &qc) {

  auto targetQubitVector = qview.getQubits(QubitRole::Target).ids;
  assert((targetQubitVector.size() == 1) &&
         "Only Single Qubit Measurement Ops supported");

  auto targetQubit = targetQubitVector[0].index;
  llvm::outs() << "measure Op: " << *measureOp << "\n";
  for (auto entry : qview.measurements) {
    llvm::outs().indent(4) << "meas: " << entry.QubitIndex << " -> "
                           << entry.ClassicalBitIndex << "\n";
    qc.measure(entry.QubitIndex, entry.ClassicalBitIndex);
  }
}

void createMappedCircuit(func::FuncOp &kernel, size_t AllocatedQubits,
                         QuantumComputation &qcMapped) {

  mlir::IRRewriter builder(kernel->getContext());
  mlir::Location loc = kernel.getLoc();

  mlir::Block &entry = kernel.getBody().front();

  // entry.clear();                         // erase old ops
  builder.setInsertionPointToEnd(&entry); // CRITICAL

  // Location loc = kernel.getLoc();
  Value newAllocOp = createAllocOp(loc, builder, AllocatedQubits);
  llvm::outs().indent(4) << "Alloc Op: " << newAllocOp << "\n";

  for (const auto &op : qcMapped) {
    if (op->getType() == qc::Barrier)
      continue;
    auto &targets = op->getTargets();
    auto &controls = op->getControls();
    auto parameter = op->getParameter();

    // defining the list of controls, targets and parameters
    SmallVector<Value, 2> params = {};
    SmallVector<Value, 2> controlValues = {};
    SmallVector<Value, 2> targetValues = {};

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
      llvm::APFloat constantValue(p);
      auto constantOp = createArithFloatOp(loc, builder, constantValue);
      llvm::outs().indent(4) << "-->Const Op: " << constantOp << "\n";
      params.push_back(constantOp);
    }

    llvm::outs().indent(4) << "-->Gate ty to create: " << op->getType() << "\n";

    mlir::Operation *newop;

    if (op->getType() == qc::X) {
      newop = createNewGate(loc, parseGateTy(Gate::CNOT), controlValues,
                            targetValues, params, builder);
      llvm::outs().indent(4) << "-->CNOT: " << *newop << "\n";
    } else if (op->getType() == qc::H) {
      newop = createNewGate(loc, parseGateTy(Gate::H), controlValues,
                            targetValues, params, builder);
      llvm::outs().indent(4) << "-->H: " << *newop << "\n";
    } else if (op->getType() == qc::SWAP) {
      newop = createNewGate(loc, parseGateTy(Gate::SWAP), controlValues,
                            targetValues, params, builder);
      llvm::outs().indent(4) << "-->SWAP: " << *newop << "\n";
    }
  }

  builder.create<func::ReturnOp>(loc);
#ifdef DEBUG
  std::cout << "Dumping QC after mapping:\n";
  qcMapped.print(std::cout);
#endif
}

void performMapping(MyModuleAnalysis &analysis, Architecture architecture,
                    Configuration settings) {

  for (auto &[kernel, info] : analysis.getKernelDialectInfo()) {

    llvm::outs() << "\nkernel: " << kernel.getSymName()
                 << " total input qubits: " << info.AllocatedQubits
                 << " Measure qubits: " << info.NumMeasureQubits << "\n\n";

    qc::QuantumComputation qc{info.AllocatedQubits, info.NumMeasureQubits};
    for (auto &[Op, qview] : info.OpQViewMap) {
      if (qview.GateTy != Gate::UNKNOWN && qview.isControlled()) {
        loadControlledGates(Op, qview, qc);
      }
      if (qview.isMeasureOp) {
        loadMeasureOp(Op, qview, qc);
      }
    }

    llvm::errs() << "--> Before mapping, Dumping QC:\n";
    qc.print(std::cout);

    // Map the circuit
    const auto mapper = std::make_unique<HeuristicMapper>(qc, architecture);
    mapper->map(settings);

    auto qcMapped = qc::QuantumComputation();
    qcMapped = mapper->moveMappedCircuit();

    // cleaning the mlir::funcOp corresponding to the quake circuit

    analysis.clearKernelBody(kernel);

    createMappedCircuit(kernel, info.AllocatedQubits, qcMapped);
  }
}

class Mapping : public mqss_backend::CommonMappingPassBase<Mapping> {

  void runOnOperation() override {

    // Note: Dialect specific analysis is needed to proceed
    //       This is needed currently because we do not "parse" the dialects.
    //        Parsing would involve a more sophisticated Internal IR to
    //        represent operations of all supported dialects.

    llvm::outs() << "\n[Applying Pass: MappingPass]\n";

    PipelineConfig config;
    if (!input.empty())
      config = PipelineConfig::fromFile(input);
    else {
      llvm::outs() << "--> No input file provided, using pass defaults!"
                   << "\n\n";
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
  return std::make_unique<Mapping>();
}
