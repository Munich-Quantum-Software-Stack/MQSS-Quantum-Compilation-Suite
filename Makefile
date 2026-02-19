
CATALYST_CCDB = build/tools/catalyst/compile_commands.json
CUDAQ_CCDB    = build/tools/cudaq/compile_commands.json
MERGED_CCDB   = build/compile_commands.json


.PHONY: cudaq catalyst all


cudaq:
	./scripts/build_cudaq.sh
	$(MAKE) merge-one SRC=$(CUDAQ_CCDB)

catalyst:
	./scripts/build_catalyst.sh
	$(MAKE) merge-one SRC=$(CATALYST_CCDB)

all: cudaq catalyst

ccdb-clean:
	rm -f $(MERGED_CCDB)

clean:
	rm -rf build/tools/catalyst
	rm -rf build/tools/cudaq 

merge-one:
	@if [ ! -f $(MERGED_CCDB) ]; then \
	  cp $(SRC) $(MERGED_CCDB); \
	else \
	  jq -s 'add' $(MERGED_CCDB) $(SRC) > $(MERGED_CCDB).tmp && \
	  mv $(MERGED_CCDB).tmp $(MERGED_CCDB); \
	fi


