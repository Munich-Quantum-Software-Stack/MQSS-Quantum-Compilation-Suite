# Fetches the prebuilt LLVM/MLIR toolchain via MQT's setup-mlir.sh, mirroring
# build.sh's existing behavior, and sets LLVM_DIR/MLIR_DIR so the rest of the
# project's find_package(MLIR CONFIG) picks it up. Must be include()'d BEFORE
# find_package(MLIR CONFIG) in the top-level CMakeLists.txt, and before
# FindCUDAQ.cmake/FindCatalyst.cmake's auto-fetch logic runs (both require
# LLVM_DIR/MLIR_DIR already set).

option(
  MQSS_LLVM_AUTO_FETCH
  "Download the prebuilt LLVM/MLIR toolchain if LLVM_DIR/MLIR_DIR aren't already set"
  ON)

set(MQSS_LLVM_VERSION
    "22.1.0"
    CACHE STRING "LLVM/MLIR toolchain version to fetch")

if(MQSS_LLVM_AUTO_FETCH AND (NOT LLVM_DIR OR NOT MLIR_DIR))
  # Before downloading LLVM, check whether a matching LLVM/MLIR is already
  # available through the normal CMake package search (a system package, a
  # previous manual build, CMAKE_PREFIX_PATH, etc.).
  find_package(LLVM ${MQSS_LLVM_VERSION} QUIET CONFIG)
  find_package(MLIR ${MQSS_LLVM_VERSION} QUIET CONFIG)
endif()

if(MQSS_LLVM_AUTO_FETCH AND (NOT LLVM_DIR OR NOT MLIR_DIR))
  message(
    STATUS "MQSSFetchLLVM: no existing LLVM/MLIR ${MQSS_LLVM_VERSION} found")
  set(_mqss_llvm_root
      "${CMAKE_CURRENT_BINARY_DIR}/_deps/LLVM-${MQSS_LLVM_VERSION}-toolchain")

  if(NOT EXISTS "${_mqss_llvm_root}")
    message(
      STATUS
        "MQSSFetchLLVM: LLVM-${MQSS_LLVM_VERSION}-toolchain not found, downloading..."
    )
    execute_process(
      COMMAND
        bash -c
        "curl -LsSf https://github.com/munich-quantum-software/setup-mlir/releases/latest/download/setup-mlir.sh | bash -s -- -v ${MQSS_LLVM_VERSION} -p ${_mqss_llvm_root}"
      RESULT_VARIABLE _mqss_llvm_rc
      ERROR_VARIABLE _mqss_llvm_err)
    if(NOT _mqss_llvm_rc EQUAL 0)
      message(
        FATAL_ERROR
          "MQSSFetchLLVM: setup-mlir.sh failed (exit ${_mqss_llvm_rc}).\n${_mqss_llvm_err}"
      )
    endif()
  else()
    message(
      STATUS "MQSSFetchLLVM: reusing existing toolchain at ${_mqss_llvm_root}")
  endif()

  set(LLVM_DIR
      "${_mqss_llvm_root}/lib/cmake/llvm"
      CACHE PATH "LLVM cmake config dir" FORCE)
  set(MLIR_DIR
      "${_mqss_llvm_root}/lib/cmake/mlir"
      CACHE PATH "MLIR cmake config dir" FORCE)

  if(NOT EXISTS "${LLVM_DIR}" OR NOT EXISTS "${MLIR_DIR}")
    message(
      FATAL_ERROR
        "MQSSFetchLLVM: fetched toolchain but LLVM_DIR/MLIR_DIR don't exist at "
        "expected paths (${LLVM_DIR}, ${MLIR_DIR}) -- setup-mlir.sh's install "
        "layout may have changed; verify manually before trusting this module.")
  endif()
endif()
