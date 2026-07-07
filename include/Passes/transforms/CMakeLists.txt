
set(LLVM_TARGET_DEFINITIONS Transforms.td)

mlir_tablegen(Transforms.h.inc
  -gen-pass-decls
  -name MQSSTransforms
)

add_public_tablegen_target(MQSSTransformsPassIncGen)