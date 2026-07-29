#!/bin/bash

source /workspaces/compilers/core-plugins-catalyst/scripts/setup.sh

# Optimize with MQSS (Our Compiler)
# Input: Catalyst-quantum dialect
# Output: Catalyst-quantum dialect (Optimized)
mqss-opt ../catalyst-quantum/HadamardXGateSwitchPass.quantum --passes=CommonSwitchHXPass -o optimized.mlir

# Convert catalyst-quantum to MQTOpt dialect using MQT's Catalyst Converter plugin
# Input: Optimized Catalyst-quantum dialect
# Output: MQTOpt dialect
# export catalyst=/Path/to/catalyst
catalyst --tool=opt --load-pass-plugin='/workspaces/compilers/core-plugins-catalyst/build/lib/mqt-core-plugins-catalyst.so' \
            --load-dialect-plugin='/workspaces/compilers/core-plugins-catalyst/build/lib/mqt-core-plugins-catalyst.so' \
            --catalyst-pipeline="builtin.module(catalystquantum-to-mqtopt)" optimized.mlir > mqt-opt.mlir

# Convert MQTOpt to QIR via MQTRef
# Input: Optimized MQTOpt dialect
# Output: QIR
# export quantum-opt=/Path/to/quantum-opt
# Note: The Conversion is not direct
quantum-opt -split-input-file --mqtopt-to-mqtref --mqtref-to-qir mqt-opt.mlir -o out.qir
