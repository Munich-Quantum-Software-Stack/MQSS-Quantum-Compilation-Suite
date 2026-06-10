include(FetchContent)

FetchContent_Declare(
  qdmi
  GIT_REPOSITORY https://github.com/Munich-Quantum-Software-Stack/QDMI.git
  GIT_TAG develop)

set(BUILD_QDMI_EXAMPLES ON  CACHE BOOL "" FORCE)
set(BUILD_QDMI_TESTS    OFF CACHE BOOL "" FORCE)
set(BUILD_QDMI_TEMPLATES OFF CACHE BOOL "" FORCE)
set(BUILD_QDMI_DOCS     OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(qdmi)