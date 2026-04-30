#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
JOBS="${JOBS:-1}"
HOST="${HOST:-x86_64-pc-linux-gnu}"
PREFIX="$ROOT/depends/$HOST"
BASE_CONFIG="$PREFIX/share/config.site"
BUILD_HOST="${BUILD_HOST:-$("$ROOT/depends/config.guess")}"
NATIVE_BIN="$ROOT/depends/build/$BUILD_HOST/bin"
PROTOC="$NATIVE_BIN/protoc"

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

reset_qt_configure_state() {
  local qt_work="$ROOT/depends/work/build/$HOST/qt"
  [[ -d "$qt_work" ]] || return 0

  echo "Clearing stale Qt configure state for $HOST..."
  rm -rf "$qt_work"
}

cd "$ROOT"

require_cmd make
require_cmd pkg-config
require_cmd gcc
require_cmd g++
require_cmd cmake
require_cmd ninja

reset_qt_configure_state

echo "Building native depends for $HOST..."
make -C depends HOST="$HOST" NO_QT=0 -j"$JOBS"
require_path "$BASE_CONFIG"
require_path "$PROTOC"

if [[ ! -f configure ]]; then
  ./autogen.sh
fi

echo "Configuring Ubuntu Qt6 wallet build..."
CONFIG_SITE="$BASE_CONFIG" ./configure \
  --disable-maintainer-mode \
  --disable-tests \
  --disable-bench \
  --with-gui=qt6 \
  --with-qtdbus=no \
  --with-protoc-bindir="$NATIVE_BIN"

echo "Building Ubuntu Qt wallet with JOBS=$JOBS..."
make -j"$JOBS"

echo "Linux wallet build complete:"
echo "  $ROOT/src/qt/agrarian-qt"
echo "  $ROOT/src/agrariand"
echo "  $ROOT/src/agrarian-cli"
echo "  $ROOT/src/agrarian-tx"
