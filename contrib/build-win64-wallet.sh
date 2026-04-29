#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
JOBS="${JOBS:-1}"
HOST="${HOST:-x86_64-w64-mingw32}"
PREFIX="$ROOT/depends/$HOST"
NATIVE_BIN="$PREFIX/native/bin"
PROTOBUF_VERSION="${PROTOBUF_VERSION:-2.6.1}"
PROTOBUF_SOURCE="$ROOT/depends/sources/protobuf-$PROTOBUF_VERSION.tar.bz2"
PROTOBUF_BUILD="${PROTOBUF_BUILD:-/tmp/agrarian-protobuf-$PROTOBUF_VERSION-native}"

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

copy_first_match() {
  local name="$1"
  local match
  match="$(find "$ROOT/depends/work/build/$HOST" -path "*/qtbase/bin/$name" -type f | sort | tail -n 1 || true)"
  if [[ -z "$match" ]]; then
    echo "Could not find Qt host tool after depends build: $name" >&2
    exit 1
  fi
  cp "$match" "$NATIVE_BIN/$name"
}

ensure_posix_mingw() {
  require_cmd "$HOST-g++"
  if ! "$HOST-g++" --version | head -n 1 | grep -qi posix; then
    cat >&2 <<EOF
The active $HOST-g++ is not the POSIX threading variant.

On Ubuntu, install the POSIX package and select it:
  sudo apt-get install -y g++-mingw-w64-x86-64-posix
  sudo update-alternatives --set $HOST-gcc /usr/bin/$HOST-gcc-posix
  sudo update-alternatives --set $HOST-g++ /usr/bin/$HOST-g++-posix
EOF
    exit 1
  fi
}

build_native_protoc() {
  require_path "$PROTOBUF_SOURCE"
  rm -rf "$PROTOBUF_BUILD"
  mkdir -p "$PROTOBUF_BUILD"
  tar -xjf "$PROTOBUF_SOURCE" -C "$PROTOBUF_BUILD" --strip-components=1
  chmod -R u+rwX "$PROTOBUF_BUILD"
  (
    cd "$PROTOBUF_BUILD"
    bash configure --disable-shared --without-zlib
    make -C src protoc -j"$JOBS" SHELL=/bin/bash
  )
  cp "$PROTOBUF_BUILD/src/protoc" "$NATIVE_BIN/protoc"
}

ensure_native_tools() {
  mkdir -p "$NATIVE_BIN"

  for tool in moc uic rcc; do
    if [[ ! -x "$NATIVE_BIN/$tool" ]]; then
      copy_first_match "$tool"
    fi
  done

  for tool in lrelease lupdate; do
    if [[ ! -x "$NATIVE_BIN/$tool" ]]; then
      require_cmd "$tool"
      cp "$(command -v "$tool")" "$NATIVE_BIN/$tool"
    fi
  done

  if [[ ! -x "$NATIVE_BIN/protoc" ]] || ! "$NATIVE_BIN/protoc" --version | grep -q "libprotoc $PROTOBUF_VERSION"; then
    build_native_protoc
  fi
}

cd "$ROOT"

require_cmd make
require_cmd tar
require_cmd find
require_cmd sort
require_cmd tail
require_cmd grep
require_cmd cp
ensure_posix_mingw

echo "Building Win64 depends for $HOST..."
make -C depends HOST="$HOST" NO_QT=0 -j"$JOBS"
require_path "$PREFIX/share/config.site"
ensure_native_tools

if [[ ! -f configure ]]; then
  ./autogen.sh
fi

echo "Configuring Win64 Qt wallet build..."
CONFIG_SITE="$PREFIX/share/config.site" ./configure \
  --prefix=/ \
  --disable-maintainer-mode \
  --disable-tests \
  --disable-bench \
  --with-gui=qt5 \
  --with-qt-incdir="$PREFIX/include" \
  --with-qt-libdir="$PREFIX/lib"

echo "Building Win64 Qt wallet with JOBS=$JOBS..."
make -j"$JOBS"

echo "Windows wallet build complete:"
echo "  $ROOT/src/qt/agrarian-qt.exe"
echo "  $ROOT/src/agrariand.exe"
echo "  $ROOT/src/agrarian-cli.exe"
echo "  $ROOT/src/agrarian-tx.exe"
