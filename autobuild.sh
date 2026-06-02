#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${SCRIPT_DIR}"
BUILD_DIR="${PROJECT_ROOT}/build"

if [[ "${1:-}" == "--clean" ]]; then
  echo "Clean build: removing build directory..."
  rm -rf "${BUILD_DIR}"
fi

mkdir -p "${BUILD_DIR}"

cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" -j"$(nproc)"

echo "Build complete."
