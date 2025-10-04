#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FILES=$(cd "${ROOT_DIR}" && rg --files -g"*.c" -g"*.h")
if [ -z "${FILES}" ]; then
  echo "No C/C++ files found."
  exit 0
fi

cd "${ROOT_DIR}"
clang-format -i ${FILES}
