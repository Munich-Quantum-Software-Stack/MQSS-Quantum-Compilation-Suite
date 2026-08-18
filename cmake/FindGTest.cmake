include(FetchContent)

set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)
set(GTEST_VERSION
    1.17.0
    CACHE STRING "Google Test version")
set(GTEST_URL
    https://github.com/google/googletest/archive/refs/tags/v${GTEST_VERSION}.tar.gz
)
set(INSTALL_GTEST
    OFF
    CACHE BOOL "Disable GoogleTest installation")

FetchContent_Declare(googletest URL ${GTEST_URL} FIND_PACKAGE_ARGS
                                    ${GTEST_VERSION} NAMES GTest)
FetchContent_MakeAvailable(googletest)
