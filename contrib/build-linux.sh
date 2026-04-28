#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
JOBS="${JOBS:-1}"

cd "$ROOT"

./autogen.sh

./configure \
  --without-gui \
  --disable-tests \
  --disable-bench \
  --disable-zmq \
  --with-miniupnpc=no \
  --with-incompatible-bdb \
  CXXFLAGS="${CXXFLAGS:--O0 -g0 --param ggc-min-expand=1 --param ggc-min-heapsize=32768}"

make -j"$JOBS"
echo "Build complete."
