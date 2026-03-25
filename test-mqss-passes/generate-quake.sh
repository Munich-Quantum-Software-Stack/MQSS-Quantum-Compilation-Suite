#!/bin/bash

# Set the base directory and output folder
base_dir="code"
output_dir="dialects/quake"

# Ensure the output base directory exists
mkdir -p "$output_dir"
CUDAQQUAKE="/workspaces/MQSS-Passes-Suite/build/_deps/cuda-quantum/build/bin/cudaq-quake"
CUDAQOPT="/workspaces/MQSS-Passes-Suite/build/_deps/cuda-quantum/build/bin/cudaq-opt"

# Find all .cpp files in the code directory and loop through them
find "$base_dir" -type f -name "*.cpp" | while read -r cpp_file; do
    # Create the relative path for the output file by stripping the base directory part
    relative_path="${cpp_file#$base_dir/}"
    # Extract the subdirectory path from the relative path (without the filename)
    subdir=$(dirname "$relative_path")

    # Create the corresponding subdirectory in the output directory
    mkdir -p "$output_dir/$subdir"

    # Output file name based on the input .cpp file, stored in the corresponding subdirectory in folderX
    output_file="$output_dir/$subdir/$(basename "${cpp_file%.cpp}.qke")"

    # Run the cudaq-quake command
    ${CUDAQQUAKE} "$cpp_file" -o o.qke && \
    # Run the cudaq-opt command
    ${CUDAQOPT} --canonicalize --unrolling-pipeline o.qke -o "$output_file" && \
    # Remove the intermediate o.qke file
    rm -f o.qke x.qke

    echo "Processed $cpp_file -> $output_file"
done
