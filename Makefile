
CATALYST_CCDB = build/tools/mqss-catalyst/compile_commands.json
CUDAQ_CCDB    = build/tools/mqss-cudaq/compile_commands.json
MERGED_CCDB   = build/compile_commands.json

MQSS_INSTALL_DIR = /workspaces/mqss-install

.PHONY: mqss-cudaq mqss-catalyst all


mqss-cudaq:
	./scripts/build_cudaq.sh --install-dir ${MQSS_INSTALL_DIR}
	$(MAKE) merge-one SRC=$(CUDAQ_CCDB)

mqss-catalyst:
	./scripts/build_catalyst.sh
	$(MAKE) merge-one SRC=$(CATALYST_CCDB)

all: mqss-cudaq mqss-catalyst

ccdb-clean:
	rm -f $(MERGED_CCDB)

clean:
	rm -rf ${MQSS_INSTALL_DIR}/bin/mqss-catalyst
	rm -rf ${MQSS_INSTALL_DIR}/bin/mqss-cudaq 

merge-one:
	@if [ ! -f $(MERGED_CCDB) ]; then \
	  cp $(SRC) $(MERGED_CCDB); \
	else \
	  jq -s 'add' $(MERGED_CCDB) $(SRC) > $(MERGED_CCDB).tmp && \
	  mv $(MERGED_CCDB).tmp $(MERGED_CCDB); \
	fi


