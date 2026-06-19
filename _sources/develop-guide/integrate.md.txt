
# Integrating the MQSS-QCS within your project

One can integrate MQSS-QCS as a cmake module using the following entry:

```sh

include(ExternalProject)

set(MQSSCI_INSTALL_DIR ${CMAKE_BINARY_DIR}/_deps/mqssci-build)

ExternalProject_Add(mqssci_external
  GIT_REPOSITORY   https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite.git
  GIT_TAG          <commit-hash or branch name>
  PREFIX            ${CMAKE_BINARY_DIR}/_deps/mqssci-build
  SOURCE_DIR        ${CMAKE_BINARY_DIR}/_deps/mqssci-src
  BINARY_DIR        ${CMAKE_BINARY_DIR}/_deps/mqssci-src   # in-source make, adjust if out-of-source supported
  CONFIGURE_COMMAND ""   # no configure step needed for plain make
  BUILD_COMMAND bash -c "make setup-env && . /workspaces/QRM/build/_deps/mqssci-src/_deps/.venv/bin/activate && make build INSTALL_DIR=${MQSSCI_INSTALL_DIR}"
  INSTALL_COMMAND   make target INSTALL_DIR=${MQSSCI_INSTALL_DIR}
  # BUILD_IN_SOURCE   TRUE  # set if module-Y's Makefile expects to run from its source root
)

set(MQSSCI_SRC_DIR ${CMAKE_BINARY_DIR}/_deps/mqssci-src)
```
