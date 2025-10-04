#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

if [ ! -f "${BUILD_DIR}/CMakeCache.txt" ]; then
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja "$@"
else
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja "$@"
fi

cmake --build "${BUILD_DIR}"
