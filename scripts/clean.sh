#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
for dir in build web .cache; do
  if [ -d "${ROOT_DIR}/${dir}" ]; then
    echo "Removing ${dir}/"
    rm -rf "${ROOT_DIR}/${dir}"
  fi
done
