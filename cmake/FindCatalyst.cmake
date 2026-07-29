# Locates (or builds) PennyLane-Catalyst's MLIRQuantum dialect library.
#
# Output: CATALYST_FOUND CATALYST_INCLUDE_DIRS Imported target:
# CATALYST::Quantum

option(
  CATALYST_AUTO_FETCH
  "Clone and build Catalyst's MLIR Quantum dialect from source if not found"
  OFF)
set(CATALYST_GIT_REPOSITORY
    "https://github.com/PennyLaneAI/catalyst.git"
    CACHE STRING "Catalyst git repository")
# NOTE: unlike CUDA-Q's numbered releases, I have not confirmed Catalyst's
# actual release tag naming scheme. Defaulting to `main` deliberately rather
# than guessing a tag string -- verify real tags yourself before pinning: git
# ls-remote --tags https://github.com/PennyLaneAI/catalyst.git
set(CATALYST_GIT_TAG
    "v0.15.0"
    CACHE STRING "Catalyst git ref: tag, branch, or commit SHA")

set(CATALYST_BUILD_PARALLELISM
    "2"
    CACHE
      STRING
      "Parallel build jobs for the Catalyst auto-fetch build (lower this if cc1plus gets OOM-killed)"
)

if(CATALYST_AUTO_FETCH AND NOT CATALYST_ROOT)
  set(_catalyst_src "${CMAKE_BINARY_DIR}/_deps/catalyst-src")
  set(_catalyst_build "${CMAKE_BINARY_DIR}/_deps/catalyst-build")
  set(_catalyst_install "${CMAKE_BINARY_DIR}/_deps/catalyst-install")

  if(NOT LLVM_DIR OR NOT MLIR_DIR)
    message(
      FATAL_ERROR
        "CATALYST_AUTO_FETCH requires LLVM_DIR/MLIR_DIR to already be set ")
  endif()

  set(_catalyst_marker "${_catalyst_build}/lib/Quantum/IR/libMLIRQuantum.a")
  if(NOT EXISTS "${_catalyst_marker}")
    message(STATUS "FindCatalyst: no build found; fetching+building Catalyst "
                   "'${CATALYST_GIT_TAG}' (MLIRQuantum only)")

    if(NOT EXISTS "${_catalyst_src}/.git")
      execute_process(
        COMMAND git clone --depth 1 --branch ${CATALYST_GIT_TAG}
                ${CATALYST_GIT_REPOSITORY} ${_catalyst_src}
        RESULT_VARIABLE _catalyst_rc
        ERROR_VARIABLE _catalyst_err)
      if(NOT _catalyst_rc EQUAL 0)
        message(
          FATAL_ERROR
            "FindCatalyst: git clone failed (exit ${_catalyst_rc}) for ref "
            "'${CATALYST_GIT_TAG}'.\nGit said:\n${_catalyst_err}\n"
            "Check available refs with: git ls-remote --tags ${CATALYST_GIT_REPOSITORY}"
        )
      endif()
    endif()

    # Driver pulls in Enzyme (find_package(Enzyme CONFIG REQUIRED)), which we
    # have no use for when only consuming MLIRQuantum. Patch it out of our
    # scratch clone rather than building Enzyme just to satisfy an unrelated
    # tool's configure-time check.
    set(_catalyst_lib_cmakelists "${_catalyst_src}/mlir/lib/CMakeLists.txt")
    file(READ "${_catalyst_lib_cmakelists}" _catalyst_lib_cmakelists_content)
    string(
      REPLACE "add_subdirectory(Driver)"
              "# add_subdirectory(Driver) -- patched out by FindCatalyst.cmake"
              _catalyst_lib_cmakelists_content
              "${_catalyst_lib_cmakelists_content}")
    file(WRITE "${_catalyst_lib_cmakelists}"
         "${_catalyst_lib_cmakelists_content}")

    # IMPORTANT: unlike cuda-quantum, there is no repo-root CMakeLists.txt. The
    # CMake project lives at mlir/ -- configure THAT directory, not the clone
    # root.
    execute_process(
      COMMAND
        ${CMAKE_COMMAND} -G ${CMAKE_GENERATOR} -S ${_catalyst_src}/mlir -B
        ${_catalyst_build} -DCMAKE_INSTALL_PREFIX=${_catalyst_install}
        -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR=${LLVM_DIR} -DMLIR_DIR=${MLIR_DIR}
        -DCATALYST_DOCS_ONLY=ON
      RESULT_VARIABLE _catalyst_rc
      ERROR_VARIABLE _catalyst_err)
    if(NOT _catalyst_rc EQUAL 0)
      message(
        FATAL_ERROR
          "FindCatalyst: configure failed (exit ${_catalyst_rc}).\n${_catalyst_err}\n"
          "NOTE: Catalyst's mlir/CMakeLists.txt may require additional cache "
          "variables (options gating subdirectories, third-party deps like "
          "an mlir-hlo checkout, etc.) not accounted for here -- read the "
          "actual error above and mlir/CMakeLists.txt directly.")
    endif()

    execute_process(
      COMMAND ${CMAKE_COMMAND} --build ${_catalyst_build} --target MLIRMBQC
              MLIRQRef MLIRQuantum -- -j${CATALYST_BUILD_PARALLELISM}
      RESULT_VARIABLE _catalyst_rc)
    if(NOT _catalyst_rc EQUAL 0)
      message(
        FATAL_ERROR
          "FindCatalyst: MLIRQuantum build failed (exit ${_catalyst_rc})")
    endif()
    foreach(_hdr_dir Quantum QRef MBQC)
      file(COPY ${_catalyst_src}/mlir/include/${_hdr_dir}
           DESTINATION ${_catalyst_install}/include)
      if(EXISTS ${_catalyst_build}/include/${_hdr_dir})
        file(COPY ${_catalyst_build}/include/${_hdr_dir}
             DESTINATION ${_catalyst_install}/include)
      endif()
    endforeach()

  else()
    message(
      STATUS "FindCatalyst: reusing cached auto-build at ${_catalyst_install}")
  endif()

  set(CATALYST_ROOT "${_catalyst_install}")
endif()

foreach(_lib MLIRMBQC MLIRQRef MLIRQuantum)
  execute_process(
    COMMAND find ${_catalyst_build} -name "lib${_lib}.a"
    OUTPUT_VARIABLE _catalyst_lib_path
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT _catalyst_lib_path)
    message(
      FATAL_ERROR
        "FindCatalyst: built target '${_lib}' but couldn't locate lib${_lib}.a under ${_catalyst_build}"
    )
  endif()
  file(COPY ${_catalyst_lib_path} DESTINATION ${_catalyst_install}/lib)
endforeach()

# ---- Locate headers and library, whether auto-fetched or pre-built -------
find_path(
  CATALYST_INCLUDE_DIR
  NAMES Quantum/IR/QuantumDialect.h
  HINTS ${CATALYST_ROOT} ${CATALYST_DIR} ENV CATALYST_INSTALL_PREFIX
  PATH_SUFFIXES include)

function(_catalyst_find_lib _var)
  find_library(
    ${_var}
    NAMES ${ARGN}
    HINTS ${CATALYST_ROOT} ${CATALYST_DIR} ENV CATALYST_INSTALL_PREFIX
    PATH_SUFFIXES lib lib64)
  mark_as_advanced(${_var})
endfunction()

_catalyst_find_lib(CATALYST_DIALECT_LIBRARY MLIRQuantum)
_catalyst_find_lib(CATALYST_QREF_LIBRARY MLIRQRef)
_catalyst_find_lib(CATALYST_MBQC_LIBRARY MLIRMBQC)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Catalyst REQUIRED_VARS CATALYST_INCLUDE_DIR CATALYST_DIALECT_LIBRARY
                         CATALYST_QREF_LIBRARY CATALYST_MBQC_LIBRARY)

if(CATALYST_FOUND)
  set(CATALYST_INCLUDE_DIRS ${CATALYST_INCLUDE_DIR})

  if(NOT TARGET CATALYST::MBQC)
    add_library(CATALYST::MBQC UNKNOWN IMPORTED GLOBAL)
    set_target_properties(
      CATALYST::MBQC
      PROPERTIES IMPORTED_LOCATION "${CATALYST_MBQC_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${CATALYST_INCLUDE_DIRS}"
                 # Best-guess transitive deps -- correct against real link
                 # errors,
                 # same as CUDAQ::Quake/CUDAQ::CC earlier in this project.
                 INTERFACE_LINK_LIBRARIES "MLIRIR;MLIRSupport;MLIRFuncDialect")
  endif()
  if(NOT TARGET CATALYST::QRef)
    add_library(CATALYST::QRef UNKNOWN IMPORTED GLOBAL)
    set_target_properties(
      CATALYST::QRef
      PROPERTIES IMPORTED_LOCATION "${CATALYST_QREF_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${CATALYST_INCLUDE_DIRS}"
                 # Best-guess transitive deps -- correct against real link
                 # errors,
                 # same as CUDAQ::Quake/CUDAQ::CC earlier in this project.
                 INTERFACE_LINK_LIBRARIES
                 "CATALYST::MBQC;MLIRIR;MLIRSupport;MLIRFuncDialect")
  endif()
  if(NOT TARGET CATALYST::Quantum)
    add_library(CATALYST::Quantum UNKNOWN IMPORTED GLOBAL)
    set_target_properties(
      CATALYST::Quantum
      PROPERTIES IMPORTED_LOCATION "${CATALYST_DIALECT_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${CATALYST_INCLUDE_DIRS}"
                 # Best-guess transitive deps -- correct against real link
                 # errors,
                 # same as CUDAQ::Quake/CUDAQ::CC earlier in this project.
                 INTERFACE_LINK_LIBRARIES
                 "CATALYST::QRef;MLIRIR;MLIRSupport;MLIRFuncDialect")
  endif()

endif()
