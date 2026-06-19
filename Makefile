
################ User Settings#######################

MQSS_INSTALL_DIR = /workspaces/mqss-install

NUM_JOBS = 4

INSTALL_DIR ?= $(HOME)/.local/bin

INSTALL_PATH = --install-dir ${INSTALL_DIR}

DEBUG_FLAG =		# --debug (if you want pass debug info)

######################################################

##################### Clangd Paths ###################

CATALYST_CCDB = build/lib/mqss-catalyst/compile_commands.json
CUDAQ_CCDB    = build/lib/mqss-cudaq/compile_commands.json
MERGED_CCDB   = build/compile_commands.json

######################################################

################# Paths to Dependencies ###############

DEPS_DIR=$(CURDIR)/_deps
VENV_DIR=$(DEPS_DIR)/.venv

SRC_SCRIPT=scripts/mqss-cc
RESOLVER_SCRIPT=scripts/resolve_python_input.py
CUDAQ_QUAKE=_deps/mqss-cudaq/cudaq/bin/cudaq-quake

CMAKE_BIN=_deps/cmake-3.29.0-linux-aarch64/bin/cmake

######################################################

##################### Key Commands ###################

.PHONY: mqss-cudaq mqss-catalyst all

mqss-cudaq:
	@./scripts/build_cudaq.sh ${DEBUG_FLAG} ${INSTALL_PATH} 

mqss-catalyst:
	@./scripts/build_catalyst.sh ${DEBUG_FLAG} ${INSTALL_PATH} 

docs:
	./scripts/build_docs.sh

compile_commands:
	@ln -sf "$(realpath _deps/mqss-cudaq/clang+llvm-16.0.4-aarch64-linux-gnu/bin/clangd)" /usr/local/bin/clangd
	@$(MAKE) merge-one SRC=$(CUDAQ_CCDB)
	@$(MAKE) merge-one SRC=$(CATALYST_CCDB)

set-target-paths:
	echo 'source $(VENV_DIR)/bin/activate'
	echo 'export PATH="$(INSTALL_DIR):$$PATH"'
	echo 'export PATH="/usr/local/bin:$$PATH"'
	echo 'export PATH="$(DEPS_DIR)/mqss-cudaq/cudaq/bin:$$PATH"'

setup-env: 
	@./scripts/setup-env.sh
# 	@ln -sf $(abspath $(CMAKE_BIN)) $(INSTALL_DIR)/cmake

target:
	@ninja -j $(NUM_JOBS) -C $(CURDIR)/build/lib/mqss-catalyst
	@ninja -j $(NUM_JOBS) -C $(CURDIR)/build/lib/mqss-cudaq
	@ln -sf $(abspath $(RESOLVER_SCRIPT)) $(INSTALL_DIR)/resolve_python_input.py
	@ln -sf $(abspath $(SRC_SCRIPT)) $(INSTALL_DIR)/mqss-cc
	@chmod +x $(INSTALL_DIR)/mqss-cc 

test-dialects:
	@ninja -C $(CURDIR)/build/lib/mqss-catalyst check-mqss
	@ninja -C $(CURDIR)/build/lib/mqss-cudaq check-mqss

test-all:
	@ninja -C $(CURDIR)/build/lib/mqss-catalyst check-mqss
	@ninja -C $(CURDIR)/build/lib/mqss-cudaq check-mqss
	@ninja -C $(CURDIR)/build/lib/mqss-cudaq check-mqss-code
	@ninja -C $(CURDIR)/build/lib/mqss-catalyst check-mqss-code
	
build: mqss-cudaq mqss-catalyst
	@echo "	"
	@echo "Configured Build, PLEASE RUN: make target"

ccdb-clean:
	rm -f $(MERGED_CCDB)

clean:
	rm -rf ${MQSS_INSTALL_DIR}/mqss-catalyst
	rm -rf ${MQSS_INSTALL_DIR}/mqss-cudaq 

merge-one:
	@if [ ! -f $(MERGED_CCDB) ]; then \
	  cp $(SRC) $(MERGED_CCDB); \
	else \
	  jq -s 'add' $(MERGED_CCDB) $(SRC) > $(MERGED_CCDB).tmp && \
	  mv $(MERGED_CCDB).tmp $(MERGED_CCDB); \
	fi


