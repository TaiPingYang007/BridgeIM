#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BIN_DIR="${SCRIPT_DIR}/bin"

printf '[1/4] Cleaning old build artifacts...\n'
rm -rf "${BUILD_DIR}" "${BIN_DIR}"

printf '[2/4] Configuring CMake...\n'
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}"

printf '[3/4] Building BridgeIM...\n'
cmake --build "${BUILD_DIR}" -j"$(nproc)"

printf '[4/4] Done. Executables are under %s\n' "${BIN_DIR}"
