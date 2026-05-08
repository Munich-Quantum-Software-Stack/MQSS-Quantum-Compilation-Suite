include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

FetchContent_Declare(
  mqt-qmap
  GIT_REPOSITORY https://github.com/cda-tum/mqt-qmap.git
  GIT_TAG 186b9dcb5bc3395f865e734bc69dc6887071045f)

FetchContent_MakeAvailable(mqt-qmap)

FetchContent_GetProperties(mqt-qmap)

set(QMAP_INCLUDE_DIRS "${mqt-qmap_SOURCE_DIR}/include")
