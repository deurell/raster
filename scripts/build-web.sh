#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/web"

if ! command -v emcmake >/dev/null 2>&1; then
  echo "error: emcmake not found. Please activate the Emscripten SDK." >&2
  exit 1
fi

if [ -f "${BUILD_DIR}/CMakeCache.txt" ]; then
  rm -rf "${BUILD_DIR}"
fi

mkdir -p "${BUILD_DIR}"

emcmake cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja "$@"
cmake --build "${BUILD_DIR}"

echo "Web build complete. Run with: emrun ${BUILD_DIR}/examples/hello_world/hello_world.html"
