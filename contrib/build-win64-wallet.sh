#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
JOBS="${JOBS:-1}"
HOST="${HOST:-x86_64-w64-mingw32}"
BUILD_HOST="${BUILD_HOST:-$("$ROOT/depends/config.guess")}"
PREFIX="$ROOT/depends/$HOST"
NATIVE_BIN="$ROOT/depends/build/$BUILD_HOST/bin"
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
  match="$(find "$ROOT/depends/$BUILD_HOST" \( -path "*/bin/$name" -o -path "*/libexec/$name" \) -type f | sort | tail -n 1 || true)"
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

  for tool in moc uic rcc lrelease; do
    if [[ ! -x "$NATIVE_BIN/$tool" ]]; then
      copy_first_match "$tool"
    fi
  done

  if [[ ! -x "$NATIVE_BIN/lupdate" ]] && command -v lupdate >/dev/null 2>&1; then
    cp "$(command -v lupdate)" "$NATIVE_BIN/lupdate"
  fi

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
echo "Building native Qt6 host tools and metadata for $BUILD_HOST..."
# Keep the top-level depends scheduler serial. Individual package build
# systems still use parallelism, but parallel package configure steps race on
# the shared depends prefix and can make Qt see missing zlib/OpenSSL paths.
make -C depends HOST="$BUILD_HOST" NO_QT=0 qt -j1
qt_host_cache=( "$ROOT/depends/built/$BUILD_HOST/qt"/qt-*.tar.gz )
if [[ ${#qt_host_cache[@]} -ne 1 || ! -f "${qt_host_cache[0]}" ]]; then
  echo "Could not locate a single native Qt6 cache archive for $BUILD_HOST" >&2
  exit 1
fi
mkdir -p "$ROOT/depends/$BUILD_HOST"
tar --no-same-owner -xf "${qt_host_cache[0]}" -C "$ROOT/depends/$BUILD_HOST"
make -C depends HOST="$HOST" NO_QT=0 -j1
require_path "$PREFIX/share/config.site"
ensure_native_tools

if [[ ! -f configure || ! -f src/secp256k1/configure || ! -f src/secp256k1/Makefile.in ]]; then
  ./autogen.sh
fi

if [[ build-aux/m4/bitcoin_qt.m4 -nt configure || build-aux/m4/bitcoin_qt.m4 -nt aclocal.m4 ]]; then
  ./autogen.sh
fi

echo "Configuring Win64 Qt6 wallet build..."
CONFIG_SITE="$PREFIX/share/config.site" ./configure \
  --prefix=/ \
  --disable-maintainer-mode \
  --disable-tests \
  --disable-bench \
  --with-gui=qt6 \
  --with-qt-incdir="$PREFIX/include" \
  --with-qt-libdir="$PREFIX/lib" \
  --with-qt-plugindir="$PREFIX/plugins" \
  --with-qt-translationdir="$PREFIX/translations" \
  --with-qt-bindir="$NATIVE_BIN" \
  --with-protoc-bindir="$NATIVE_BIN"

echo "Cleaning stale target objects before compiling..."
make clean

echo "Building Win64 Qt6 wallet with JOBS=$JOBS..."
make -j"$JOBS"

echo "Windows wallet build complete:"
echo "  $ROOT/src/qt/agrarian-qt.exe"
echo "  $ROOT/src/agrariand.exe"
echo "  $ROOT/src/agrarian-cli.exe"
echo "  $ROOT/src/agrarian-tx.exe"
