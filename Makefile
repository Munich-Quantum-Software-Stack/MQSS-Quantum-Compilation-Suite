.PHONY: cudaq catalyst all

cudaq:
	./scripts/build_cudaq.sh

catalyst:
	./scripts/build_catalyst.sh

all: cudaq catalyst

