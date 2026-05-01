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

ensure_native_protoc() {
  local archive found

  if [[ -x "$PROTOC" ]]; then
    return 0
  fi

  archive="$(find "$ROOT/depends/built/$HOST/native_protobuf" \
    -name 'native_protobuf-*.tar.gz' -type f 2>/dev/null | sort | tail -n 1 || true)"

  if [[ -n "$archive" ]]; then
    echo "Extracting native protoc from $archive"
    mkdir -p "$ROOT/depends/build/$BUILD_HOST"
    tar -xzf "$archive" -C "$ROOT/depends/build/$BUILD_HOST" ./bin/protoc
  fi

  if [[ -x "$PROTOC" ]]; then
    return 0
  fi

  found="$(find "$ROOT/depends/build" "$ROOT/depends/work/staging" \
    -path '*/bin/protoc' -type f 2>/dev/null | sort | head -n 1 || true)"

  if [[ -n "$found" ]]; then
    echo "Staging native protoc from $found"
    mkdir -p "$NATIVE_BIN"
    cp "$found" "$PROTOC"
    chmod +x "$PROTOC"
  fi

  require_path "$PROTOC"
}

remove_invalid_native_protobuf_cache() {
  local archive

  archive="$(find "$ROOT/depends/built/$HOST/native_protobuf" \
    -name 'native_protobuf-*.tar.gz' -type f 2>/dev/null | sort | tail -n 1 || true)"

  [[ -n "$archive" ]] || return 0
  if tar -tzf "$archive" ./bin/protoc >/dev/null 2>&1; then
    return 0
  fi

  echo "Removing invalid native_protobuf cache without bin/protoc: $archive"
  rm -rf "$ROOT/depends/built/$HOST/native_protobuf"
}

cd "$ROOT"

require_cmd make
require_cmd pkg-config
require_cmd gcc
require_cmd g++
require_cmd cmake
require_cmd ninja

reset_qt_configure_state
remove_invalid_native_protobuf_cache

echo "Building native depends for $HOST..."
make -C depends HOST="$HOST" NO_QT=0 -j"$JOBS"
require_path "$BASE_CONFIG"
ensure_native_protoc

if [[ ! -f configure || ! -f src/secp256k1/configure || ! -f src/secp256k1/Makefile.in ]]; then
  ./autogen.sh
fi

if [[ build-aux/m4/bitcoin_qt.m4 -nt configure || build-aux/m4/bitcoin_qt.m4 -nt aclocal.m4 ]]; then
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
