
include(FetchContent)

FetchContent_Declare(
  qdmi
  GIT_REPOSITORY https://github.com/Munich-Quantum-Software-Stack/QDMI.git
  GIT_TAG v1.3.2)

set(BUILD_QDMI_EXAMPLES ON  CACHE BOOL "" FORCE)
set(BUILD_QDMI_TESTS    OFF CACHE BOOL "" FORCE)
set(BUILD_QDMI_TEMPLATES OFF CACHE BOOL "" FORCE)
set(BUILD_QDMI_DOCS     OFF CACHE BOOL "" FORCE)
set(_qdmi_saved_shared_libs ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(qdmi)
set(BUILD_SHARED_LIBS ${_qdmi_saved_shared_libs} CACHE BOOL "" FORCE)