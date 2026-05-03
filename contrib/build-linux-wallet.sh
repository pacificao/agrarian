#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
JOBS="${JOBS:-1}"
BUILD_HOST="${BUILD_HOST:-$("$ROOT/depends/config.guess")}"
HOST="${HOST:-$BUILD_HOST}"
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

reset_qt_configure_state() {
  local qt_work="$ROOT/depends/work/build/$HOST/qt"
  [[ -d "$qt_work" ]] || return 0

  echo "Clearing stale Qt configure state for $HOST..."
  rm -rf "$qt_work"
}

detect_system_pkg_config_libdir() {
  local dirs=()
  local multiarch pc_path dir

  if command -v dpkg-architecture >/dev/null 2>&1; then
    multiarch="$(dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null || true)"
    [[ -n "$multiarch" ]] && dirs+=("/usr/lib/$multiarch/pkgconfig" "/lib/$multiarch/pkgconfig")
  fi

  multiarch="$(gcc -print-multiarch 2>/dev/null || true)"
  [[ -n "$multiarch" ]] && dirs+=("/usr/lib/$multiarch/pkgconfig" "/lib/$multiarch/pkgconfig")

  IFS=: read -r -a pc_path <<<"$(pkg-config --variable pc_path pkg-config 2>/dev/null || true)"
  dirs+=("${pc_path[@]}" /usr/lib/pkgconfig /usr/share/pkgconfig)

  local unique=()
  for dir in "${dirs[@]}"; do
    [[ -n "$dir" && -d "$dir" ]] || continue
    case ":${unique[*]}:" in
      *":$dir:"*) ;;
      *) unique+=("$dir") ;;
    esac
  done

  (IFS=:; echo "${unique[*]}")
}

preflight_linux_qt_deps() {
  local missing=()
  local modules=(
    x11 x11-xcb xcb xcb-cursor xcb-icccm xcb-image xcb-keysyms
    xcb-randr xcb-render xcb-renderutil xcb-shape xcb-shm xcb-sync
    xcb-xfixes xcb-xkb xkbcommon xkbcommon-x11 fontconfig freetype2
    harfbuzz zlib libpng libbrotlidec libbrotlicommon expat glib-2.0
    graphite2
  )
  local module

  export QT_SYSTEM_PKG_CONFIG_LIBDIR="${QT_SYSTEM_PKG_CONFIG_LIBDIR:-$(detect_system_pkg_config_libdir)}"

  for module in "${modules[@]}"; do
    if ! PKG_CONFIG_LIBDIR="$QT_SYSTEM_PKG_CONFIG_LIBDIR" pkg-config --exists "$module"; then
      missing+=("$module")
    fi
  done

  if ((${#missing[@]})); then
    cat >&2 <<EOF
Missing Linux Qt/XCB development pkg-config modules:
  ${missing[*]}

Install the Ubuntu packages from contrib/agrarian-build-menu.sh, then rerun.
For a direct build on Ubuntu 24.04, start with:
  sudo apt-get install libfontconfig1-dev libfreetype6-dev libharfbuzz-dev libbrotli-dev libbz2-dev libexpat1-dev libglib2.0-dev libgraphite2-dev libpng-dev zlib1g-dev libx11-xcb-dev libxcb1-dev libxcb-cursor-dev libxcb-icccm4-dev libxcb-image0-dev libxcb-keysyms1-dev libxcb-randr0-dev libxcb-render0-dev libxcb-render-util0-dev libxcb-shape0-dev libxcb-shm0-dev libxcb-sync-dev libxcb-util-dev libxcb-xfixes0-dev libxcb-xinerama0-dev libxcb-xkb-dev libxau-dev libxdmcp-dev libxext-dev libxi-dev libxrender-dev libxkbcommon-dev libxkbcommon-x11-dev
EOF
    exit 1
  fi

  echo "Using system pkg-config path for Qt/XCB:"
  echo "  $QT_SYSTEM_PKG_CONFIG_LIBDIR"
}

reset_configure_state() {
  rm -f config.cache config.log config.status libtool

  # Stale generated makefiles and libtool scripts can trigger rechecks with
  # old autotools/libtool macros after a failed or moved build.
  find . \( -name Makefile -o -name config.status -o -name config.log -o -name libtool \) \
    ! -path './depends/*' \
    ! -path './.git/*' \
    -delete
}

cd "$ROOT"

require_cmd make
require_cmd pkg-config
require_cmd gcc
require_cmd g++
require_cmd cmake
require_cmd ninja

if [[ "$HOST" == "$BUILD_HOST" ]]; then
  preflight_linux_qt_deps
else
  echo "Linux Qt wallet build expects a native HOST. Got HOST=$HOST BUILD_HOST=$BUILD_HOST" >&2
  exit 1
fi

reset_qt_configure_state

echo "Building native depends for $HOST..."
# The legacy depends system mutates depends/$HOST while configuring each
# package. Package-level parallelism can race and remove headers/libs another
# package is probing, so keep depends serial and use JOBS for the final wallet
# compile below.
make -C depends clean
make -C depends HOST="$HOST" NO_QT=0 -j1
require_path "$BASE_CONFIG"

reset_configure_state
./autogen.sh

echo "Configuring Ubuntu Qt6 wallet build..."
QT_SYSTEM_PKG_CONFIG_LIBDIR="$QT_SYSTEM_PKG_CONFIG_LIBDIR" CONFIG_SITE="$BASE_CONFIG" ./configure \
  --disable-maintainer-mode \
  --disable-tests \
  --disable-bench \
  --with-gui=qt6 \
  --with-qtdbus=no

echo "Cleaning stale target objects before compiling..."
make clean

echo "Building Ubuntu Qt wallet with JOBS=$JOBS..."
make -j"$JOBS"

echo "Linux wallet build complete:"
echo "  $ROOT/src/qt/agrarian-qt"
echo "  $ROOT/src/agrariand"
echo "  $ROOT/src/agrarian-cli"
echo "  $ROOT/src/agrarian-tx"
