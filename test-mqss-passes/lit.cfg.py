

import lit.formats

config.name = "test-mqss-passes"

config.test_format = lit.formats.ShTest()

config.suffixes = ['.qke','.quantum']

config.test_source_root = os.path.dirname(__file__)


config.substitutions.append(
    ('%mqss-cudaq-opt', '/root/.local/bin/mqss-cudaq-opt')
)

config.substitutions.append(
    ('%mqss-catalyst-opt', '/root/.local/bin/mqss-catalyst-opt')
)

config.substitutions.append(
    ('FileCheck', '/workspaces/MQSS-Passes-Suite/build/_deps/LLVM-21.1.8-toolchain/bin/FileCheck')
)