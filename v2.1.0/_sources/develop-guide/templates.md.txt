<!--------------------------------------------------------------------------------------------------
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
----------------------------------------------------------------------------------------------------->

# Writing Passes for the MQSS

Following text provides guides you through the process of adding an MLIR pass and adding an MLIR
dialect to the MQSS Quantum Compilation Suite

## Adding your pass

- Two types of passes can be added : MLIR Dialect-Agnostic and Dialect-specific
- Both type of Passes can be added within `lib/Passes`.
- A Dialect Agnostic Pass should make use of our abstractions mentioned in
  `include/Passes/Analysis`. The key abstraction here is the interface class `MyModuleAnalysis`. For
  every supported dialect, an implementation of this interface should be provided. Currently, We
  have two such implementations : `QuakeAnalysis` and `CatalystQuantumAnalysis` for Quake and
  Catalyst-quantum dialects.

- There is a `DialectAnalysisSelector` class within `DialectAnalysisSelector.h` which selects the
  appropriate `MyModuleAnalysis` object based on the detected dialect of the input MLIR module.

- A key step in adding your pass is pass registration. This ensures that the passes can be found by
  the target `mqss-opt`.

  - To register a pass we need to make additions to 2 files `Transforms.h` and `Transforms.td`.
    These files can be found within `include/Passes/Transforms` .

  - Here is an example of pass registration within `Transforms.td`:

  ```sh
    def CommonNormalizeArgAnglePass : Pass<"CommonNormalizeArgAnglePass", "mlir::ModuleOp"> {
      let summary = "Normalize Arg angle of RX, RY, RZ gates. ";

      let description = [{
        Normalize Arg angle of RX, RY, RZ gates.
      }];

      let constructor = "mqss::opt::CommonNormalizeArgAnglePass()";
    }

  ```

  The pass is registered for the `mqss-opt` target. The pass does not have any options.

  - If your pass will need options/arguments to be passed, you can add the field:

    ```
    let options = [
    Option<"<optional-name>", "mode", "<option-type>",
      /*default=*/"\"<default value>\"", "Description of Options.">,
    ];
    ```

  - In `Transforms.h` within the `mqss::opt` namespace, the following should be added:

    ```sh
       std::unique_ptr<mlir::Pass> CommonNormalizeArgAnglePass();
    ```

    The above line is based on the definition, signature and structure of the pass viz. discussed in
    the next section.

## Pass Structure

A basic pass structure is as shown:

```c++

     #include "Passes/Transforms/PassUtils.h"

      using namespace mlir;
      using namespace llvm;

      namespace mqss::opt {

      #define GEN_PASS_DEF_COMMONCNOTREVERSEPASS
      #include "Passes/Transforms/Transforms.h.inc"

      } // namespace mqss::opt

     namespace {


      class CommonCNOTReverse : public mqss_backend::impl::CommonCNOTReversePassBase<CommonCNOTReverse> {

      public:

        // Default constructor (required for pass registry)
        CommonCNOTReverse() = default;

        void runOnOperation() override {
            ...;
        }
      };

      } // namespace

      std::unique_ptr<mlir::Pass> mqss_backend::CommonCNOTReversePass() {
        return std::make_unique<CommonCNOTReverse>();
      }

```

After the pass is registered within `Transforms.td`, and the project is re-built, a templated class
is automatically created within `build/include/Passes/Transforms.h.inc`. The class looks like this:

```mlir
#ifdef GEN_PASS_DECL_COMMONCNOTREVERSEPASS
#undef GEN_PASS_DECL_COMMONCNOTREVERSEPASS
#endif // GEN_PASS_DECL_COMMONCNOTREVERSEPASS
#ifdef GEN_PASS_DEF_COMMONCNOTREVERSEPASS
namespace impl {

template <typename DerivedT>
class CommonCNOTReversePassBase : public ::mlir::OperationPass<mlir::ModuleOp> {
public:
  using Base = CommonCNOTReversePassBase;

  CommonCNOTReversePassBase() : ::mlir::OperationPass<mlir::ModuleOp>(::mlir::TypeID::get<DerivedT>()) {}
  CommonCNOTReversePassBase(const CommonCNOTReversePassBase &other) : ::mlir::OperationPass<mlir::ModuleOp>(other) {}
  CommonCNOTReversePassBase& operator=(const CommonCNOTReversePassBase &) = delete;
  CommonCNOTReversePassBase(CommonCNOTReversePassBase &&) = delete;
  CommonCNOTReversePassBase& operator=(CommonCNOTReversePassBase &&) = delete;
  ~CommonCNOTReversePassBase() = default;

  /// Returns the command-line argument attached to this pass.
  static constexpr ::llvm::StringLiteral getArgumentName() {
    return ::llvm::StringLiteral("CommonCNOTReversePass");
  }
  ::llvm::StringRef getArgument() const override { return "CommonCNOTReversePass"; }

  ::llvm::StringRef getDescription() const override { return R"PD(Reverse the control and targets of each CNot gate in a circuit)PD"; }

  /// Returns the derived pass name.
  static constexpr ::llvm::StringLiteral getPassName() {
    return ::llvm::StringLiteral("CommonCNOTReversePass");
  }
  ::llvm::StringRef getName() const override { return "CommonCNOTReversePass"; }

  /// Support isa/dyn_cast functionality for the derived pass class.
  static bool classof(const ::mlir::Pass *pass) {
    return pass->getTypeID() == ::mlir::TypeID::get<DerivedT>();
  }

  /// A clone method to create a copy of this pass.
  std::unique_ptr<::mlir::Pass> clonePass() const override {
    return std::make_unique<DerivedT>(*static_cast<const DerivedT *>(this));
  }

  /// Return the dialect that must be loaded in the context before this pass.
  void getDependentDialects(::mlir::DialectRegistry &registry) const override {

  }

  /// Explicitly declare the TypeID for this class. We declare an explicit private
  /// instantiation because Pass classes should only be visible by the current
  /// library.
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CommonCNOTReversePassBase<DerivedT>)

protected:
private:
};
} // namespace impl
#undef GEN_PASS_DEF_COMMONCNOTREVERSEPASS
#endif // GEN_PASS_DEF_COMMONCNOTREVERSEPASS
```

The pass class `CommonCNOTReverse` inherits from the `pass` base class within:
`build/_deps/LLVM-22.1.0-toolchain/include/mlir/Pass/Pass.h`.

## Adding a Dialect

Adding a dialect requires definitions for its Operations, Types, Attributes, Interfaces, and related
constructs. These definitions are emitted by TableGen into `.h.inc` (declarations) and `.cpp.inc`
(definitions) files, which are generated automatically as part of a successful build.</br> Our
compiler depends on the generated headers and sources for the Quake and Catalyst-quantum dialects.
To obtain them, we selectively build cudaq-quantum (referred to as CUDAQ) and Catalyst from source,
wiring them in as external CMake dependencies:

- Quake dialect — provided by CUDAQ. We build its `QuakeDialect` target. See cmake/FindCUDAQ.cmake.
- Catalyst-quantum dialect — provided by Catalyst. The equivalent target is `MLIRQuantum`. See
  cmake/FindCatalyst.cmake.

For details on how each dialect's definitions are declared, refer to the upstream source or within
`build/_deps` under the respective framework's include/ directory. To add a dialect to our compiler
and enable our quantum abstractions on top of it, follow these steps:

- An implementation of the `MyModuleAnalysis` class for the dialect needs to be added within
  `include/Passes/Analysis`. The name of this file should follow the convention :
  "{Dialect-Name}"Extractor.h.
- Include the header files with the required dialect definitions within
  `include/Utils/dialectutils.h`.
- Add an entry for the dialect within `include/Analysis/DialectAnalysisSelector.h`.
- Register the dialect within `mqss-cc.cpp` (`registry.insert<>()`).
