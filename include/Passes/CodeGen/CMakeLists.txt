set(LLVM_TARGET_DEFINITIONS CodeGenPasses.td)

mlir_tablegen(CodeGen.h.inc -gen-pass-decls -name MQSSCodeGen)

add_public_tablegen_target(MQSSMQSSCodeGenPassIncGen)
