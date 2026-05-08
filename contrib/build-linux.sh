#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
JOBS="${JOBS:-1}"
MODE="${MODE:-daemon}"
DAEMON_TARGETS="${DAEMON_TARGETS:-agrariand agrarian-cli agrarian-tx}"
HOST="${HOST:-$("$ROOT/depends/config.guess")}"
PREFIX="$ROOT/depends/$HOST"
BASE_CONFIG="$PREFIX/share/config.site"

require_path() {
  if [[ ! -e "$1" ]]; then
    echo "Missing required path: $1" >&2
    exit 1
  fi
}

reset_configure_state() {
  rm -f config.cache config.log config.status libtool

  # Stale generated makefiles and libtool scripts can trigger rechecks with
  # old configure arguments or old autotools/libtool macros after a failed
  # build attempt.
  find . \( -name Makefile -o -name config.status -o -name config.log -o -name libtool \) \
    ! -path './depends/*' \
    ! -path './.git/*' \
    -delete
}

cd "$ROOT"

case "$MODE" in
  daemon)
    echo "Building native daemon depends for $HOST..."
    # The legacy depends system mutates depends/$HOST while configuring each
    # package, so package-level parallelism can race and remove headers/libs
    # another package is probing. Keep depends serial; use JOBS for the final
    # Agrarian compile below.
    make -C depends clean
    make -C depends HOST="$HOST" NO_QT=1 -j1
    require_path "$BASE_CONFIG"

    reset_configure_state
    ./autogen.sh

    echo "Configuring Linux daemon build..."
    CONFIG_SITE="$BASE_CONFIG" ./configure \
      --without-gui \
      --disable-tests \
      --disable-bench \
      --disable-zmq \
      --with-miniupnpc=no \
      CXXFLAGS="${CXXFLAGS:--O0 -g0 --param ggc-min-expand=1 --param ggc-min-heapsize=32768}"

    echo "Cleaning stale target objects before compiling..."
    make clean
    ;;
  wallet)
    exec "$ROOT/contrib/build-linux-wallet.sh"
    ;;
  *)
    echo "Unknown MODE: $MODE" >&2
    echo "Use MODE=daemon or MODE=wallet." >&2
    exit 2
    ;;
esac

if [[ "$MODE" == "daemon" ]]; then
  make -j"$JOBS" $DAEMON_TARGETS
else
  make -j"$JOBS"
fi
echo "Linux $MODE build complete."
