#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/web"

if [ ! -f "${BUILD_DIR}/CMakeCache.txt" ]; then
  echo "Web build not configured. Building..."
  "${ROOT_DIR}/scripts/build-web.sh"
fi

if command -v emrun >/dev/null 2>&1; then
  emrun --no_browser "${BUILD_DIR}/examples/hello_world/hello_world.html"
else
  echo "emrun not found; falling back to python3 -m http.server"
  (cd "${BUILD_DIR}" && python3 -m http.server)
fi
