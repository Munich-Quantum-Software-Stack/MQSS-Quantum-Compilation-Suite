

import lit.formats

config.name = "test-mqss-passes"

config.test_format = lit.formats.ShTest()

config.suffixes = ['.qke','.quantum']

config.test_source_root = os.path.dirname(__file__)


config.substitutions.append(
    ('%mqss-cudaq-opt', '/workspaces/MQSS-Passes-Suite/build/tools/mqss-cudaq/bin/mqss-cudaq-opt')
)

config.substitutions.append(
    ('%mqss-catalyst-opt', '/workspaces/MQSS-Passes-Suite/build/tools/mqss-catalyst/bin/mqss-catalyst-opt')
)

config.substitutions.append(
    ('FileCheck', '/workspaces/compilers/MQSS-Catalyst-Compiler/mlir/llvm-project/build/bin/FileCheck')
)