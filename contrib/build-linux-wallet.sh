#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
JOBS="${JOBS:-1}"
HOST="${HOST:-x86_64-pc-linux-gnu}"
PREFIX="$ROOT/depends/$HOST"
BASE_CONFIG="$PREFIX/share/config.site"

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required command: $1" >&2
    exit 1
  fi
}

require_path() {
  if [[ ! -e "$1" ]]; then
    echo "Missing required path: $1" >&2
    exit 1
  fi
}

cd "$ROOT"

require_cmd make
require_cmd pkg-config
require_cmd gcc
require_cmd g++
require_cmd cmake
require_cmd ninja

echo "Building native depends for $HOST..."
make -C depends HOST="$HOST" NO_QT=0 -j"$JOBS"
require_path "$BASE_CONFIG"

if [[ ! -f configure ]]; then
  ./autogen.sh
fi

echo "Configuring Ubuntu Qt6 wallet build..."
CONFIG_SITE="$BASE_CONFIG" ./configure \
  --disable-maintainer-mode \
  --disable-tests \
  --disable-bench \
  --with-gui=qt6 \
  --with-qtdbus=no

echo "Building Ubuntu Qt wallet with JOBS=$JOBS..."
make -j"$JOBS"

echo "Linux wallet build complete:"
echo "  $ROOT/src/qt/agrarian-qt"
echo "  $ROOT/src/agrariand"
echo "  $ROOT/src/agrarian-cli"
echo "  $ROOT/src/agrarian-tx"
