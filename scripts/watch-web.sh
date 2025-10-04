#!/usr/bin/env bash
set -euo pipefail

if ! command -v entr >/dev/null 2>&1; then
  echo "error: entr is required (brew install entr)." >&2
  exit 1
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WATCH_LIST=$(cd "${ROOT_DIR}" && rg --files -g"*.c" -g"*.h" -g"*.glsl")

echo "Watching source files for changes..."
printf "%s
" ${WATCH_LIST} | entr -c "${ROOT_DIR}/scripts/build-web.sh"
