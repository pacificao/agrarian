#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOST="x86_64-pc-linux-gnu"
DEP="$ROOT/depends/$HOST"

cd "$ROOT"

make -C depends -j"$(nproc)" NO_QT=1
make -C depends NO_QT=1 install

./autogen.sh

CONFIG_SITE="$DEP/share/config.site" \
./configure --build="$HOST" --host="$HOST" \
  --prefix="$DEP" \
  --disable-tests --disable-bench

make -j"$(nproc)"
echo "Build complete."
