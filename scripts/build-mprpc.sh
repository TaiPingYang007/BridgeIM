#!/usr/bin/env bash
# Build the vendored mprpc submodule into third_party/mprpc/dist so that
# BridgeIM's CMake (cmake/Mprpc.cmake) can find it from a fresh clone.
#
# Usage: ./scripts/build-mprpc.sh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
MPRPC_SRC="${PROJECT_ROOT}/third_party/mprpc"
MPRPC_BUILD="${MPRPC_SRC}/build"
MPRPC_DIST="${MPRPC_SRC}/dist"

if [[ ! -f "${MPRPC_SRC}/CMakeLists.txt" ]]; then
  echo "error: mprpc submodule not found at ${MPRPC_SRC}." >&2
  echo "       run: git submodule update --init --recursive" >&2
  exit 1
fi

echo "[build-mprpc] configuring..."
cmake -S "${MPRPC_SRC}" -B "${MPRPC_BUILD}" -DCMAKE_BUILD_TYPE=Release

echo "[build-mprpc] building static library (target: mprpc)..."
cmake --build "${MPRPC_BUILD}" --target mprpc -j"$(nproc)"

echo "[build-mprpc] staging headers + lib into ${MPRPC_DIST}..."
rm -rf "${MPRPC_DIST}"
mkdir -p "${MPRPC_DIST}/lib" "${MPRPC_DIST}/include"
cp "${MPRPC_BUILD}/lib/libmprpc.a" "${MPRPC_DIST}/lib/"
cp "${MPRPC_SRC}/src/include/"*.h "${MPRPC_DIST}/include/"

echo "[build-mprpc] done -> ${MPRPC_DIST} (include/ + lib/libmprpc.a)"
