
set(Boost_NO_SYSTEM_PATHS OFF)
set(Boost_NO_BOOST_CMAKE ON) # Sometimes helps bypass buggy config files

# Provide the search hint
find_package(Boost REQUIRED 
    COMPONENTS program_options 
    PATHS /usr 
    NO_DEFAULT_PATH
)

if(Boost_FOUND)
    message(STATUS "Found system Boost; skipping FetchContent.")
    return() # Exit this file early!
endif()

# Include FetchContent module
include(FetchContent)

# Declare Boost details
FetchContent_Declare(
  boost
  GIT_REPOSITORY https://github.com/boostorg/boost.git
  GIT_TAG master # or a specific version tag, e.g., boost-1.81.0
)

# Populate Boost
FetchContent_MakeAvailable(boost)
