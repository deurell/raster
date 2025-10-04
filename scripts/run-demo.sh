#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

if [ ! -x "${BUILD_DIR}/examples/hello_world/hello_world" ]; then
  echo "Native demo not built yet. Building..."
  "${ROOT_DIR}/scripts/build-native.sh"
fi

(cd "${BUILD_DIR}/examples/hello_world" && ./hello_world "$@")
