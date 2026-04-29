
CATALYST_CCDB = build/tools/mqss-catalyst/compile_commands.json
CUDAQ_CCDB    = build/tools/mqss-cudaq/compile_commands.json
MERGED_CCDB   = build/compile_commands.json

MQSS_INSTALL_DIR = /workspaces/mqss-install

INSTALL_PATH := $(HOME)/.local/bin

SRC_SCRIPT=tools/mqss-cc
RESOLVER_SCRIPT=tools/resolve_python_input.py

.PHONY: mqss-cudaq mqss-catalyst all

mqss-cudaq:
	./scripts/build_cudaq.sh --install-dir ${INSTALL_PATH}
	$(MAKE) merge-one SRC=$(CUDAQ_CCDB)

mqss-catalyst:
	./scripts/build_catalyst.sh --install-dir ${INSTALL_PATH}
	$(MAKE) merge-one SRC=$(CATALYST_CCDB)

all: mqss-catalyst mqss-cudaq
	@mkdir -p ~/.local/bin
	@export PATH="$HOME/.local/bin:$PATH"
	@ln -sf $(abspath $(RESOLVER_SCRIPT)) $(INSTALL_PATH)
	@ln -sf $(abspath $(SRC_SCRIPT)) $(INSTALL_PATH)
	
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


