#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
JOBS="${JOBS:-1}"
HOST="${HOST:-x86_64-pc-linux-gnu}"
PREFIX="$ROOT/depends/$HOST"
BASE_CONFIG="$PREFIX/share/config.site"
CONFIG_SITE_FILE="${CONFIG_SITE_FILE:-/tmp/agrarian-linux-wallet-config.site}"
QT_PKG_CONFIG_DIR="${QT_PKG_CONFIG_DIR:-/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig}"
QT_BINDIR="${QT_BINDIR:-/usr/lib/qt5/bin}"
PROTOC_BINDIR="${PROTOC_BINDIR:-/usr/bin}"

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
require_cmd sed
require_cmd cp
require_cmd "$PROTOC_BINDIR/protoc"
require_path "$QT_BINDIR/moc"
require_path "$QT_BINDIR/uic"
require_path "$QT_BINDIR/rcc"
require_path "$QT_BINDIR/lrelease"
require_path "$QT_BINDIR/lupdate"

echo "Building native depends for $HOST..."
make -C depends -j"$JOBS"
require_path "$BASE_CONFIG"

cp "$BASE_CONFIG" "$CONFIG_SITE_FILE"
sed -i.old \
  -e "s#^with_qt_bindir=.*#with_qt_bindir='$QT_BINDIR'#" \
  -e "s#^with_protoc_bindir=.*#with_protoc_bindir='$PROTOC_BINDIR'#" \
  -e "s#^PKG_CONFIG_LIBDIR=.*#PKG_CONFIG_LIBDIR='$PREFIX/lib/pkgconfig:$PREFIX/share/pkgconfig:$QT_PKG_CONFIG_DIR'#" \
  "$CONFIG_SITE_FILE"

if [[ ! -f configure ]]; then
  ./autogen.sh
fi

echo "Configuring Ubuntu Qt wallet build..."
CONFIG_SITE="$CONFIG_SITE_FILE" ./configure \
  --disable-maintainer-mode \
  --disable-tests \
  --disable-bench \
  --with-gui=qt5

echo "Building Ubuntu Qt wallet with JOBS=$JOBS..."
make -j"$JOBS"

echo "Linux wallet build complete:"
echo "  $ROOT/src/qt/agrarian-qt"
echo "  $ROOT/src/agrariand"
echo "  $ROOT/src/agrarian-cli"
echo "  $ROOT/src/agrarian-tx"
