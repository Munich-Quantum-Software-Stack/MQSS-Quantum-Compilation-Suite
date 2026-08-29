

import lit.formats

config.name = "test-mqss-passes"

config.test_format = lit.formats.ShTest()

config.suffixes = ['.qke', '.mlir', '.cpp', '.test']

config.test_source_root = os.path.dirname(__file__)
repo_root = os.path.dirname(config.test_source_root)

config.substitutions.append(
    ('%mqss-opt', os.path.join(repo_root, 'build/bin/mqss-opt'))
)

#TODO: Following path should be updated to point to the correct location of the mqss-cc binary.
# Currently, it is set to a placeholder path that may not exist in your environment.
config.substitutions.append(
    ('%mqss-cc', os.path.join(repo_root, 'build/bin/mqss-cc'))
)

#TODO: Following path should be updated to point to the correct location of the cudaq-quake binary.
# Currently, it is set to a placeholder path that may not exist in your environment.
# Please update it to the correct path where the cudaq-quake binary is located.
config.substitutions.append(
    ('%cudaq-quake', os.path.join(repo_root, 'opt/deps/cudaq/bin/cudaq-quake'))
)


config.substitutions.append(
    ('FileCheck', os.path.join(repo_root, '/opt/deps/llvm/bin/FileCheck'))
)

config.substitutions.append(
    ('%qdmi-cxx-device-so', os.path.join(repo_root, 'build/_deps/qdmi-build/examples/device/src/libcxx-qdmi-device.so'))
)
