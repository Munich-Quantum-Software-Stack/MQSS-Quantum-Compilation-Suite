

import lit.formats

config.name = "test-mqss-passes"

config.test_format = lit.formats.ShTest()

config.suffixes = ['.qke', '.mlir', '.cpp', '.test']

config.test_source_root = os.path.dirname(__file__)
repo_root = os.path.dirname(config.test_source_root)


config.substitutions.append(
    ('%mqss-cudaq-opt', '/root/.local/bin/mqss-cudaq-opt')
)

config.substitutions.append(
    ('%mqss-cc', '/root/.local/bin/mqss-cc')
)

config.substitutions.append(
    ('%cudaq-quake', os.path.join(repo_root, '_deps/mqss-cudaq/cudaq/bin/cudaq-quake '))
)

config.substitutions.append(
    ('%cudaq-opt', os.path.join(repo_root, '_deps/mqss-cudaq/cudaq/bin/cudaq-opt '))
)

config.substitutions.append(
    ('%mqss-catalyst-opt', '/root/.local/bin/mqss-catalyst-opt')
)

config.substitutions.append(
    ('FileCheck', os.path.join(repo_root, '_deps/mqss-catalyst/LLVM-21.1.8-toolchain/bin/FileCheck'))
)