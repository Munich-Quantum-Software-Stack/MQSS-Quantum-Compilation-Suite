
################ User Settings#######################

NUM_JOBS = 4

BUILD_DIR=$(CURDIR)/build

INSTALL_DIR ?= $(BUILD_DIR)/bin

INSTALL_PATH = --install-dir ${INSTALL_DIR}

DEBUG_FLAG =		# --debug (for pass debug info)

################# Paths to Dependencies ###############

DEPS_DIR=$(CURDIR)/build/_deps
VENV_DIR=$(DEPS_DIR)/.venv

SRC_SCRIPT=scripts/mqss-cc
RESOLVER_SCRIPT=scripts/resolve_python_input.py

######################################################

##################### Key Commands ###################

.PHONY: mqss-opt setup-env

setup-env:
	@./scripts/setup-env.sh

front-end-paths:
	echo 'export PATH="$(DEPS_DIR)/cudaq/bin:$$PATH"'
	echo 'source $(VENV_DIR)/bin/activate'
	echo 'export PATH="$(INSTALL_DIR):$$PATH"'

frontend: setup-env
	@./scripts/download_toolchains.sh
	@ln -sf $(abspath $(RESOLVER_SCRIPT)) $(INSTALL_DIR)/resolve_python_input.py
	@ln -sf $(abspath $(SRC_SCRIPT)) $(INSTALL_DIR)/mqss-cc
	@chmod +x $(INSTALL_DIR)/mqss-cc

mqss-opt:
	@./scripts/build.sh ${DEBUG_FLAG} ${INSTALL_PATH}

set-target-paths:
	echo 'export PATH="$(INSTALL_DIR):$$PATH"'
	echo 'export PATH="/usr/local/bin:$$PATH"'

target:
	@ninja -j $(NUM_JOBS) -C $(CURDIR)/build


test-dialects:
	@ninja -C $(CURDIR)/build check-mqss

test-interfaces:
	@ctest --test-dir $(CURDIR)/build --output-on-failure


test-all:
	@ninja -C $(CURDIR)/build check-mqss-code

build: mqss-opt

ccdb-clean:
	rm -f $(MERGED_CCDB)

clean:
	rm -rf ${INSTALL_DIR}

merge-one:
	@if [ ! -f $(MERGED_CCDB) ]; then \
	  cp $(SRC) $(MERGED_CCDB); \
	else \
	  jq -s 'add' $(MERGED_CCDB) $(SRC) > $(MERGED_CCDB).tmp && \
	  mv $(MERGED_CCDB).tmp $(MERGED_CCDB); \
	fi
