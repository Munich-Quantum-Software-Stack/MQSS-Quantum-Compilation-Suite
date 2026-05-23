
CATALYST_CCDB = build/tools/mqss-catalyst/compile_commands.json
CUDAQ_CCDB    = build/tools/mqss-cudaq/compile_commands.json
MERGED_CCDB   = build/compile_commands.json
NUM_JOBS = 4

MQSS_INSTALL_DIR = /workspaces/mqss-install

VENV_DIR=$(CURDIR)/_deps/.venv

INSTALL_PATH := $(HOME)/.local/bin

SRC_SCRIPT=tools/mqss-cc
RESOLVER_SCRIPT=tools/resolve_python_input.py

.PHONY: mqss-cudaq mqss-catalyst all

mqss-cudaq:
	./scripts/build_cudaq.sh --install-dir ${INSTALL_PATH}

mqss-catalyst:
	./scripts/build_catalyst.sh --install-dir ${INSTALL_PATH}

docs:
	@./scripts/build_docs.sh

compile_commands:
	$(MAKE) merge-one SRC=$(CUDAQ_CCDB)
	$(MAKE) merge-one SRC=$(CATALYST_CCDB)

python:
	./scripts/python-setup.sh

build:
	@ninja -j $(NUM_JOBS) -C $(CURDIR)/build/tools/mqss-catalyst
	@ninja -j $(NUM_JOBS) -C $(CURDIR)/build/tools/mqss-cudaq

test-dialects:
	@ninja -C $(CURDIR)/build/tools/mqss-catalyst check-mqss
	@ninja -C $(CURDIR)/build/tools/mqss-cudaq check-mqss

test-all:
	@ninja -C $(CURDIR)/build/tools/mqss-catalyst check-mqss
	@ninja -C $(CURDIR)/build/tools/mqss-cudaq check-mqss
	@ninja -C $(CURDIR)/build/tools/mqss-cudaq check-mqss-code
	@ninja -C $(CURDIR)/build/tools/mqss-catalyst check-mqss-code

setup-env:
	echo 'source $(VENV_DIR)/bin/activate'
	echo 'export PATH="~/.local/bin:$$PATH"'
	
all: mqss-cudaq mqss-catalyst
	@mkdir -p $(INSTALL_PATH)
	@ln -sf $(abspath $(RESOLVER_SCRIPT)) $(INSTALL_PATH)/resolve_python_input.py
	@ln -sf $(abspath $(SRC_SCRIPT)) $(INSTALL_PATH)/mqss-cc
	@chmod +x $(INSTALL_PATH)/mqss-cc 
	@echo " "
	@echo "Installed scripts to $(INSTALL_PATH)"
	@echo 'Make sure $(INSTALL_PATH) is in your PATH'
	@echo 'PLEASE RUN THE COMMAND: eval "$$(make setup-env)"'

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


