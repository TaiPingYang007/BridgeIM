#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

if [[ "${1:-}" == "--clean" ]]; then
  echo "Clean build: removing build directory..."
  rm -rf "${BUILD_DIR}"
fi

mkdir -p "${BUILD_DIR}"

# Ensure the vendored mprpc dependency is available (built into third_party/mprpc/dist).
# Skips when MPRPC_ROOT is set, or a built dist / sibling project is already present.
if [[ -z "${MPRPC_ROOT:-}" \
      && ! -f "${PROJECT_ROOT}/third_party/mprpc/dist/include/mprpcchannel.h" \
      && ! -f "${PROJECT_ROOT}/../03_rpc_framework/dist/mprpc/include/mprpcchannel.h" ]]; then
  echo "mprpc not found; preparing vendored submodule..."
  if [[ ! -f "${PROJECT_ROOT}/third_party/mprpc/CMakeLists.txt" ]]; then
    git -C "${PROJECT_ROOT}" submodule update --init --recursive
  fi
  "${SCRIPT_DIR}/mprpc.sh"
fi

cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" -j"$(nproc)"

echo "Build complete."
