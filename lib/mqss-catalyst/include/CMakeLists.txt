add_subdirectory(IR)
add_subdirectory(MQSSCatalystPasses)

# Add include directory
# target_include_directories(MLIRPasses PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/Passes
# )