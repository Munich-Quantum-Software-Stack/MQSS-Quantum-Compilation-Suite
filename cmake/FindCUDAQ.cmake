# ---- Fetch and build CUDA-Q from source if not found -----------
# Targets Built: QuakeDialect, CCDialect, QECDialect, OptimBuilder, OptCodeGen
# TODO(akshay): no-op touch to satisfy deps-image.yml's paths: filter on push,
# only to trigger a real build+publish run for testing on this branch before
# merge. Revert this comment before merging.
option(CUDAQ_AUTO_FETCH "Clone and build CUDA-Q from source if not found" OFF)
set(CUDAQ_GIT_REPOSITORY
    "https://github.com/NVIDIA/cuda-quantum.git"
    CACHE STRING "CUDA-Q git repository")
set(CUDAQ_GIT_TAG
    "0.15.0"
    CACHE STRING "CUDA-Q v0.15.0 build")

# ---- Check for an existing CUDA-Q installation first -----------------------
# Building CUDA-Q from source is a multi-minute-to-hour operation, so only do it
# as a last resort: first check whether it's already available via CUDAQ_ROOT,
# CUDAQ_DIR, CUDAQ_INSTALL_PREFIX, or the default CMake search paths (a system
# package, a previous manual build, etc).
find_path(
  CUDAQ_INCLUDE_DIR
  NAMES cudaq/Optimizer/Dialect/Quake/QuakeOps.h
  HINTS ${CUDAQ_ROOT} ${CUDAQ_DIR} ENV CUDAQ_INSTALL_PREFIX
  PATH_SUFFIXES include)
find_path(
  CUDAQ_GENERATED_INCLUDE_DIR
  NAMES cudaq/Optimizer/Dialect/Quake/QuakeOps.h.inc
  HINTS ${CUDAQ_ROOT} ${CUDAQ_DIR} ENV CUDAQ_INSTALL_PREFIX
  PATH_SUFFIXES include)

function(_cudaq_find_lib _var)
  find_library(
    ${_var}
    NAMES ${ARGN}
    HINTS ${CUDAQ_ROOT} ${CUDAQ_DIR} ENV CUDAQ_INSTALL_PREFIX
    PATH_SUFFIXES lib lib64)
  mark_as_advanced(${_var})
endfunction()

_cudaq_find_lib(CUDAQ_QUAKE_LIBRARY QuakeDialect)
_cudaq_find_lib(CUDAQ_CC_LIBRARY CCDialect)
_cudaq_find_lib(CUDAQ_QEC_LIBRARY QECDialect)
_cudaq_find_lib(CUDAQ_BUILDER_LIBRARY OptimBuilder)
_cudaq_find_lib(CUDAQ_CODEGEN_LIBRARY OptCodeGen)

if(CUDAQ_INCLUDE_DIR
   AND CUDAQ_GENERATED_INCLUDE_DIR
   AND CUDAQ_QUAKE_LIBRARY
   AND CUDAQ_CC_LIBRARY
   AND CUDAQ_QEC_LIBRARY)
  message(
    STATUS "FindCUDAQ: found existing installation (${CUDAQ_INCLUDE_DIR})")
elseif(CUDAQ_AUTO_FETCH AND NOT DEFINED ENV{CUDAQ_INSTALL_PREFIX})
  set(_cudaq_src "${CMAKE_CURRENT_BINARY_DIR}/_deps/cudaq-src")
  set(_cudaq_build "${CMAKE_CURRENT_BINARY_DIR}/_deps/cudaq-build")
  set(_cudaq_install "${CMAKE_CURRENT_BINARY_DIR}/_deps/cudaq-install")

  # CUDA-Q must be built against the same LLVM this project uses, so require
  # that find_package(MLIR CONFIG) already ran.
  if(NOT LLVM_DIR OR NOT MLIR_DIR)
    message(
      FATAL_ERROR
        "CUDAQ_AUTO_FETCH requires LLVM_DIR/MLIR_DIR to already be set "
        "(call find_package(MLIR CONFIG) before find_package(CUDAQ)), so "
        "CUDA-Q builds against this project's LLVM, not its own pinned one. "
        "Note this does NOT guarantee that LLVM is the commit CUDA-Q expects "
        "-- the version guard further down in this file still applies.")
  endif()

  set(_cudaq_marker
      "${_cudaq_install}/include/cudaq/Optimizer/Dialect/Quake/QuakeOps.h")
  if(NOT EXISTS "${_cudaq_marker}")
    message(STATUS "FindCUDAQ: no install found; fetching+building CUDA-Q "
                   "'${CUDAQ_GIT_TAG}' -- first run may take a long time")

    if(NOT EXISTS "${_cudaq_src}/.git")
      execute_process(
        COMMAND git clone --depth 1 --branch ${CUDAQ_GIT_TAG}
                ${CUDAQ_GIT_REPOSITORY} ${_cudaq_src} RESULT_VARIABLE _cudaq_rc)
      if(NOT _cudaq_rc EQUAL 0)
        message(FATAL_ERROR "FindCUDAQ: git clone failed (exit ${_cudaq_rc})")
      endif()
    endif()

    set(_cudaq_lib_cmakelists "${_cudaq_src}/cudaq/tools/CMakeLists.txt")
    file(READ "${_cudaq_lib_cmakelists}" _cudaq_lib_cmakelists_content)
    string(
      REPLACE "    add_subdirectory(nvqpp)"
              "#    add_subdirectory(nvqpp) -- patched out by FindCUDAQ.cmake"
              _cudaq_lib_cmakelists_content "${_cudaq_lib_cmakelists_content}")
    file(WRITE "${_cudaq_lib_cmakelists}" "${_cudaq_lib_cmakelists_content}")

    execute_process(
      COMMAND
        ${CMAKE_COMMAND} -G Ninja -S ${_cudaq_src} -B ${_cudaq_build}
        -DCMAKE_INSTALL_PREFIX=${_cudaq_install} -DCMAKE_BUILD_TYPE=Release
        -DLLVM_DIR=${LLVM_DIR} -DMLIR_DIR=${MLIR_DIR}
        -DCMAKE_CXX_FLAGS_RELEASE=-O1 -DCUDAQ_ENABLE_PYTHON=OFF
        -DCUDAQ_BUILD_TESTS=OFF -DCUDAQ_DISABLE_RUNTIME=ON
        -DCMAKE_DISABLE_FIND_PACKAGE_Clang=TRUE
      RESULT_VARIABLE _cudaq_rc)
    if(NOT _cudaq_rc EQUAL 0)
      message(FATAL_ERROR "FindCUDAQ: configure failed (exit ${_cudaq_rc})")
    endif()

    include(ProcessorCount)
    ProcessorCount(_cudaq_nproc)
    if(_cudaq_nproc EQUAL 0)
      set(_cudaq_nproc 2)
    endif()

    # CodeGen translation units are memory-heavy (multi-dialect MLIR templates
    # at -O3); uncapped parallelism can OOM-kill cc1plus in constrained
    # containers even on machines with many cores.
    set(CUDAQ_BUILD_PARALLELISM
        "${_cudaq_nproc}"
        CACHE
          STRING
          "Parallel build jobs for the CUDA-Q auto-fetch build (lower this if cc1plus gets OOM-killed)"
    )

    execute_process(
      COMMAND
        ${CMAKE_COMMAND} --build ${_cudaq_build} --target QuakeDialect CCDialect
        QECDialect OptimBuilder OptCodeGen -- -j${CUDAQ_BUILD_PARALLELISM}
      RESULT_VARIABLE _cudaq_rc)
    if(NOT _cudaq_rc EQUAL 0)
      message(FATAL_ERROR "FindCUDAQ: build failed (exit ${_cudaq_rc})")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} --install ${_cudaq_build}
                            --component QuakeDialect RESULT_VARIABLE _cudaq_rc)
    if(_cudaq_rc EQUAL 0)
      execute_process(COMMAND ${CMAKE_COMMAND} --install ${_cudaq_build}
                              --component CCDialect RESULT_VARIABLE _cudaq_rc)
    endif()
    if(_cudaq_rc EQUAL 0)
      execute_process(COMMAND ${CMAKE_COMMAND} --install ${_cudaq_build}
                              --component QECDialect RESULT_VARIABLE _cudaq_rc)
    endif()
    if(_cudaq_rc EQUAL 0)
      execute_process(
        COMMAND ${CMAKE_COMMAND} --install ${_cudaq_build} --component
                OptimBuilder RESULT_VARIABLE _cudaq_rc)
    endif()
    if(_cudaq_rc EQUAL 0)
      execute_process(COMMAND ${CMAKE_COMMAND} --install ${_cudaq_build}
                              --component OptCodeGen RESULT_VARIABLE _cudaq_rc)
    endif()
    if(NOT _cudaq_rc EQUAL 0)
      message(FATAL_ERROR "FindCUDAQ: install failed (exit ${_cudaq_rc})")
    endif()
  else()
    message(STATUS "FindCUDAQ: reusing cached auto-build at ${_cudaq_install}")
  endif()

  set(CUDAQ_ROOT "${_cudaq_install}")

  # The scratch build's headers live in the clone/build tree, not the install
  # prefix (only the libraries above get installed) -- re-run the searches from
  # the pre-check above now that CUDA-Q has just been built. find_path and
  # find_library both retry automatically since the pre-check left these cache
  # variables as NOTFOUND.
  find_path(
    CUDAQ_INCLUDE_DIR
    NAMES cudaq/Optimizer/Dialect/Quake/QuakeOps.h
    HINTS ${CMAKE_CURRENT_BINARY_DIR}/_deps/cudaq-src/cudaq
    PATH_SUFFIXES include
    NO_DEFAULT_PATH)
  find_path(
    CUDAQ_GENERATED_INCLUDE_DIR
    NAMES cudaq/Optimizer/Dialect/Quake/QuakeOps.h.inc
    HINTS ${CMAKE_CURRENT_BINARY_DIR}/_deps/cudaq-build/cudaq
    PATH_SUFFIXES include
    NO_DEFAULT_PATH)
  _cudaq_find_lib(CUDAQ_QUAKE_LIBRARY QuakeDialect)
  _cudaq_find_lib(CUDAQ_CC_LIBRARY CCDialect)
  _cudaq_find_lib(CUDAQ_QEC_LIBRARY QECDialect)
  _cudaq_find_lib(CUDAQ_BUILDER_LIBRARY OptimBuilder)
  _cudaq_find_lib(CUDAQ_CODEGEN_LIBRARY OptCodeGen)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  CUDAQ
  REQUIRED_VARS
    CUDAQ_INCLUDE_DIR
    CUDAQ_GENERATED_INCLUDE_DIR
    CUDAQ_QUAKE_LIBRARY
    CUDAQ_QEC_LIBRARY
    CUDAQ_CC_LIBRARY
    CUDAQ_BUILDER_LIBRARY
    CUDAQ_CODEGEN_LIBRARY)

if(CUDAQ_FOUND)
  set(CUDAQ_INCLUDE_DIRS ${CUDAQ_INCLUDE_DIR} ${CUDAQ_GENERATED_INCLUDE_DIR})
  list(REMOVE_DUPLICATES CUDAQ_INCLUDE_DIRS)

  if(NOT TARGET CUDAQ::CC)
    add_library(CUDAQ::CC UNKNOWN IMPORTED GLOBAL)
    set_target_properties(
      CUDAQ::CC
      PROPERTIES
        IMPORTED_LOCATION "${CUDAQ_CC_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${CUDAQ_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES
        "MLIRComplexDialect;MLIRControlFlowDialect;MLIRIR;MLIRLLVMDialect;MLIRLoopLikeInterface;MLIRFuncDialect"
    )
  endif()

  if(NOT TARGET CUDAQ::Quake)
    add_library(CUDAQ::Quake UNKNOWN IMPORTED GLOBAL)
    set_target_properties(
      CUDAQ::Quake
      PROPERTIES IMPORTED_LOCATION "${CUDAQ_QUAKE_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${CUDAQ_INCLUDE_DIRS}"
                 INTERFACE_LINK_LIBRARIES "CUDAQ::CC;MLIRIR;MLIRFuncDialect")
  endif()

  if(NOT TARGET CUDAQ::QEC)
    add_library(CUDAQ::QEC UNKNOWN IMPORTED GLOBAL)
    set_target_properties(
      CUDAQ::QEC
      PROPERTIES IMPORTED_LOCATION "${CUDAQ_QEC_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${CUDAQ_INCLUDE_DIRS}"
                 INTERFACE_LINK_LIBRARIES "CUDAQ::Quake;CUDAQ::CC;MLIRIR")
  endif()

  if(CUDAQ_CODEGEN_LIBRARY AND NOT TARGET CUDAQ::CodeGen)
    add_library(CUDAQ::CodeGen UNKNOWN IMPORTED GLOBAL)
    set_target_properties(
      CUDAQ::CodeGen
      PROPERTIES
        IMPORTED_LOCATION "${CUDAQ_CODEGEN_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${CUDAQ_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES
        "CUDAQ::CC;MLIRIR;MLIRSupport;MLIRFuncDialect;MLIRArithDialect;MLIRControlFlowDialect"
    )
  endif()

  if(CUDAQ_BUILDER_LIBRARY AND NOT TARGET CUDAQ::Builder)
    add_library(CUDAQ::Builder UNKNOWN IMPORTED GLOBAL)
    set_target_properties(
      CUDAQ::Builder
      PROPERTIES
        IMPORTED_LOCATION "${CUDAQ_BUILDER_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${CUDAQ_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES
        "CUDAQ::CC;MLIRIR;MLIRSupport;MLIRFuncDialect;MLIRArithDialect;MLIRControlFlowDialect"
    )
  endif()

endif()
