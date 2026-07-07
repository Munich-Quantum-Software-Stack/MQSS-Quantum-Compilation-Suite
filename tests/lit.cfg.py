

import lit.formats

config.name = "test-mqss-passes"

config.test_format = lit.formats.ShTest()

config.suffixes = ['.qke', '.mlir', '.cpp', '.test']

config.test_source_root = os.path.dirname(__file__)
repo_root = os.path.dirname(config.test_source_root)

config.substitutions.append(
    ('%mqss-opt', '/root/.local/bin/mqss-opt')
)

config.substitutions.append(
    ('%mqss-cc', '/root/.local/bin/mqss-cc')
)

config.substitutions.append(
    ('%cudaq-quake', os.path.join(repo_root, 'build/_deps/cudaq/bin/cudaq-quake '))
)

config.substitutions.append(
    ('%cudaq-opt', os.path.join(repo_root, 'build/_deps/cudaq/bin/cudaq-opt '))
)

config.substitutions.append(
    ('FileCheck', os.path.join(repo_root, 'build/_deps/LLVM-22.1.0-toolchain/bin/FileCheck'))
)